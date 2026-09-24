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

		std::unique_ptr<Event> preallocEndTestEvent(TestFrameView testFrame) override
		{
			return makeEventEndTest(testFrame, std::chrono::system_clock::now());
		}

		bool emitBeginTest(TestFrameView testFrame, Timestamp timestamp) override
		{
			return emitEvent(makeEventBeginTest(testFrame, timestamp));
		}

		/**
		* Emit an end test event to the event queue.
		*
		* @param endTestEvent The preallocated end test event to be emitted
		* @return true if the event was emitted successfully, false otherwise
		*/
		bool emitEndTest(std::unique_ptr<Event> endTestEvent, Timestamp timestamp) override
		{
			endTestEvent->setTimestamp(timestamp);
			return emitEvent(std::move(endTestEvent));
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
