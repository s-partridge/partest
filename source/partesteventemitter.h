#ifndef PARTEST_EVENT_EMITTER_H
#define PARTEST_EVENT_EMITTER_H

#include "partestevent.h"
#include "partestdispatcher.h"

namespace partest
{
	class EventEmitter
	{
		// Non-owning pointer to the event dispatcher. This is used to push events to the dispatcher.
		EventDispatcher *m_dispatcher = nullptr;

		bool shouldEmit() const noexcept
		{
			return m_dispatcher != nullptr && m_dispatcher->isDispatching();
		}

		// Emit an event to the event queue. This function is called by the test framework when an event occurs.
		void emitEvent(PARTEST_STRING_PARAM eventType, std::unique_ptr<EventInterface> event)
		{
			if (shouldEmit())
			{
				m_dispatcher->pushEvent(PARTEST_STRING_PARAM_TO_STRING(eventType), std::move(event));
			}
		}

	public:
		explicit EventEmitter(EventDispatcher *dispatcher = nullptr) : m_dispatcher(dispatcher) {}
		~EventEmitter() = default;

		void emitBeginTest(unsigned testId, unsigned parentTestId, PARTEST_STRING_PARAM testName)
		{
			emitEvent(EVENT_BEGIN_TEST, std::make_unique<EventBeginTest>(testId, parentTestId, testName));
		}

		void emitEndTest(unsigned testId, unsigned parentTestId, PARTEST_STRING_PARAM testName, TestResult result)
		{
			emitEvent(EVENT_END_TEST, std::make_unique<EventEndTest>(testId, parentTestId, testName, result));
		}

		void emitAssertion(unsigned testId, unsigned parentTestId, const AssertionResult &assertionResult)
		{
			emitEvent(EVENT_ASSERTION, std::make_unique<EventAssertion>(testId, parentTestId, assertionResult));
		}

		void emitLog(unsigned testId, unsigned parentTestId, const LogEntry &logEntry)
		{
			emitEvent(EVENT_LOG, std::make_unique<EventLog>(testId, parentTestId, logEntry));
		}

		void emitPassthrough(unsigned testId, unsigned parentTestId, PARTEST_STRING_PARAM message)
		{
			emitEvent(EVENT_PASSTHROUGH, std::make_unique<EventPassthrough>(testId, parentTestId, message));
		}
	};
}

#endif