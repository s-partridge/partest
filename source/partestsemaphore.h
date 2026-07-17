#ifndef PARTEST_SEMAPHORE_H
#define PARTEST_SEMAPHORE_H

#include <mutex>
#include <condition_variable>
#include <cassert>
#include <stddef.h>
#include "partestcommon.h"

namespace partest
{
	// Simplified implementation of a counting semaphore for C++17 and earlier, since std::counting_semaphore is only available in C++20 and later.
	template <std::ptrdiff_t least_max_value = PTRDIFF_MAX>
	class CountingSemaphore
	{
		std::mutex m_mutex;
		std::condition_variable m_cv;
		std::ptrdiff_t m_count;

	public:
		static_assert(least_max_value >= 0, "The maximum value of the semaphore must be non-negative.");

		explicit CountingSemaphore(std::ptrdiff_t desired = 0) noexcept 
			: m_count(desired)
		{
			// Ensure that the initial count does not exceed the maximum value
			assert(desired <= least_max_value && "Initial count exceeds the maximum value of the semaphore.");
		}
		~CountingSemaphore() = default;

		// Remove copy construtors
		CountingSemaphore(const CountingSemaphore &) = delete;
		CountingSemaphore &operator=(const CountingSemaphore &) = delete;

		// Get the maximum value of the semaphore
		static constexpr std::ptrdiff_t max() noexcept {
			return least_max_value;
		}

		void acquire()
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			m_cv.wait(lock, [this]() { return m_count > 0; });
			--m_count;
		}

		bool try_acquire()
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			if (m_count > 0)
			{
				--m_count;
				return true;
			}
			return false;
		}

		template<class Rep, class Period>
		bool try_acquire_for(const std::chrono::duration<Rep, Period>& rel_time)
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			if (m_cv.wait_for(lock, rel_time, [this]() { return m_count > 0; }))
			{
				--m_count;
				return true;
			}
			return false;
		}

		template<class Clock, class Duration>
		bool try_acquire_until(const std::chrono::time_point<Clock, Duration>& abs_time)
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			if (m_cv.wait_until(lock, abs_time, [this]() { return m_count > 0; }))
			{
				--m_count;
				return true;
			}
			return false;
		}

		void release(std::ptrdiff_t n = 1)
		{
			if(n <= 0)
				return;

			m_mutex.lock();
			assert(m_count + n <= least_max_value && "Releasing would exceed the maximum value of the semaphore.");
			m_count += n;
			m_mutex.unlock();

			if(n == 1)
			{
				m_cv.notify_one();
			}
			else
			{
				m_cv.notify_all();
			}
		}
	};
}

#endif