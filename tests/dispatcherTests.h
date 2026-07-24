#ifndef DISPATCHER_TESTS_H
#define DISPATCHER_TESTS_H

#include <thread>
#include <memory>
#include <algorithm>

#include <partest/eventdispatcher.h>
#include <partest/eventreporter.h>
#include <partest/testbase.h>

class MockReporter : public partest::EventReporterInterface
{
	std::vector<std::unique_ptr<partest::Event>> m_logs;

	void writeLog(std::unique_ptr<partest::Event> event)
	{
		m_logs.emplace_back(std::move(event));
	}

public:
	MockReporter() = default;
    MockReporter(const MockReporter&) = delete;
    MockReporter& operator=(const MockReporter&) = delete;
    MockReporter(MockReporter&&) = default;
    MockReporter& operator=(MockReporter&&) = default;

	void onTestBegin(const partest::Event &event, const partest::BeginTestPayload &payload) override
	{
		writeLog(event.clone());
	}

	void onTestEnd(const partest::Event &event, const partest::EndTestPayload &payload) override
	{
		writeLog(event.clone());
	}

	void onAssertion(const partest::Event &event, const partest::AssertionPayload &payload) override
	{
		writeLog(event.clone());
	}

	void onLog(const partest::Event &event, const partest::LogPayload &payload) override
	{
		writeLog(event.clone());
	}

	void onPassthrough(const partest::Event &event, const partest::PassthroughPayload &payload) override
	{
		writeLog(event.clone());
	}

	void onDie(const partest::Event &event, const partest::DiePayload &payload) override
	{
		writeLog(event.clone());
	}

	const std::vector<std::unique_ptr<partest::Event>> &logs() const { return m_logs; }
	void clearLogs() { m_logs.clear(); }
};

class DispatcherTests : public partest::TestBase
{
	std::unique_ptr<partest::EventDispatcherInterface> m_dispatcher;
	std::vector<MockReporter> m_reporters;
	std::vector<std::unique_ptr<partest::Event>> m_logs;

	void mirrorLog(std::unique_ptr<partest::Event> event)
	{
		m_logs.emplace_back(std::move(event));
	}

	// Setup to initialize the dispatcher
	void setUpSerial(unsigned reporterCount)
	{
		m_dispatcher = partest::make_unique<partest::SerialEventDispatcher>();
		initializeReporting(reporterCount);

		// Initialize a series of events to pass to the logger.
		partest::TestFrameView nullTestFrame = partest::TestFrameView::getNullTestFrameView();
		// Begin test
		mirrorLog(partest::makeEventBeginTest(nullTestFrame));
		// Assert fail
		mirrorLog(partest::makeEventAssertion(nullTestFrame, partest::AssertionResult()));
		// End test
		mirrorLog(partest::makeEventEndTest(nullTestFrame));
		// Passthrough log
		mirrorLog(partest::makeEventPassthrough(nullTestFrame, std::this_thread::get_id(), "Log from user code"));
		// Normal log
		mirrorLog(partest::makeEventLog(nullTestFrame, partest::LogEntry(partest::LogLevel::Info, partest::LOG_TYPE_DEFAULT, "Framework log from test code")));
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
		partest::TestFrameView nullTestFrame = partest::TestFrameView::getNullTestFrameView();

		// Initialize a series of events to pass to the logger.
		for(unsigned x = 0; x < threadCount; ++x)
		{
			// Begin test
			mirrorLog(partest::makeEventBeginTest(nullTestFrame));
			// Assert fail
			mirrorLog(partest::makeEventAssertion(nullTestFrame, partest::AssertionResult()));
			// End test
			mirrorLog(partest::makeEventEndTest(nullTestFrame));
			// Passthrough log
			mirrorLog(partest::makeEventPassthrough(nullTestFrame, std::this_thread::get_id(), "Log from user code"));
			// Normal log
			mirrorLog(partest::makeEventLog(nullTestFrame, partest::LogEntry(partest::LogLevel::Info, partest::LOG_TYPE_DEFAULT, "Framework log from test code")));
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
	DispatcherTests() : TestBase("DispatcherTests", "Tests for the EventDispatcher classes.")
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
		// Initialize a series of events to pass to the logger.
		partest::TestFrameView nullTestFrame = partest::TestFrameView::getNullTestFrameView();
		
		for(MockReporter &reporter: m_reporters)
			dispatcher->registerReporter(&reporter);

		// Iterate over logs
		for(unsigned x = 0; x < m_logs.size(); ++x)
		{
			dispatcher->pushEvent(m_logs[x]->clone());
		}

		dispatcher->killDispatcher();

		bool success = dispatcher->pushEvent(makeEventAssertion(nullTestFrame, partest::AssertionResult()));
		// A dead dispatcher should refuse any new events.
		ASSERT_FALSE(success);

		unsigned invalidEvents = 0;
		for(unsigned x = 0; x < m_logs.size(); ++x)
		{
			const partest::Event &lhs = *m_logs[x];

			for(MockReporter &reporter: m_reporters)
			{
				const partest::Event &rhs = *reporter.logs()[x];

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
			ASSERT_EQUAL(partest::EventType::Die, reporter.logs().back()->getEventType());
		}
	}

	void concurrentDispatcherPassesAllEvents(unsigned threadCount)
	{
		partest::EventDispatcherInterface *dispatcher = m_dispatcher.get();
		// Initialize a series of events to pass to the logger.
		partest::TestFrameView nullTestFrame = partest::TestFrameView::getNullTestFrameView();
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
					dispatcher->pushEvent(m_logs[x]->clone());
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
		bool success = dispatcher->pushEvent(partest::makeEventAssertion(nullTestFrame, partest::AssertionResult()));

		ASSERT_FALSE(success);

		dispatcherThread.join();

		success = dispatcher->pushEvent(partest::makeEventAssertion(nullTestFrame, partest::AssertionResult()));
		// Ensure no events propagate after join.
		// This should be impossible because no thread is running the dispatchEvents() call.
		ASSERT_FALSE(success);

		unsigned uncopiedEvents = 0;
		// Check that all the events from m_logs were propagated to one reporter
		const MockReporter &firstReporter = m_reporters.front();
		for(const std::unique_ptr<partest::Event> &lhs: m_logs)
		{
			std::vector<std::unique_ptr<partest::Event>>::const_iterator iter = std::find_if(
				firstReporter.logs().cbegin(),
				firstReporter.logs().cend(),
				[&](const std::unique_ptr<partest::Event> &rhs) {
					return *lhs == *rhs;
				});
			if(iter == firstReporter.logs().cend())
				++uncopiedEvents;
		}

		ASSERT_EQUAL(0, uncopiedEvents);
		// Last event should always be the kill event
		ASSERT_EQUAL(partest::EventType::Die, firstReporter.logs().back()->getEventType());

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
			const partest::Event &lhs = *firstReporter.logs()[logIdx];

			for(unsigned reporterIdx = 1; reporterIdx < m_reporters.size(); ++reporterIdx)
			{
				const partest::Event &rhs = *m_reporters[reporterIdx].logs()[logIdx];

				if(lhs != rhs)
					++invalidEvents;
			}
		}
		ASSERT_EQUAL(0, invalidEvents);
	}

	void eventsWithoutReportersSerial()
	{
		partest::EventDispatcherInterface *dispatcher = m_dispatcher.get();
		// Initialize a series of events to pass to the logger.
		partest::TestFrameView nullTestFrame = partest::TestFrameView::getNullTestFrameView();
		unsigned success = dispatcher->pushEvent(partest::makeEventAssertion(nullTestFrame, partest::AssertionResult()));

		// Event without reporters registered, the event should be processed.
		ASSERT_TRUE(success);

		MockReporter &reporter = m_reporters.front();
		dispatcher->registerReporter(&reporter);

		dispatcher->killDispatcher();

		ASSERT_EQUAL(1, reporter.logs().size());
		ASSERT_EQUAL(partest::EventType::Die, reporter.logs().back()->getEventType());
	}

	void eventsWithoutReportersConcurrent()
	{
		partest::EventDispatcherInterface *dispatcher = m_dispatcher.get();
		// Initialize a series of events to pass to the logger.
		partest::TestFrameView nullTestFrame = partest::TestFrameView::getNullTestFrameView();
		
		partest::counting_semaphore<1> sem(0);
		std::thread dispatcherThread = std::thread([dispatcher, &sem]() {
			dispatcher->dispatchEvents();
		});


		unsigned success = dispatcher->pushEvent(partest::makeEventAssertion(nullTestFrame, partest::AssertionResult()));

		// Even without reporters registered, the event should be processed.
		ASSERT_TRUE(success);
		
		// Give it time to throw away the event deliberately.
		std::this_thread::sleep_for(std::chrono::milliseconds(500));

		MockReporter &reporter = m_reporters.front();
		dispatcher->registerReporter(&reporter);

		// Add another event following registration.
		dispatcher->pushEvent(partest::makeEventLog(nullTestFrame, partest::LogEntry(partest::LogLevel::Info, partest::LOG_TYPE_DEFAULT, "Framework log from test code")));
		dispatcher->killDispatcher();
		dispatcherThread.join();

		ASSERT_EQUAL(2, reporter.logs().size());
		ASSERT_EQUAL(partest::EventType::Log, reporter.logs().front()->getEventType());
		ASSERT_EQUAL(partest::EventType::Die, reporter.logs().back()->getEventType());
	}

	void eventsPushedBeforeBeginDispatch()
	{
		partest::EventDispatcherInterface *dispatcher = m_dispatcher.get();
		MockReporter &reporter = m_reporters.front();
		dispatcher->registerReporter(&reporter);
		
		// Iterate over logs, before the dispatcher has been spun up.
		for(unsigned x = 0; x < m_logs.size(); ++x)
		{
			dispatcher->pushEvent(m_logs[x]->clone());
		}

		std::thread dispatcherThread = std::thread([dispatcher]() { dispatcher->dispatchEvents(); });

		dispatcher->killDispatcher();

		dispatcherThread.join();

		unsigned invalidEvents = 0;
		for(unsigned x = 0; x < m_logs.size(); ++x)
		{
			const partest::Event &lhs = *m_logs[x];
			const partest::Event &rhs = *reporter.logs()[x];

			if(lhs != rhs)
				++invalidEvents;
		}
		// All events should be identical across every firstReporter and the local reference log
		ASSERT_EQUAL(0, invalidEvents);

		ASSERT_EQUAL(m_logs.size() + 1, reporter.logs().size());
		ASSERT_EQUAL(partest::EventType::Die, reporter.logs().back()->getEventType());
	}
};

#endif