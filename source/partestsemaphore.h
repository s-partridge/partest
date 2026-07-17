#ifndef PARTEST_SEMAPHORE_H
#define PARTEST_SEMAPHORE_H

#include <mutex>
#include <condition_variable>
#include "partestcommon.h"

namespace partest
{
	class Semaphore
	{
		std::mutex m_mutex;
		std::condition_variable m_cv;
		unsigned m_count;

	public:
		Semaphore(unsigned count = 0) : m_count(count) {}
		~Semaphore() = default;

		// Remove copy construtors and assignment operators
		Semaphore(const Semaphore &) = delete;
		Semaphore &operator=(const Semaphore &) = delete;
		Semaphore(Semaphore &&) = delete;
		Semaphore &operator=(Semaphore &&) = delete;

		void acquire() {}
		void release() {}
	};
}

#endif