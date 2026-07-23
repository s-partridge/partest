#ifndef PARTEST_EVENT_EMITTER_INTERFACE_H
#define PARTEST_EVENT_EMITTER_INTERFACE_H

#include <memory>
#include <thread>

#include <partest/common.h>

namespace partest
{
	class EventDispatcherInterface;
	class AssertionResultView;
	class TestFrameView;
	struct LogEntry;
	class Event;

	struct EmitterConfig
	{
		EventDispatcherInterface *dispatcher;
	};

	class EventEmitterInterface
	{
	protected:
		// Non-owning pointer to the event dispatcher. This is used to push events to the dispatcher.
		EventDispatcherInterface *m_dispatcher = nullptr;

		virtual bool shouldEmit() const noexcept = 0;
		virtual bool emitEvent(std::unique_ptr<Event> event) = 0;

	public:
		explicit EventEmitterInterface(EventDispatcherInterface *dispatcher = nullptr) : m_dispatcher(dispatcher) {}
		virtual ~EventEmitterInterface() = default;

		void setConfiguration(const EmitterConfig &emitterConfig)
		{
			m_dispatcher = emitterConfig.dispatcher;
		}
		
		virtual bool emitBeginTest(TestFrameView testFrame) = 0;
		virtual bool emitEndTest(TestFrameView testFrame) = 0;
		virtual bool emitAssertion(TestFrameView testFrame, AssertionResultView assertionResult) = 0;
		virtual bool emitLog(TestFrameView testFrame, const LogEntry &logEntry) = 0;
		virtual bool emitPassthrough(TestFrameView testFrame, std::thread::id threadId, PARTEST_STRING_PARAM message) = 0;
	};
}

#endif