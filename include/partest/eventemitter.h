#ifndef PARTEST_EVENT_EMITTER_H
#define PARTEST_EVENT_EMITTER_H

#include <partest/eventemitterinterface.h>
#include <partest/event.h>
#include <partest/eventdispatcher.h>

namespace partest
{
	class EventEmitter : public EventEmitterInterface
	{
		bool shouldEmit() const noexcept override
		{
			return m_dispatcher != nullptr && m_dispatcher->isDispatching();
		}

		// Emit an event to the event queue. This function is called by the test framework when an event occurs.
		bool emitEvent(std::unique_ptr<Event> event) override
		{
			if (shouldEmit())
			{
				return m_dispatcher->pushEvent(std::move(event));
			}
			
			return false;
		}

	public:
		explicit EventEmitter(EventDispatcherInterface *dispatcher = nullptr) : EventEmitterInterface(dispatcher) {}
		~EventEmitter() = default;

		bool emitBeginTest(TestFrameView testFrame) override
		{
			return emitEvent(makeEventBeginTest(testFrame));
		}

		bool emitEndTest(TestFrameView testFrame) override
		{
			return emitEvent(makeEventEndTest(testFrame));
		}

		bool emitAssertion(TestFrameView testFrame, AssertionResultView assertionResult) override
		{
			return emitEvent(makeEventAssertion(testFrame, assertionResult));
		}

		bool emitLog(TestFrameView testFrame, const LogEntry &logEntry) override
		{
			return emitEvent(makeEventLog(testFrame, logEntry));
		}

		bool emitPassthrough(TestFrameView testFrame, std::thread::id threadId, PARTEST_STRING_PARAM message) override
		{
			return emitEvent(makeEventPassthrough(testFrame, threadId, message));
		}
	};
}

#endif