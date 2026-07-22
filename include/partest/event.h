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
		EventPayload(TestFrameView testFrame)
			: testFrame(testFrame) {}
		virtual ~EventPayload() = default;

		virtual std::unique_ptr<EventPayload> clone() const
		{
			return partest::make_unique<EventPayload>(*this);
		}

		TestFrameView testFrame;
	};

	// For assertions
	struct AssertionPayload : public EventPayload
	{
		AssertionPayload(TestFrameView testFrame, AssertionResultView assertionResult)
			: EventPayload(testFrame), assertionResult(assertionResult) {}

		virtual std::unique_ptr<EventPayload> clone() const
		{
			return partest::make_unique<AssertionPayload>(*this);
		}

		AssertionResultView assertionResult;
	};

	struct LogPayload : public EventPayload
	{
		LogPayload(TestFrameView testFrame, const LogEntry &logEntry)
			: EventPayload(testFrame), logEntry(logEntry) { }

		virtual std::unique_ptr<EventPayload> clone() const
		{
			return partest::make_unique<LogPayload>(*this);
		}

		// Passed by value in case a transient log goes out of scope
		LogEntry logEntry;
	};

	struct PassthroughPayload : public EventPayload
	{
		PassthroughPayload(TestFrameView testFrame, std::thread::id threadId, PARTEST_STRING_PARAM message)
			: EventPayload(testFrame), threadId(threadId), message(message) {}

		virtual std::unique_ptr<EventPayload> clone() const
		{
			return partest::make_unique<PassthroughPayload>(*this);
		}

		// Passed by value in case a transient message goes out of scope
		std::thread::id threadId;
		std::string message;
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
		
		EventPayload *m_payload;

		static unsigned int nextID() noexcept {
		
			static std::atomic<unsigned> eventCount(0);
			return eventCount.fetch_add(1, std::memory_order_relaxed);
		}

	public:
		Event(EventType eventType, std::unique_ptr<EventPayload> payload)
			: m_eventId(nextID()), m_timestamp(std::chrono::steady_clock::now()), m_eventType(eventType), m_payload(payload.release()) {}

		// Copy ctors
		Event(const Event &other)
		{
			m_eventId = other.m_eventId;
			m_timestamp = other.m_timestamp;
			m_eventType = other.m_eventType;
			m_payload = other.m_payload->clone().release();
		}

		Event & operator=(const Event &rhs)
		{
			if(this != &rhs)
			{
				m_eventId = rhs.m_eventId;
				m_timestamp = rhs.m_timestamp;
				m_eventType = rhs.m_eventType;

				delete m_payload;
				m_payload = rhs.m_payload->clone().release();
			}

			return *this;
		}

		// Move ctors
		Event(Event &&other) noexcept
		{
			m_eventId = other.m_eventId;
			m_timestamp = other.m_timestamp;
			m_eventType = other.m_eventType;
			m_payload = other.m_payload;
			other.m_payload = nullptr;
		}

		Event &operator=(Event &&rhs) noexcept
		{
			if(this != &rhs)
			{
				m_eventId = rhs.m_eventId;
				m_timestamp = rhs.m_timestamp;
				m_eventType = rhs.m_eventType;

				delete m_payload;
				m_payload = rhs.m_payload;
				rhs.m_payload = nullptr;
			}

			return *this;
		}

		~Event() { delete m_payload; }

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
		std::unique_ptr<BeginTestPayload> payload = partest::make_unique<BeginTestPayload>(testFrame);
		return partest::make_unique<Event>(EventType::BeginTest, std::move(payload));
	}

	std::unique_ptr<Event> makeEventEndTest(TestFrameView testFrame)
	{
		std::unique_ptr<EndTestPayload> payload = partest::make_unique<EndTestPayload>(testFrame);
		return partest::make_unique<Event>(EventType::EndTest, std::move(payload));
	}

	std::unique_ptr<Event> makeEventAssertion(TestFrameView testFrame, AssertionResultView assertionResult)
	{
		std::unique_ptr<AssertionPayload> payload = partest::make_unique<AssertionPayload>(testFrame, assertionResult);
		return partest::make_unique<Event>(EventType::Assertion, std::move(payload));
	}

	std::unique_ptr<Event> makeEventLog(TestFrameView testFrame, const LogEntry &logEntry)
	{
		std::unique_ptr<LogPayload> payload = partest::make_unique<LogPayload>(testFrame, logEntry);
		return partest::make_unique<Event>(EventType::Log, std::move(payload));
	}

	std::unique_ptr<Event> makeEventPassthrough(TestFrameView testFrame, std::thread::id threadId, PARTEST_STRING_PARAM message)
	{
		std::unique_ptr<PassthroughPayload> payload = partest::make_unique<PassthroughPayload>(testFrame, threadId, message);
		return partest::make_unique<Event>(EventType::Passthrough, std::move(payload));
	}

	std::unique_ptr<Event> makeEventDie()
	{
		std::unique_ptr<DiePayload> payload = partest::make_unique<DiePayload>(TestFrameView::getNullTestFrameView());
		return partest::make_unique<Event>(EventType::Die, std::move(payload));
	}
}
#endif