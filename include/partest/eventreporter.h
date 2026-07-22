#ifndef PARTEST_REPORTER_H
#define PARTEST_REPORTER_H

#include <unordered_map>
#include <vector>
#include <string>

#include <partest/event.h>

namespace partest
{
	class EventReporterInterface
	{
		public:
		EventReporterInterface() = default;
		virtual ~EventReporterInterface() = default;

		// Dispatch an event to the appropriate handler based on its type. This function is called by the EventDispatcher when an event is popped from the queue.
		void reportEvent(const Event &event)
		{
			switch(event.getEventType())
			{
			case EventType::BeginTest:
				onTestBegin(event, static_cast<const BeginTestPayload &>(event.getPayload()));
				break;
			
			case EventType::EndTest:
				onTestEnd(event, static_cast<const EndTestPayload &>(event.getPayload()));
				break;
			
			case EventType::Assertion:
				onAssertion(event, static_cast<const AssertionPayload &>(event.getPayload()));
				break;
			
			case EventType::Log:
				onLog(event, static_cast<const LogPayload &>(event.getPayload()));
				break;
			
			case EventType::Passthrough:
				onPassthrough(event, static_cast<const PassthroughPayload &>(event.getPayload()));
				break;
			
			case EventType::Die:
				onDie(event, static_cast<const DiePayload &>(event.getPayload()));
				break;
			}
		}

		// Called when a test begins
		virtual void onTestBegin(const Event &event, const BeginTestPayload &payload) = 0;
		// Called when a test ends
		virtual void onTestEnd(const Event &event, const EndTestPayload &payload) = 0;
		// Called when an assertion is made
		virtual void onAssertion(const Event &event, const AssertionPayload &payload) = 0;
		// Called when a log entry is made
		virtual void onLog(const Event &event, const LogPayload &payload) = 0;
		// Called when a passthrough event is received
		virtual void onPassthrough(const Event &event, const PassthroughPayload &payload) = 0;
		// Called when a die event is received
		virtual void onDie(const Event &event, const DiePayload &payload) = 0;
	};
}

#endif