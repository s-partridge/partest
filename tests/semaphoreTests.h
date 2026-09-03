#ifndef SEMAPHORE_TESTS_H
#define	SEMAPHORE_TESTS_H

#include <atomic>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <partest/testbase.h>
#include <partest/semaphore.h>
#include <partest/assert.h>

class SemaphoreTests : public partest::TestBase
{
public:
	SemaphoreTests() : TestBase("SemaphoreTests", "Tests for the Semaphore class.")
	{
		partest::TestFlags flags = partest::TEST_FLAGS_INHERIT;

		addTest(partest::TestInfo("Acquire", "Validate semaphore via acquire with single blocking thread."),
			flags,
			PARTEST_CTX(this) { return this->acquire(ctx); });
		addTest(partest::TestInfo("AcquireAsQueue", "Simulate a producer/consumer queue to validate quick concurrency."),
			flags,
			PARTEST_CTX(this) { return this->acquireAsQueue(ctx, 10000); });
		addTest("AcquireWithTryAcquire", "Validate concurrency for try_acquire.",
			flags,
			PARTEST_CTX(this) { return this->tryAcquire(ctx); });
		addTest("acquireWithTryAcquireTimed", "Validate non-blocking acquire with wait times.",
			flags,
			PARTEST_CTX(this) { return this->TryAcquireWithTimeout(ctx); });
		addTest("ReleaseWakesThreads", "Validate that release wakes up waiting threads.",
			flags.withStopOnFail(),
			PARTEST_CTX(this) { return this->releaseWakesThreads(ctx, 10); });
		addTest("thunderingHerdQueue", "Validate large numbers of threads with many concurrent operations.",
			flags,
			PARTEST_CTX(this) { return this->thunderingHerdQueue(ctx, 1000, 10); });
	}

	// Validate acquire and release functionality in a multithreaded context.
	void acquire(TestContext &ctx)
	{
		std::thread semThread;
		partest::counting_semaphore<> sem(0);
		std::atomic<int> counter(0);

		semThread = std::thread([this, &sem, &counter] {
			acquireSemHelper(sem, counter, 1);
		});

		sem.release();
		semThread.join(); // Wait for the thread to finish
		ASSERT_EQUAL(counter, 1);
	}

	// Mock a work queue to validate concurrent acquire/release over two threads
	void acquireAsQueue(TestContext &ctx, unsigned iterations = 10000)
	{
		std::thread producerThread, consumerThread;
		partest::counting_semaphore<> sem(0);
		std::atomic<int> counter(0);

		producerThread = std::thread([&] { releaseSemHelper(sem, iterations); });
		consumerThread = std::thread([&] { acquireSemHelper(sem, counter, iterations); });

		producerThread.join();
		consumerThread.join();

		ASSERT_EQUAL(counter, iterations);
	}

	void tryAcquire(TestContext &ctx)	
	{
		ctx.subtest("AcquiresSerial", "Validate semaphore with default initialization.", PARTEST_CTX()
		{
			partest::counting_semaphore<> sem(0);
			ASSERT_EQUAL(sem.count_snapshot(), 0);
			ASSERT_FALSE(sem.try_acquire()); // Should not be able to acquire since count is 0
			sem.release(); // Release the semaphore, increasing count to 1
			ASSERT_EQUAL(sem.count_snapshot(), 1);
			ASSERT_TRUE(sem.try_acquire()); // Should be able to acquire now
			sem.release(10); // Release the semaphore ten times
			ASSERT_EQUAL(sem.count_snapshot(), 10);
		});

		ctx.subtest("AcquiresMaxValue", "Validate semaphore with max set, and count set to max.", PARTEST_CTX()
		{
			partest::counting_semaphore<> sem(partest::counting_semaphore<10>::max());
			ASSERT_EQUAL(sem.count_snapshot(), 10);
			ASSERT_TRUE(sem.try_acquire()); // Should be able to acquire since count is at max
			ASSERT_EQUAL(sem.count_snapshot(), 9);
			sem.release(); // Release the sem, increasing count to 10
			ASSERT_EQUAL(sem.count_snapshot(), 10);
		});

		ctx.subtest("AcquiresParallel", "Validate that semaphore can be acquired after blocking", PARTEST_CTX()
		{
			std::thread semThread;
			partest::counting_semaphore<> blockedSem(0);
			partest::counting_semaphore<> availableSem(1);
			std::atomic<bool> acquiredBlocked(false);
			std::atomic<bool> acquiredAvailable(false);

			semThread = std::thread([&] {
				acquiredBlocked = blockedSem.try_acquire();
				acquiredAvailable = availableSem.try_acquire();
			});
			
			semThread.join(); // Wait for the thread to finish
			ASSERT_FALSE(acquiredBlocked);
			ASSERT_TRUE(acquiredAvailable);

			// Ensure both semaphores have zero count after join
			ASSERT_EQUAL(blockedSem.count_snapshot(), 0);
			ASSERT_EQUAL(availableSem.count_snapshot(), 0);
		});
	}

	void TryAcquireWithTimeout(TestContext &ctx)
	{
		// Test thread will pause for waitTime before releasing semaphores
		std::chrono::milliseconds waitTime = std::chrono::milliseconds(1000);
		// Time to wait for (hopefully) successful acquires
		std::chrono::milliseconds durationSuccess = waitTime + std::chrono::milliseconds(500);
		// Time to wait for (hopefully) unsuccessful acquires
		std::chrono::milliseconds durationFailure = waitTime - std::chrono::milliseconds(500);

		std::chrono::milliseconds durationZero = std::chrono::milliseconds(0);
		std::chrono::milliseconds durationNegative = std::chrono::milliseconds(-500);

		ctx.subtest("AcquiresWithPositiveDuration", "Validate that semaphore can be acquired after specified wait.", PARTEST_CTX(durationSuccess)
		{
			std::thread forThread, untilThread;
			partest::counting_semaphore<> forSem(0);
			partest::counting_semaphore<> untilSem(0);
			std::atomic<bool> acquiredFor(false);
			std::atomic<bool> acquiredUntil(false);

			// Test thread will pause for waitTime before releasing semaphores
			std::chrono::milliseconds sleepTime = std::chrono::milliseconds(10);

			std::mutex waitMutex;
			std::condition_variable waitCV;
			unsigned readyCount = 0;

			forThread = std::thread([&](){
				std::unique_lock<std::mutex> lock(waitMutex);
				++readyCount;
				lock.unlock();
				waitCV.notify_one();

				acquiredFor = forSem.try_acquire_for(durationSuccess);
			});

			untilThread = std::thread([&](){
				std::unique_lock<std::mutex> lock(waitMutex);
				++readyCount;
				lock.unlock();
				waitCV.notify_one();

				acquiredUntil = untilSem.try_acquire_until(std::chrono::steady_clock::now() + durationSuccess);
			});

			std::unique_lock<std::mutex> lock(waitMutex);
			while(readyCount < 2)
			{
				waitCV.wait(lock);
			}
			lock.unlock();

			std::this_thread::sleep_for(sleepTime);

			forSem.release();
			untilSem.release();

			forThread.join();
			untilThread.join();

			// Ensure both flags are set
			ASSERT_TRUE(acquiredFor);
			ASSERT_TRUE(acquiredUntil);
			// Ensure both semaphores have zero count after join
			ASSERT_EQUAL(forSem.count_snapshot(), 0);
			ASSERT_EQUAL(untilSem.count_snapshot(), 0);
		});

		ctx.subtest(partest::TestInfo("FailsWithPositiveDuration", "Validate that semaphore won't be acquired if wait time expires."), PARTEST_CTX(durationFailure, durationSuccess)
		{
			std::thread forThread, untilThread;
			partest::counting_semaphore<> forSem(0);
			partest::counting_semaphore<> untilSem(0);
			std::atomic<bool> acquiredFor(false);
			std::atomic<bool> acquiredUntil(false);

			// Test thread will pause for waitTime before releasing semaphores
			std::chrono::milliseconds sleepTime = std::chrono::milliseconds(durationSuccess);

			std::mutex waitMutex;
			std::condition_variable waitCV;
			unsigned readyCount = 0;

			forThread = std::thread([&](){
				std::unique_lock<std::mutex> lock(waitMutex);
				++readyCount;
				lock.unlock();
				waitCV.notify_one();

				acquiredFor = forSem.try_acquire_for(durationFailure);
			});

			untilThread = std::thread([&](){
				std::unique_lock<std::mutex> lock(waitMutex);
				++readyCount;
				lock.unlock();
				waitCV.notify_one();

				acquiredUntil = untilSem.try_acquire_until(std::chrono::steady_clock::now() + durationFailure);
			});

			std::unique_lock<std::mutex> lock(waitMutex);
			while(readyCount < 2)
			{
				waitCV.wait(lock);
			}
			lock.unlock();

			std::this_thread::sleep_for(durationSuccess);

			forSem.release();
			untilSem.release();

			forThread.join();
			untilThread.join();

			// Ensure both flags are set
			ASSERT_FALSE(acquiredFor);
			ASSERT_FALSE(acquiredUntil);
			// Ensure both semaphores have count of 1 after join
			ASSERT_EQUAL(forSem.count_snapshot(), 1);
			ASSERT_EQUAL(untilSem.count_snapshot(), 1);
		});

		ctx.subtest("AcquiresWithNegativeDuration", "Validate that an available semaphore is acquired if passed negative time.", PARTEST_CTX(durationNegative)
		{
			std::thread forThread, untilThread;
			partest::counting_semaphore<> forSem(1);
			partest::counting_semaphore<> untilSem(1);
			std::atomic<bool> acquiredFor(false);
			std::atomic<bool> acquiredUntil(false);

			forThread = std::thread([&](){
				acquiredFor = forSem.try_acquire_for(durationNegative);
			});
			untilThread = std::thread([&](){
				acquiredUntil = untilSem.try_acquire_until(std::chrono::steady_clock::now() + durationNegative);
			});

			forThread.join();
			untilThread.join();

			// Ensure both flags are set
			ASSERT_TRUE(acquiredFor);
			ASSERT_TRUE(acquiredUntil);

			// Nothing should have been released, so both semaphores should have count of 0 after join
			ASSERT_EQUAL(forSem.count_snapshot(), 0);
			ASSERT_EQUAL(untilSem.count_snapshot(), 0);
		});

		ctx.subtest("FailsWithNegativeDuration", "Validate that a locked semaphore is not acquired if passed negative time.", PARTEST_CTX(durationNegative)
		{
			std::thread forThread, untilThread;
			partest::counting_semaphore<> forSem(0);
			partest::counting_semaphore<> untilSem(0);
			std::atomic<bool> acquiredFor(false);
			std::atomic<bool> acquiredUntil(false);

			forThread = std::thread([&](){
				acquiredFor = forSem.try_acquire_for(durationNegative);
			});
			untilThread = std::thread([&](){
				acquiredUntil = untilSem.try_acquire_until(std::chrono::steady_clock::now() + durationNegative);
			});

			forThread.join();
			untilThread.join();

			// Ensure both flags are set
			ASSERT_FALSE(acquiredFor);
			ASSERT_FALSE(acquiredUntil);

			// Nothing should have been released, so both semaphores should have count of 0 after join
			ASSERT_EQUAL(forSem.count_snapshot(), 0);
			ASSERT_EQUAL(untilSem.count_snapshot(), 0);
		});

		ctx.subtest("AcquiresWithZeroDuration", "Validate that semaphore returns true if unblocked with zero duration.", PARTEST_CTX(durationZero)
		{
			std::thread forThread, untilThread;
			partest::counting_semaphore<> forSem(1);
			partest::counting_semaphore<> untilSem(1);
			std::atomic<bool> acquiredFor(false);
			std::atomic<bool> acquiredUntil(false);

			forThread = std::thread([&](){
				acquiredFor = forSem.try_acquire_for(durationZero);
			});
			untilThread = std::thread([&](){
				acquiredUntil = untilSem.try_acquire_until(std::chrono::steady_clock::now() + durationZero);
			});

			forThread.join();
			untilThread.join();
			// Ensure flags are set
			ASSERT_TRUE(acquiredFor);
			ASSERT_TRUE(acquiredUntil);
			// Ensure semaphores have count of 0 after join
			ASSERT_EQUAL(forSem.count_snapshot(), 0);
			ASSERT_EQUAL(untilSem.count_snapshot(), 0);
		});

		ctx.subtest("FailsWithZeroDuration", "Validate that semaphore returns false if locked with zero duration.", PARTEST_CTX(durationZero, waitTime)
		{
			std::thread forThread, untilThread;
			partest::counting_semaphore<> forSem(0);
			partest::counting_semaphore<> untilSem(0);
			std::atomic<bool> acquiredFor(false);
			std::atomic<bool> acquiredUntil(false);

			forThread = std::thread([&](){
				acquiredFor = forSem.try_acquire_for(durationZero);
			});
			untilThread = std::thread([&](){
				acquiredUntil = untilSem.try_acquire_until(std::chrono::steady_clock::now() + durationZero);
			});
			std::this_thread::sleep_for(waitTime);

			forThread.join();
			untilThread.join();

			// Ensure flag is set
			ASSERT_FALSE(acquiredFor);
			ASSERT_FALSE(acquiredUntil);
			// Ensure semaphores have count of 0 after join
			ASSERT_EQUAL(forSem.count_snapshot(), 0);
			ASSERT_EQUAL(untilSem.count_snapshot(), 0);
		});
	}

	void releaseWakesThreads(TestContext &ctx, unsigned threadCount = 10)
	{
		if(threadCount < 4)
		{
			ctx.recordLog(partest::LogLevel::Warning, partest::LOG_TYPE_TEST, "Warning: releaseWakesThreads test requires at least 4 threads to run properly. Defaulting to 4 threads.");
			threadCount = 4;
		}
		// RAII wrapper for vector of threads
		struct ThreadVector
		{
			std::vector<std::thread> threads;

			~ThreadVector()
			{
				for(std::thread &thread : threads)
				{
					if(thread.joinable())
						thread.detach();
				}
			}
		};

		ThreadVector pool;
		partest::counting_semaphore<> sem(0);

		std::mutex readyMutex;
		std::mutex completedMutex;

		std::condition_variable readyCV;
		std::condition_variable completedCV;
		std::atomic<unsigned> readyCount(0);
		std::atomic<unsigned> completedCount(0);

		// create a duration
		std::chrono::milliseconds pauseDuration = std::chrono::milliseconds(100);
		std::chrono::milliseconds timeout = std::chrono::milliseconds(2000);

		for(unsigned x = 0; x < threadCount; ++x)
		{
			std::thread waitThread = std::thread([&sem, &readyMutex, &completedMutex, &readyCV, &completedCV, &readyCount, &completedCount](){
				std::unique_lock<std::mutex> readyLock(readyMutex);
				++readyCount;
				readyLock.unlock();

				readyCV.notify_one();
				sem.acquire();

				std::unique_lock<std::mutex> completedLock(completedMutex);
				++completedCount;
				completedLock.unlock();
				completedCV.notify_one();
			});
			pool.threads.emplace_back(std::move(waitThread));
		}

		std::chrono::steady_clock::time_point stopTime = std::chrono::steady_clock::now() + timeout;
		std::unique_lock<std::mutex> readyLock(readyMutex);
		while(readyCount < threadCount)
		{
			std::cv_status status = readyCV.wait_until(readyLock, stopTime);
			ASSERT_EQUAL(status, std::cv_status::no_timeout);
		}
		readyLock.unlock();

		// Try waking one thread
		sem.release(1);
		std::this_thread::sleep_for(pauseDuration);		

		std::unique_lock<std::mutex> completedLock(completedMutex);
		stopTime = std::chrono::steady_clock::now() + timeout;
		while(completedCount < 1)
		{
			std::cv_status status = completedCV.wait_until(completedLock, stopTime);
			ASSERT_EQUAL(status, std::cv_status::no_timeout);
		}
		completedLock.unlock();

		if(threadCount < 2)
			return;

		// Try waking two threads
		sem.release(2);
		std::this_thread::sleep_for(pauseDuration);

		completedLock.lock();
		stopTime = std::chrono::steady_clock::now() + timeout;
		while(completedCount < 3)
		{
			std::cv_status status = completedCV.wait_until(completedLock, stopTime);
			ASSERT_EQUAL(status, std::cv_status::no_timeout);
		}
		completedLock.unlock();

		if(threadCount < 4)
			return;

		// Try waking all remaining threads
		sem.release(threadCount - 3);
		std::this_thread::sleep_for(pauseDuration);

		completedLock.lock();
		stopTime = std::chrono::steady_clock::now() + timeout;
		while(completedCount < threadCount)
		{
			std::cv_status status = completedCV.wait_until(completedLock, stopTime);
			ASSERT_EQUAL(status, std::cv_status::no_timeout);
		}
		completedLock.unlock();

		for(unsigned x = 0; x < threadCount; ++x)
		{
			pool.threads[x].join();
		}
		ASSERT_EQUAL(completedCount, threadCount);
	}

	void thunderingHerdQueue(TestContext &ctx, unsigned iterationsPerThread, unsigned threadsPerChannel = 10)
	{
		std::vector<std::thread> producers, consumers;
		//std::vector<partest::counting_semaphore<1>> finishedFlags;
		
		partest::counting_semaphore<> sem(0);
		std::atomic<int> counter(0);

		for(unsigned x = 0; x < threadsPerChannel; ++x)
		{
			std::thread producerThread = std::thread([&] { releaseSemHelper(sem, iterationsPerThread); });
			std::thread consumerThread = std::thread([&] { acquireSemHelper(sem, counter, iterationsPerThread); });

			producers.emplace_back(std::move(producerThread));
			consumers.emplace_back(std::move(consumerThread));
		}

		for(unsigned x = 0; x < threadsPerChannel; ++x)
		{
			producers[x].join();
			consumers[x].join();
		}

		ASSERT_EQUAL(counter, iterationsPerThread * threadsPerChannel);
	}

private:
	void releaseSemHelper(partest::counting_semaphore<> &sem, unsigned iterations)
	{
		for(unsigned x = 0; x < iterations; ++x)
			sem.release();
	}

	void acquireSemHelper(partest::counting_semaphore<> &sem, std::atomic<int> &counter, unsigned iterations)
	{
		for(unsigned x = 0; x < iterations; ++x)
		{
			sem.acquire();
			++counter;
		}
	}
};

#endif
