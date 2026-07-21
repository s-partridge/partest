#ifndef PARTEST_REPORTER_H
#define PARTEST_REPORTER_H

#include <unordered_map>
#include <vector>
#include <string>

#include <partest/event.h>

namespace partest
{
	struct TestFrameSnapshot
	{
		unsigned parentId = 0;
		unsigned depth = 0;

		std::string name = "";

		std::vector<EventPair> events;
	};

	class EventReporterInterface
	{
		public:
		EventReporterInterface() = default;
		virtual ~EventReporterInterface() = default;

		// Dispatch an event to the appropriate handler based on its type. This function is called by the EventDispatcher when an event is popped from the queue.
		void reportEvent(const EventPair &eventPair)
		{
			const std::string &eventType = eventPair.first;
			const std::unique_ptr<EventInterface> &event = eventPair.second;
			if(eventType == EVENT_BEGIN_TEST)
			{
				onTestBegin(static_cast<const EventBeginTest &>(*event));
			}
			else if(eventType == EVENT_END_TEST)
			{
				onTestEnd(static_cast<const EventEndTest &>(*event));
			}
			else if(eventType == EVENT_ASSERTION)
			{
				onAssertion(static_cast<const EventAssertion &>(*event));
			}
			else if(eventType == EVENT_LOG)
			{
				onLog(static_cast<const EventLog &>(*event));
			}
			else if(eventType == EVENT_PASSTHROUGH)
			{
				onPassthrough(static_cast<const EventPassthrough &>(*event));
			}
			else if(eventType == EVENT_DIE)
			{
				onDie(static_cast<const EventDie &>(*event));
			}
		}

		// Called when a test begins
		virtual void onTestBegin(const EventBeginTest &event) = 0;
		// Called when a test ends
		virtual void onTestEnd(const EventEndTest &event) = 0;
		// Called when an assertion is made
		virtual void onAssertion(const EventAssertion &event) = 0;
		// Called when a log entry is made
		virtual void onLog(const EventLog &event) = 0;
		// Called when a passthrough event is received
		virtual void onPassthrough(const EventPassthrough &event) = 0;
		// Called when a die event is received
		virtual void onDie(const EventDie &event) = 0;
	};
}

#endif