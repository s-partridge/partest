#ifndef PARTEST_FRAMEWORK_CONTEXT_H
#define PARTEST_FRAMEWORK_CONTEXT_H

#include <mutex>

#include <partest/eventemitter.h>

namespace partest
{
	class TestRunner;

	class FrameworkContext
	{
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
			// Use placement-new to create permanent mutex in static storage, which does not have its destructor called at program exit. This avoids potential issues with static destruction order.
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

		friend TestRunner;
	};
}

#endif
