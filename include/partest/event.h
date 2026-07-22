#ifndef PARTEST_EVENT_H
#define PARTEST_EVENT_H

#include <chrono>
#include <thread>
#include <partest/log.h>
#include <partest/types.h>
#include <partest/testframe.h>
#include <partest/assertresult.h>

namespace partest
{
	enum class EventType : uint8_t
	{
		BeginTest = 0,
		EndTest,
		Assertion,
		Log,
		Passthrough,
		Die
	};

	struct EventPayload
	{
		EventPayload() = default;
		virtual ~EventPayload() = default;

		TestFrameView testFrame;
	};

	// For assertions
	struct AssertionPayload : public EventPayload
	{
		AssertionResultView assertionResult;
	};

	struct LogPayload : public EventPayload
	{
		// Passed by value in case a transient log goes out of scope
		LogEntry logEntry;
	};

	struct PassthroughPayload : public EventPayload
	{
		// Passed by value in case a transient message goes out of scope
		std::string message;
		std::thread::id threadId;
	};

	// Aliases for events that currently have no extra members
	// Future-proofed, in case requirements change
	using BeginTestPayload = EventPayload;
	using EndTestPayload = EventPayload;
	using DiePayload = EventPayload;

	/**
	* @class Event
	* @brief Container class for all test events
	* @member eventId The unique ID of this event
	* @member m_timestamp The time the event was created
	* @member m_eventType Type of event the object contains
	* @member m_payload The unique members for the given event type
	*/
	struct Event
	{
		unsigned m_eventId;
		std::chrono::steady_clock::time_point m_timestamp;
		EventType m_eventType;
		std::unique_ptr<EventPayload> m_payload;

		static unsigned int nextID() noexcept {
		
			static std::atomic<unsigned> eventCount(0);
			return eventCount.fetch_add(1, std::memory_order_relaxed);
		}

	public:
		Event(EventType eventType, std::unique_ptr<EventPayload> payload)
			: m_eventId(nextID()), m_timestamp(std::chrono::steady_clock::now()), m_eventType(eventType), m_payload(std::move(payload)) {}

		std::unique_ptr<Event> clone() const 
		{
			return partest::make_unique<Event>(*this);
		}


		inline unsigned getEventId() const noexcept { return m_eventId; }
		EventType getEventType() const noexcept { return m_eventType; }
		const EventPayload &getPayload() const noexcept { return *m_payload; }
		inline std::chrono::steady_clock::time_point getTimestamp() const noexcept { return m_timestamp; }

		bool operator==(const Event &rhs) const noexcept
		{
			return m_eventId == rhs.m_eventId;
		}

		bool operator!=(const Event &rhs) const noexcept { return !(*this == rhs); }
	};

	std::unique_ptr<Event> makeEventBeginTest(TestFrameView testFrame)
	{
		std::unique_ptr<BeginTestPayload> payload = partest::make_unique<BeginTestPayload>();
		payload->testFrame = testFrame;

		return partest::make_unique<Event>(EventType::BeginTest, payload);
	}

	std::unique_ptr<Event> makeEventEndTest(TestFrameView testFrame)
	{
		std::unique_ptr<EndTestPayload> payload = partest::make_unique<EndTestPayload>();
		payload->testFrame = testFrame;
		return partest::make_unique<Event>(EventType::EndTest, payload);
	}

	std::unique_ptr<Event> makeEventAssertion(TestFrameView testFrame, AssertionResultView assertionResult)
	{
		std::unique_ptr<AssertionPayload> payload = partest::make_unique<AssertionPayload>();
		payload->testFrame = testFrame;
		payload->assertionResult = assertionResult;
		return partest::make_unique<Event>(EventType::Assertion, payload);
	}

	std::unique_ptr<Event> makeEventLog(TestFrameView testFrame, const LogEntry &logEntry)
	{
		std::unique_ptr<LogPayload> payload = partest::make_unique<LogPayload>();
		payload->testFrame = testFrame;
		payload->logEntry = logEntry;
		return partest::make_unique<Event>(EventType::Log, payload);
	}

	std::unique_ptr<Event> makeEventPassthrough(TestFrameView testFrame, std::thread::id threadId, PARTEST_STRING_PARAM message)
	{
		std::unique_ptr<PassthroughPayload> payload = partest::make_unique<PassthroughPayload>();
		payload->testFrame = testFrame;
		payload->message = message;
		payload->threadId = threadId;
		return partest::make_unique<Event>(EventType::Passthrough, payload);
	}

	std::unique_ptr<Event> makeEventDie()
	{
		std::unique_ptr<DiePayload> payload = partest::make_unique<DiePayload>();
		payload->testFrame = TestFrameView::getNullTestFrameView();
		return partest::make_unique<Event>(EventType::Die, payload);
	}
}
#endif