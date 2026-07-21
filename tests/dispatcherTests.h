#ifndef DISPATCHER_TESTS_H
#define DISPATCHER_TESTS_H

#include <thread>
#include <memory>
#include <algorithm>

#include <partest/eventdispatcher.h>
#include <partest/eventreporter.h>
#include <partest/partestbase.h>

class MockReporter : public partest::EventReporterInterface
{
	std::vector<partest::EventPair> m_logs;

	void writeLog(PARTEST_STRING_PARAM eventType, std::unique_ptr<partest::EventInterface> event)
	{
		m_logs.emplace_back(std::make_pair(eventType, std::move(event)));
	}

public:
	MockReporter() = default;
    MockReporter(const MockReporter&) = delete;
    MockReporter& operator=(const MockReporter&) = delete;
    MockReporter(MockReporter&&) = default;
    MockReporter& operator=(MockReporter&&) = default;

	void onTestBegin(const partest::EventBeginTest &event) override
	{
		writeLog(EVENT_BEGIN_TEST, event.clone());
	}

	void onTestEnd(const partest::EventEndTest &event) override
	{
		writeLog(EVENT_END_TEST, event.clone());
	}

	void onAssertion(const partest::EventAssertion &event) override
	{
		writeLog(EVENT_ASSERTION, event.clone());
	}

	void onLog(const partest::EventLog &event) override
	{
		writeLog(EVENT_LOG, event.clone());
	}

	void onPassthrough(const partest::EventPassthrough &event) override
	{
		writeLog(EVENT_PASSTHROUGH, event.clone());
	}

	void onDie(const partest::EventDie &event) override
	{
		writeLog(EVENT_DIE, event.clone());
	}

	const std::vector<partest::EventPair> &logs() const { return m_logs; }
	void clearLogs() { m_logs.clear(); }
};

class DispatcherTests : public partest::PartestBase
{
	std::unique_ptr<partest::EventDispatcherInterface> m_dispatcher;
	std::vector<MockReporter> m_reporters;
	std::vector<partest::EventPair> m_logs;

	void mirrorLog(PARTEST_STRING_PARAM eventType, std::unique_ptr<partest::EventInterface> event)
	{
		m_logs.emplace_back(std::make_pair(eventType, std::move(event)));
	}

	// Setup to initialize the dispatcher
	void setUpSerial(unsigned reporterCount)
	{
		m_dispatcher = partest::make_unique<partest::SerialEventDispatcher>();
		initializeReporting(reporterCount);

		// Initialize a series of events to pass to the logger.

		// Begin test
		mirrorLog(EVENT_BEGIN_TEST, partest::makeEventBeginTest(1, 0, "Validation"));
		// Assert fail
		mirrorLog(EVENT_ASSERTION, partest::makeEventAssertion(1, 0, partest::AssertionResult()));
		// End test
		mirrorLog(EVENT_END_TEST, partest::makeEventBeginTest(1, 0, "Validation"));
		// Passthrough log
		mirrorLog(EVENT_PASSTHROUGH, partest::makeEventPassthrough(1, 0, "Log from user code"));
		// Normal log
		mirrorLog(EVENT_LOG, partest::makeEventLog(1, 0, partest::LogEntry(partest::LogLevel::Info, PARTEST_LOG_TYPE_DEFAULT, "Framework log from test code", 1)));
	}

	void tearDownSerial()
	{
		clearReporting();
	}

	void setUpConcurrent(unsigned threadCount)
	{
		m_dispatcher = partest::make_unique<partest::ConcurrentEventDispatcher>();
		initializeReporting(threadCount);

		// Initialize a series of events to pass to the logger.
		for(unsigned x = 0; x < threadCount; ++x)
		{
			// Begin test
			mirrorLog(EVENT_BEGIN_TEST, partest::makeEventBeginTest(x, 0, "Validation"));
			// Assert fail
			mirrorLog(EVENT_ASSERTION, partest::makeEventAssertion(x, 0, partest::AssertionResult()));
			// End test
			mirrorLog(EVENT_END_TEST, partest::makeEventBeginTest(x, 0, "Validation"));
			// Passthrough log
			mirrorLog(EVENT_PASSTHROUGH, partest::makeEventPassthrough(x, 0, "Log from user code"));
			// Normal log
			mirrorLog(EVENT_LOG, partest::makeEventLog(x, 0, partest::LogEntry(partest::LogLevel::Info, PARTEST_LOG_TYPE_DEFAULT, "Framework log from test code", x)));
		}
	}

	void tearDownConcurrent()
	{
		clearReporting();
	}

	void initializeReporting(unsigned reporterCount = 2)
	{
		for(unsigned x = 0; x < reporterCount; ++x)
		{
			m_reporters.push_back(MockReporter());
		}
	}
	void clearReporting()
	{
		m_reporters.clear();
		m_logs.clear();
	}

public:
	DispatcherTests() : PartestBase("DispatcherTests", "Tests for the EventDispatcher classes.")
	{
		partest::TestFlags flags = partest::TEST_FLAGS_INHERIT;
		unsigned serialReporters = 2;
		unsigned threadCount = 6;

		addTest(partest::TestInfo("Test Serial Dispatcher", "Validate that dispatcher passes all events intact to reporters, in sequential order."),
			flags,
			[this]() { return this->SerialDispatcherPassesAllEvents(); },
			[this, serialReporters]() { return this->setUpSerial(serialReporters); },
			[this]() { return this->tearDownSerial(); }
		);
		addTest(partest::TestInfo("Test Concurrent Dispatcher", "Validate that dispatcher passes all events intact to reporters, and that order is identical between all reporters"),
			flags,
			[this, threadCount]() { return this->concurrentDispatcherPassesAllEvents(threadCount); },
			[this, threadCount]() { return this->setUpConcurrent(threadCount); },
			[this]() { return this->tearDownConcurrent(); }
		);
		addTest(partest::TestInfo("Test Serial Unpropagated Events", "Validate that the dispatcher consumes events regardles of reporter presence"),
			flags,
			[this]() { return this->eventsWithoutReportersSerial(); },
			[this]() { return this->setUpSerial(1); },
			[this]() { return this->tearDownSerial(); }
		);
		addTest(partest::TestInfo("Test Concurrent Unpropagated Events", "Validate that the concurrent dispatcher consumes events regardles of reporter presence"),
			flags,
			[this]() { return this->eventsWithoutReportersConcurrent(); },
			[this]() { return this->setUpConcurrent(1); },
			[this]() { return this->tearDownConcurrent(); }
		);
		addTest(partest::TestInfo("Test Early Concurrent Events", "Validate that dispatcher passes events pushed before dispatch begins"),
			flags,
			[this]() { return this->eventsPushedBeforeBeginDispatch(); },
			[this]() { return this->setUpConcurrent(1); },
			[this]() { return this->tearDownConcurrent(); }
		);
	}

	// Test serial dispatcher. Happy path.
	// Ensure all events are passed to the correct handlers, with original data intact.
	// Ensure that EVENT_DIE is the last event passed, when killDispatcher is invoked.
	// Ensure that all reporters record identical events to the reference logs, in identical order.
	void SerialDispatcherPassesAllEvents()
	{
		partest::EventDispatcherInterface *dispatcher = m_dispatcher.get();
		for(MockReporter &reporter: m_reporters)
			dispatcher->registerReporter(&reporter);

		// Iterate over logs
		for(unsigned x = 0; x < m_logs.size(); ++x)
		{
			dispatcher->pushEvent(m_logs[x].first,m_logs[x].second->clone());
		}

		dispatcher->killDispatcher();

		bool success = dispatcher->pushEvent(EVENT_ASSERTION, partest::makeEventAssertion(2, 0, partest::AssertionResult()));
		// A dead dispatcher should refuse any new events.
		ASSERT_FALSE(success);

		unsigned invalidEvents = 0;
		for(unsigned x = 0; x < m_logs.size(); ++x)
		{
			const partest::EventInterface &lhs = *(m_logs[x].second.get());

			for(MockReporter &reporter: m_reporters)
			{
				const partest::EventInterface &rhs = *(reporter.logs()[x].second.get());

				if(lhs != rhs)
					++invalidEvents;
			}
		}
		// All events should be identical across every firstReporter and the local reference log
		ASSERT_EQUAL(0, invalidEvents);

		for(MockReporter &reporter: m_reporters)
		{
			// Size should be the same as the reference log + the DIE event
			ASSERT_EQUAL(m_logs.size() + 1, reporter.logs().size());
			ASSERT_EQUAL(EVENT_DIE, reporter.logs().back().first);
		}
	}

	void concurrentDispatcherPassesAllEvents(unsigned threadCount)
	{
		partest::EventDispatcherInterface *dispatcher = m_dispatcher.get();
		for(MockReporter &reporter: m_reporters)
			dispatcher->registerReporter(&reporter);

		std::vector<std::thread> producerThreads;

		std::thread dispatcherThread = std::thread([dispatcher]() { dispatcher->dispatchEvents(); });

		unsigned eventCount = (unsigned)m_logs.size();
		unsigned eventsPerThread = eventCount / threadCount;

		for(unsigned x = 0; x < threadCount; ++x)
		{
			unsigned lower = x * eventsPerThread;
			unsigned upper = (x + 1) * eventsPerThread;

			std::thread producerThread = std::thread([this, dispatcher,lower, upper]() {

				for(unsigned x = lower; x < upper; ++x)
				{
					dispatcher->pushEvent(m_logs[x].first, m_logs[x].second->clone());
				}
			});

			producerThreads.push_back(std::move(producerThread));
		}

		for(unsigned x = 0; x < threadCount; ++x)
		{
			producerThreads[x].join();
		}
		dispatcher->killDispatcher();
		// Ensure no events propagate after death
		bool success = dispatcher->pushEvent(EVENT_ASSERTION, partest::makeEventAssertion(2, 0, partest::AssertionResult()));

		ASSERT_FALSE(success);

		dispatcherThread.join();

		success = dispatcher->pushEvent(EVENT_ASSERTION, partest::makeEventAssertion(2, 0, partest::AssertionResult()));
		// Ensure no events propagate after join.
		// This should be impossible because no thread is running the dispatchEvents() call.
		ASSERT_FALSE(success);

		unsigned uncopiedEvents = 0;
		// Check that all the events from m_logs were propagated to one reporter
		const MockReporter &firstReporter = m_reporters.front();
		for(const partest::EventPair &evt: m_logs)
		{
			std::vector<partest::EventPair>::const_iterator iter = std::find_if(
				firstReporter.logs().cbegin(),
				firstReporter.logs().cend(),
				[&](const partest::EventPair &pair) {
					return *pair.second == *evt.second;
				});
			if(iter == firstReporter.logs().cend())
				++uncopiedEvents;
		}

		ASSERT_EQUAL(0, uncopiedEvents);
		// Last event should always be the kill event
		ASSERT_EQUAL(EVENT_DIE, firstReporter.logs().back().first);

		unsigned invalidEventCounts = 0;
		// Ensure all reporters have the correct number of events (eventCount + DIE)
		for(const MockReporter &reporter: m_reporters)
		{
			if(reporter.logs().size() != eventCount + 1)
				++invalidEventCounts;
		}
		ASSERT_EQUAL(0, invalidEventCounts);

		unsigned invalidEvents = 0;
 		// Check that all reporters contain identical event logs and event order
		for(unsigned logIdx = 0; logIdx < firstReporter.logs().size(); ++logIdx)
		{
			const partest::EventInterface &lhs = *(firstReporter.logs()[logIdx].second.get());

			for(unsigned reporterIdx = 1; reporterIdx < m_reporters.size(); ++reporterIdx)
			{
				const partest::EventInterface &rhs = *(m_reporters[reporterIdx].logs()[logIdx].second.get());

				if(lhs != rhs)
					++invalidEvents;
			}
		}
		ASSERT_EQUAL(0, invalidEvents);
	}

	void eventsWithoutReportersSerial()
	{
		partest::EventDispatcherInterface *dispatcher = m_dispatcher.get();
		unsigned success = dispatcher->pushEvent(EVENT_ASSERTION, partest::makeEventAssertion(2, 0, partest::AssertionResult()));

		// Event without reporters registered, the event should be processed.
		ASSERT_TRUE(success);

		MockReporter &reporter = m_reporters.front();
		dispatcher->registerReporter(&reporter);

		dispatcher->killDispatcher();

		ASSERT_EQUAL(1, reporter.logs().size());
		ASSERT_EQUAL(EVENT_DIE, reporter.logs().back().first);
	}

	void eventsWithoutReportersConcurrent()
	{
		partest::EventDispatcherInterface *dispatcher = m_dispatcher.get();
		partest::counting_semaphore<1> sem(0);
		std::thread dispatcherThread = std::thread([dispatcher, &sem]() {
			dispatcher->dispatchEvents();
		});


		unsigned success = dispatcher->pushEvent(EVENT_ASSERTION, partest::makeEventAssertion(1, 0, partest::AssertionResult()));

		// Even without reporters registered, the event should be processed.
		ASSERT_TRUE(success);
		
		// Give it time to throw away the event deliberately.
		std::this_thread::sleep_for(std::chrono::milliseconds(500));

		MockReporter &reporter = m_reporters.front();
		dispatcher->registerReporter(&reporter);

		// Add another event following registration.
		dispatcher->pushEvent(EVENT_LOG, partest::makeEventLog(2, 0, partest::LogEntry(partest::LogLevel::Info, PARTEST_LOG_TYPE_DEFAULT, "Framework log from test code", 2)));
		dispatcher->killDispatcher();
		dispatcherThread.join();

		ASSERT_EQUAL(2, reporter.logs().size());
		ASSERT_EQUAL(EVENT_LOG, reporter.logs().front().first);
		ASSERT_EQUAL(EVENT_DIE, reporter.logs().back().first);
	}

	void eventsPushedBeforeBeginDispatch()
	{
		partest::EventDispatcherInterface *dispatcher = m_dispatcher.get();
		MockReporter &reporter = m_reporters.front();
		dispatcher->registerReporter(&reporter);
		
		// Iterate over logs, before the dispatcher has been spun up.
		for(unsigned x = 0; x < m_logs.size(); ++x)
		{
			dispatcher->pushEvent(m_logs[x].first,m_logs[x].second->clone());
		}

		std::thread dispatcherThread = std::thread([dispatcher]() { dispatcher->dispatchEvents(); });

		dispatcher->killDispatcher();

		dispatcherThread.join();

		unsigned invalidEvents = 0;
		for(unsigned x = 0; x < m_logs.size(); ++x)
		{
			const partest::EventInterface &lhs = *(m_logs[x].second.get());
			const partest::EventInterface &rhs = *(reporter.logs()[x].second.get());

			if(lhs != rhs)
				++invalidEvents;
		}
		// All events should be identical across every firstReporter and the local reference log
		ASSERT_EQUAL(0, invalidEvents);

		ASSERT_EQUAL(m_logs.size() + 1, reporter.logs().size());
		ASSERT_EQUAL(EVENT_DIE, reporter.logs().back().first);
	}
};

#endif