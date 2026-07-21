#ifndef PARTEST_EVENT_EMITTER_H
#define PARTEST_EVENT_EMITTER_H

#include "partestevent.h"
#include "partestdispatcher.h"

namespace partest
{
	struct EmitterConfig
	{
		EventDispatcherInterface *dispatcher;
	};

	class EventEmitter
	{
		// Non-owning pointer to the event dispatcher. This is used to push events to the dispatcher.
		EventDispatcherInterface *m_dispatcher = nullptr;

		bool shouldEmit() const noexcept
		{
			return m_dispatcher != nullptr && m_dispatcher->isDispatching();
		}

		// Emit an event to the event queue. This function is called by the test framework when an event occurs.
		bool emitEvent(PARTEST_STRING_PARAM eventType, std::unique_ptr<EventInterface> event)
		{
			if (shouldEmit())
			{
				return m_dispatcher->pushEvent(PARTEST_STRING_PARAM_TO_STRING(eventType), std::move(event));
			}
			
			return false;
		}

	public:
		explicit EventEmitter(EventDispatcherInterface *dispatcher = nullptr) : m_dispatcher(dispatcher) {}
		~EventEmitter() = default;

		void setConfiguration(const EmitterConfig &emitterConfig)
		{
			m_dispatcher = emitterConfig.dispatcher;
		}

		bool emitBeginTest(unsigned testId, unsigned parentTestId, PARTEST_STRING_PARAM testName)
		{
			return emitEvent(EVENT_BEGIN_TEST, partest::make_unique<EventBeginTest>(testId, parentTestId, testName));
		}

		bool emitEndTest(unsigned testId, unsigned parentTestId, PARTEST_STRING_PARAM testName, TestResult result)
		{
			return emitEvent(EVENT_END_TEST, partest::make_unique<EventEndTest>(testId, parentTestId, testName, result));
		}

		bool emitAssertion(unsigned testId, unsigned parentTestId, const AssertionResult &assertionResult)
		{
			return emitEvent(EVENT_ASSERTION, partest::make_unique<EventAssertion>(testId, parentTestId, assertionResult));
		}

		bool emitLog(unsigned testId, unsigned parentTestId, const LogEntry &logEntry)
		{
			return emitEvent(EVENT_LOG, partest::make_unique<EventLog>(testId, parentTestId, logEntry));
		}

		bool emitPassthrough(unsigned testId, unsigned parentTestId, PARTEST_STRING_PARAM message)
		{
			return emitEvent(EVENT_PASSTHROUGH, partest::make_unique<EventPassthrough>(testId, parentTestId, message));
		}
	};
}

#endif