#ifndef PARTEST_EVENT_EMITTER_H
#define PARTEST_EVENT_EMITTER_H

#include <partest/eventemitterinterface.h>
#include <partest/event.h>
#include <partest/eventdispatcher.h>

namespace partest
{
	class EventEmitter : public EventEmitterInterface
	{
	protected:
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

		bool emitBeginTest(TestFrameView testFrame, Timestamp timestamp) override
		{
			return emitEvent(makeEventBeginTest(testFrame, timestamp));
		}

		bool emitEndTest(TestFrameView testFrame, Timestamp timestamp) override
		{
			return emitEvent(makeEventEndTest(testFrame, timestamp));
		}

		bool emitAssertion(TestFrameView testFrame, const AssertionResult &assertionResult, Timestamp timestamp) override
		{
			return emitEvent(makeEventAssertion(testFrame, assertionResult, timestamp));
		}

		bool emitLog(TestFrameView testFrame, const LogEntry &logEntry, Timestamp timestamp) override
		{
			return emitEvent(makeEventLog(testFrame, logEntry, timestamp));
		}

		bool emitPassthrough(TestFrameView testFrame, std::thread::id threadId, PARTEST_STRING_PARAM message, Timestamp timestamp) override
		{
			return emitEvent(makeEventPassthrough(testFrame, threadId, message, timestamp));
		}
	};
}

#endif