#ifndef PARTEST_EVENT_EMITTER_H
#define PARTEST_EVENT_EMITTER_H

#include <partest/event.h>
#include <partest/eventdispatcher.h>

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
		bool emitEvent(std::unique_ptr<Event> event)
		{
			if (shouldEmit())
			{
				return m_dispatcher->pushEvent(std::move(event));
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

		bool emitBeginTest(TestFrameView testFrame)
		{
			return emitEvent(makeEventBeginTest(testFrame));
		}

		bool emitEndTest(TestFrameView testFrame)
		{
			return emitEvent(makeEventEndTest(testFrame));
		}

		bool emitAssertion(TestFrameView testFrame, AssertionResultView assertionResult)
		{
			return emitEvent(makeEventAssertion(testFrame, assertionResult));
		}

		bool emitLog(TestFrameView testFrame, const LogEntry &logEntry)
		{
			return emitEvent(makeEventLog(testFrame, logEntry));
		}

		bool emitPassthrough(TestFrameView testFrame, std::thread::id threadId, PARTEST_STRING_PARAM message)
		{
			return emitEvent(makeEventPassthrough(testFrame, threadId, message));
		}
	};
}

#endif