#ifndef PARTEST_EVENT_H
#define PARTEST_EVENT_H

#include <chrono>
#include <thread>
#include "partesttypes.h"
#include "partestassertresult.h"
#include "partestlog.h"

PARTEST_CONSTEXPR_11 const char *EVENT_BEGIN_TEST = "EB";
PARTEST_CONSTEXPR_11 const char *EVENT_END_TEST = "EE";
PARTEST_CONSTEXPR_11 const char *EVENT_ASSERTION = "EA";
PARTEST_CONSTEXPR_11 const char *EVENT_LOG = "EL";
PARTEST_CONSTEXPR_11 const char *EVENT_PASSTHROUGH = "EP";
PARTEST_CONSTEXPR_11 const char *EVENT_DIE = "ED";

namespace partest
{
	/**
	* @class EventInterface
	* @brief Base class for all test events
	* @member m_testId The unique ID of the test associated with this event.
	* @member m_parentTestId The unique ID of the parent test, if any.
	* @member m_timestamp The time when the event was created.
	*/
	struct EventInterface
	{
		unsigned m_testId;
		unsigned m_parentTestId;
		std::chrono::steady_clock::time_point m_timestamp;

	public:
		EventInterface(unsigned testId, unsigned parentTestId)
			: m_testId(testId), m_parentTestId(parentTestId), m_timestamp(std::chrono::steady_clock::now()) {}

		virtual ~EventInterface() = default;
		virtual std::unique_ptr<EventInterface> clone() const = 0;

		inline unsigned getTestId() const noexcept { return m_testId; }
		inline unsigned getParentTestId() const noexcept { return m_parentTestId; }
		inline std::chrono::steady_clock::time_point getTimestamp() const noexcept { return m_timestamp; }
	};

	// Type alias for an event pair, which consists of an event type string and a unique pointer to an EventInterface object. This allows for easy management of events in the dispatcher.
	using EventPair = std::pair<std::string, std::unique_ptr<EventInterface>>;

	/**
	* @class EventBeginTest
	* @brief Event type for when a test begins
	* @member m_testName The name of the test that is starting
	*/
	struct EventBeginTest : public EventInterface
	{
		std::string m_testName;

	public:
		EventBeginTest(unsigned testId, unsigned parentTestId, PARTEST_STRING_PARAM testName)
			: EventInterface(testId, parentTestId), m_testName(testName) {}

		std::unique_ptr<EventInterface> clone() const override {
			return std::make_unique<EventBeginTest>(*this);
		}

		inline const std::string &getTestName() const noexcept { return m_testName; }
	};

	/**
	* @class EventEndTest
	* @brief Event type for when a test ends
	* @member m_testName The name of the test that is ending
	* @member m_result The result of the test
	*/
	struct EventEndTest : public EventInterface
	{
		std::string m_testName;
		TestResult m_result;

	public:
		EventEndTest(unsigned testId, unsigned parentTestId, PARTEST_STRING_PARAM testName, TestResult result)
			: EventInterface(testId, parentTestId), m_testName(testName), m_result(result) {}

		std::unique_ptr<EventInterface> clone() const override {
			return std::make_unique<EventEndTest>(*this);
		}

		inline const std::string &getTestName() const noexcept { return m_testName; }
		inline TestResult getResult() const noexcept { return m_result; }
	};

	/**
	* @class EventAssertion
	* @brief Event type for when an assertion is made
	* @member m_assertionResult The result of the assertion
	*/
	struct EventAssertion : public EventInterface
	{
		AssertionResult m_assertionResult;

	public:
		EventAssertion(unsigned testId, unsigned parentTestId, const AssertionResult &assertionResult)
			: EventInterface(testId, parentTestId), m_assertionResult(assertionResult) {}

		std::unique_ptr<EventInterface> clone() const override {
			return std::make_unique<EventAssertion>(*this);
		}

		inline const AssertionResult &getAssertionResult() const noexcept { return m_assertionResult; }
	};

	/**
	* @class EventLog
	* @brief Event type for logging messages
	* @member m_logEntry The log entry to be recorded
	*/
	struct EventLog : public EventInterface
	{
		LogEntry m_logEntry;

	public:
		EventLog(unsigned testId, unsigned parentTestId, const LogEntry &logEntry)
			: EventInterface(testId, parentTestId), m_logEntry(logEntry) {}

		std::unique_ptr<EventInterface> clone() const override {
			return std::make_unique<EventLog>(*this);
		}

		inline const LogEntry &getLogEntry() const noexcept { return m_logEntry; }
	};

	/**
	* @class EventPassthrough
	* @brief Event type for console output routed from user code through the test framework
	* @member m_message The message to be output
	* @member m_threadId The ID of the thread that generated the output
	*/
	struct EventPassthrough : public EventInterface
	{
		std::string m_message;
		std::thread::id m_threadId;

	public:
		EventPassthrough(unsigned testId, unsigned parentTestId, PARTEST_STRING_PARAM message)
			: EventInterface(testId, parentTestId), m_message(message), m_threadId(std::this_thread::get_id()) {}

		std::unique_ptr<EventInterface> clone() const override {
			return std::make_unique<EventPassthrough>(*this);
		}

		inline const std::string &getMessage() const noexcept { return m_message; }
		inline std::thread::id getThreadId() const noexcept { return m_threadId; }
	};

	/**
	* @class EventDie
	* @brief Special event to kill the dispatcher thread and stop the reporting system.
	*        This is used internally by the framework to signal that no more events will be generated.
	*/
	struct EventDie : public EventInterface
	{
		EventDie()
			: EventInterface(0, 0) {}

		std::unique_ptr<EventInterface> clone() const override {
			return std::make_unique<EventDie>(*this);
		}
	};

	std::unique_ptr<EventInterface> makeEventBeginTest(unsigned testId, unsigned parentTestId, PARTEST_STRING_PARAM testName)
	{
		return partest::make_unique<EventBeginTest>(testId, parentTestId, testName);
	}

	std::unique_ptr<EventInterface> makeEventEndTest(unsigned testId, unsigned parentTestId, PARTEST_STRING_PARAM testName, TestResult result)
	{
		return partest::make_unique<EventEndTest>(testId, parentTestId, testName, result);
	}

	std::unique_ptr<EventInterface> makeEventAssertion(unsigned testId, unsigned parentTestId, const AssertionResult &result)
	{
		return partest::make_unique<EventAssertion>(testId, parentTestId, result);
	}

	std::unique_ptr<EventInterface> makeEventLog(unsigned testId, unsigned parentTestId, const LogEntry &log)
	{
		return partest::make_unique<EventLog>(testId, parentTestId, log);
	}

	std::unique_ptr<EventInterface> makeEventPassthrough(unsigned testId, unsigned parentTestId, PARTEST_STRING_PARAM message)
	{
		return partest::make_unique<EventPassthrough>(testId, parentTestId, message);
	}

	std::unique_ptr<EventInterface> makeEventDie()
	{
		return partest::make_unique<EventDie>();
	}
}
#endif