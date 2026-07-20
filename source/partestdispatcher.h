#ifndef PARTEST_DISPATCHER_H
#define PARTEST_DISPATCHER_H

#include <string>
#include <queue>
#include <utility>
#include <vector>
#include <mutex>
#include <cassert>

#include "partesttypes.h"
#include "partestevent.h"
#include "partestreporter.h"
#include "partestsemaphore.h"

namespace partest
{
	class EventDispatcher {
		// Owning queue of events. Each event is a pair of event type string and a pointer to an EventInterface object.
		std::queue<EventPair> m_eventQueue;
		// Non-owning vector of reporter interfaces. These are the subscribers that will receive events from the dispatcher.
		std::vector<EventReporterInterface *> m_reporters;
		std::mutex m_queueMutex;
		std::mutex m_reportersMutex;
		counting_semaphore<> m_eventSemaphore;

		std::atomic<bool>m_dispatching; // Flag to indicate whether the dispatcher is currently dispatching events

	public:
		EventDispatcher() : m_dispatching(true) {};
		// Delete copy and move constructors and assignment operators
		EventDispatcher(const EventDispatcher&) = delete;
		EventDispatcher(EventDispatcher&&) = delete;
		EventDispatcher& operator=(const EventDispatcher&) = delete;
		EventDispatcher& operator=(EventDispatcher&&) = delete;
		~EventDispatcher() = default;

		bool pushEvent(const std::string &eventType, std::unique_ptr<EventInterface> event)
		{
			std::lock_guard<std::mutex> lock(m_queueMutex);
			if(!m_dispatching)
				return false; // If the dispatcher is not dispatching, do not accept new events

			m_eventQueue.emplace(eventType, std::move(event)); // Transfer ownership of the EventInterface pointer to the queue
			m_eventSemaphore.release();

			return true;
		}

		bool isDispatching() const noexcept
		{
			return m_dispatching;
		}

		EventPair popEventUnsafe() noexcept
		{
			EventPair eventPair = std::move(m_eventQueue.front());
			m_eventQueue.pop();
			return eventPair;
		}

		void registerReporter(EventReporterInterface *reporter)
		{
			std::lock_guard<std::mutex> lock(m_reportersMutex);
			m_reporters.push_back(reporter);
		}

		void killDispatcher()
		{
			std::lock_guard<std::mutex> lock(m_queueMutex);
			m_dispatching = false; // Stop accepting new events
			m_eventQueue.emplace(EVENT_DIE, std::make_unique<EventDie>(0, 0)); // Push an EventDie to signal the dispatcher to stop
			m_eventSemaphore.release(); // Release the semaphore to unblock
		}

		/**
		* Block and process events until an EventDie is received. This function will dispatch events to all registered reporters.
		*/
		void dispatchEvents()
		{
			// DO ***NOT*** use AUTO for any variable
			// Block until the queue is not empty. Use semaphore to wait for events to be pushed.
			while(true)
			{
				m_eventSemaphore.acquire();

				m_queueMutex.lock();
				assert(!m_eventQueue.empty() && "Invariant violated: Event queue is empty when it should not be.");
				EventPair eventPair = popEventUnsafe();
				m_queueMutex.unlock();

				m_reportersMutex.lock();
				std::vector<EventReporterInterface *> localReporters = m_reporters; // Make a copy of the reporters vector to avoid holding the lock while dispatching
				m_reportersMutex.unlock();

				// Dispatch the event to all registered reporters
				for(EventReporterInterface *reporter : localReporters)
				{
					reporter->reportEvent(eventPair);				
				}

				// If the event is an EventDie, break the loop and stop dispatching
				if(eventPair.first == EVENT_DIE)
				{
					break;
				}
			}
		}		
	};
}

#endif