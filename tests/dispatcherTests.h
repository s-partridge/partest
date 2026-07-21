#ifndef DISPATCHER_TESTS_H
#define DISPATCHER_TESTS_H

#include <memory>

#include "partestdispatcher.h"
#include "partestreporter.h"
#include "partestbase.h"

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
		writeLog(EVENT_BEGIN_TEST, std::make_unique<partest::EventBeginTest>(event));
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
	void setUpSerial()
	{
		m_dispatcher = partest::make_unique<partest::SerialEventDispatcher>();
		initializeReporting(2);

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
		// End event
		mirrorLog(EVENT_DIE, partest::makeEventDie());
	}

	void tearDownSerial()
	{
		clearReporting();
	}

	void setUpConcurrent()
	{
		m_dispatcher = partest::make_unique<partest::ConcurrentEventDispatcher>();
		initializeReporting(6);
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

		addTest(partest::TestInfo("Test Serial Dispatcher", "Validate that dispatcher passes all events intact to reporters, in sequential order."),
			flags,
			[this]() { return this->SerialDispatcherPassesAllEvents(); },
			[this]() { return this->setUpSerial(); },
			[this]() { return this->tearDownSerial(); }
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

		// Iterate over logs, skip DIE entry.
		for(unsigned x = 0; x < m_logs.size() - 1; ++x)
		{
			dispatcher->pushEvent(m_logs[x].first,m_logs[x].second->clone());
		}

		dispatcher->killDispatcher();

		for(MockReporter &reporter: m_reporters)
		{
			ASSERT_EQUAL(m_logs.size(), reporter.logs().size());
			ASSERT_TRUE(reporter.logs().back().first == EVENT_DIE);
		}
	}
};

#endif