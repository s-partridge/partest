#ifndef PARTEST_FRAMEWORK_CONTEXT_H
#define PARTEST_FRAMEWORK_CONTEXT_H

#include <iostream>
#include <mutex>
#include <condition_variable>

#include <partest/common.h>
#include <partest/eventemitterinterface.h>

namespace partest
{
	class TestRunner;

	class FrameworkContext
	{
		static constexpr unsigned badAllocThreshold = 10;

		//static accessor function for an event emitter supplied by the runner at initialization time
		static EventEmitterInterface *&globalEventEmitter() noexcept
		{
			static EventEmitterInterface *emitter = nullptr;
			return emitter;
		}

		static bool &isAlive() noexcept
		{
			static bool alive = false;
			return alive;
		}

		static unsigned &inUseCounter() noexcept
		{
			static unsigned counter = 0;
			return counter;
		}

		static void setGlobalEventEmitter(EventEmitterInterface *emitter) noexcept
		{
			globalEventEmitter() = emitter;
		}

		static std::mutex &aliveMutex()
		{
			// Use placement-new to create permanent mutex in static storage, which does not have its destructor called at program exit.
			// This avoids potential issues with static destruction order.
			alignas(std::mutex) static std::uint8_t mutexStorage[sizeof(std::mutex)];
			static std::mutex *mutexPtr = new (mutexStorage) std::mutex();
			return *mutexPtr;
		}

		static std::condition_variable &aliveCondition()
		{
			alignas(std::condition_variable) static std::uint8_t conditionStorage[sizeof(std::condition_variable)];
			static std::condition_variable *conditionPtr = new (conditionStorage) std::condition_variable();
			return *conditionPtr;
		}

	public:
		class LifetimeGuard
		{
			bool m_alive;
		public:
			LifetimeGuard()
			{
				m_alive = FrameworkContext::requestAccessIfAlive();
			}

			~LifetimeGuard()
			{
				if(m_alive)
					FrameworkContext::releaseAccess();
			}

			bool isAlive() const noexcept
			{
				return m_alive;
			}
		};

		static std::atomic<unsigned> &badAllocCount() noexcept
		{
			static std::atomic<unsigned> count(0);
			return count;
		}

		static bool hasExceededBadAllocThreshold() noexcept
		{
			return badAllocCount().load(std::memory_order_relaxed) >= badAllocThreshold;
		}

		static bool requestAccessIfAlive()
		{
			std::lock_guard<std::mutex> lock(aliveMutex());
			if(isAlive())
			{
				++inUseCounter();
				return true;
			}
			return false;
		}

		static void releaseAccess()
		{
			bool released = false;
			{
				std::lock_guard<std::mutex> lock(aliveMutex());
				if(--inUseCounter() == 0)
					released = true;
			}
			if(released)
				aliveCondition().notify_all();
		}

		static bool writeGlobalLog(LogLevel level, PARTEST_STRING_PARAM type, PARTEST_STRING_PARAM message)
		{
			// Route to the global emitter if the framework is alive. If not, route to stderr instead.
			LifetimeGuard guard;

			if(guard.isAlive()  && globalEventEmitter() != nullptr)
			{
				// TODO: Shore up the fallback path, match it to the abort routing used elsewhere in the framework. Catch exceptions and return them. They also count as FrameworkAllocationFailures, which should bubble back up like anything else.
				if(!globalEventEmitter()->emitLog(nullptr, LogEntry(level, type, message), std::chrono::system_clock::now()))
				{
					std::cerr << "Failed to emit global log [" << maybeStringify(level) << "] [" << type << "]: " << message << std::endl;
					return false;
				}
				return true;
			}
			else
			{
				std::cerr << "Failed to emit global log [" << maybeStringify(level) << "] [" << type << "]: " << message << std::endl;
			}
			return false;
		}

		friend TestRunner;
	};
}

#endif
