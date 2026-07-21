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
	class EventDispatcherInterface
	{
	protected:
		// Owning queue of events. Each event is a pair of event type string and a pointer to an EventInterface object.
		std::queue<EventPair> m_eventQueue;
		// Non-owning vector of reporter interfaces. These are the subscribers that will receive events from the dispatcher.
		std::vector<EventReporterInterface *> m_reporters;

		std::atomic<bool> m_dispatching; // True until killDispatcher is called

		EventPair popEventUnsafe() noexcept
		{
			EventPair eventPair = std::move(m_eventQueue.front());
			m_eventQueue.pop();
			return eventPair;
		}

	public:
		EventDispatcherInterface(bool dispatching = true) : m_dispatching(dispatching) {}
		virtual ~EventDispatcherInterface() = default;

		// Delete copy and move constructors and assignment operators
		EventDispatcherInterface(const EventDispatcherInterface&) = delete;
		EventDispatcherInterface(EventDispatcherInterface&&) = delete;
		EventDispatcherInterface& operator=(const EventDispatcherInterface&) = delete;
		EventDispatcherInterface& operator=(EventDispatcherInterface&&) = delete;

		bool isDispatching() const noexcept
		{
			return m_dispatching;
		}

		virtual void killDispatcher() = 0;

		virtual void registerReporter(EventReporterInterface *reporter) = 0;

		virtual bool pushEvent(const std::string &eventType, std::unique_ptr<EventInterface> event) = 0;

		virtual void dispatchEvents() = 0;
	};

	class SerialEventDispatcher : public EventDispatcherInterface
	{
	public:
		SerialEventDispatcher(bool dispatching = true) : EventDispatcherInterface(dispatching) {}
		~SerialEventDispatcher() override = default;

		void killDispatcher() override
		{
			pushEvent(EVENT_DIE, std::make_unique<EventDie>());
			m_dispatching = false;
		}

		void registerReporter(EventReporterInterface *reporter) override
		{
			m_reporters.push_back(reporter);
		}

		bool pushEvent(const std::string &eventType, std::unique_ptr<EventInterface> event) override
		{
			if(!isDispatching())
				return false;

			m_eventQueue.emplace(eventType, std::move(event));
			dispatchEvents();

			return true;
		}

		void dispatchEvents() override
		{
			while(!m_eventQueue.empty())
			{
				EventPair eventPair = popEventUnsafe();
				// Get temporary copy of reporters, since a reporter could theoretically be added in response to an event and invalidate our iterator.
				std::vector<EventReporterInterface *> localReporters = m_reporters;

				// Dispatch the event to all registered reporters
				for(EventReporterInterface *reporter : localReporters)
				{
					reporter->reportEvent(eventPair);				
				}
			}
		}
	};

	class ConcurrentEventDispatcher : public EventDispatcherInterface
	{
		std::mutex m_queueMutex;
		std::mutex m_reportersMutex;
		counting_semaphore<> m_eventSemaphore;

	public:
		ConcurrentEventDispatcher(bool dispatching = true) : EventDispatcherInterface(dispatching) {}
		// Delete copy and move constructors and assignment operators
		ConcurrentEventDispatcher(const ConcurrentEventDispatcher&) = delete;
		ConcurrentEventDispatcher(ConcurrentEventDispatcher&&) = delete;
		ConcurrentEventDispatcher& operator=(const ConcurrentEventDispatcher&) = delete;
		ConcurrentEventDispatcher& operator=(ConcurrentEventDispatcher&&) = delete;
		~ConcurrentEventDispatcher() override = default;

		void killDispatcher() override
		{
			std::lock_guard<std::mutex> lock(m_queueMutex);
			m_dispatching = false; // Stop accepting new events
			m_eventQueue.emplace(EVENT_DIE, std::make_unique<EventDie>()); // Push an EventDie to signal the dispatcher to stop
			m_eventSemaphore.release(); // Release the semaphore to unblock
		}

		void registerReporter(EventReporterInterface *reporter) override
		{
			std::lock_guard<std::mutex> lock(m_reportersMutex);
			m_reporters.push_back(reporter);
		}

		bool pushEvent(const std::string &eventType, std::unique_ptr<EventInterface> event) override
		{
			std::lock_guard<std::mutex> lock(m_queueMutex);
			if(!m_dispatching)
				return false; // If the dispatcher is not dispatching, do not accept new events

			m_eventQueue.emplace(eventType, std::move(event)); // Transfer ownership of the EventInterface pointer to the queue
			m_eventSemaphore.release();

			return true;
		}

		/**
		* Block and process events until an EventDie is received. This function will dispatch events to all registered reporters.
		*/
		void dispatchEvents() override
		{
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