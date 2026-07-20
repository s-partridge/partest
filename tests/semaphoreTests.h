#ifndef SEMAPHORE_TESTS_H
#define	SEMAPHORE_TESTS_H

#include <atomic>
#include <vector>
#include <chrono>
#include <thread>

#include "partestbase.h"
#include "partestsemaphore.h"
#include "partestassert.h"

class SemaphoreTests : public partest::PartestBase
{
public:
	SemaphoreTests() : PartestBase("SemaphoreTests", "Tests for the Semaphore class.")
	{
		partest::TestFlags flags = partest::TEST_FLAGS_INHERIT;

		addTest(partest::TestInfo("AcquireSerial", "Validate semaphore in isolation on a single non-blocking thread."),
			flags,
			[this]() { return this->acquireSerial(); });
		addTest(partest::TestInfo("AcquireParallel", "Validate semaphore with single blocking thread."),
			flags,
			[this]() { return this->acquireParallel(); });
		addTest(partest::TestInfo("AcquireParallelAsQueue", "Simulate a producer/consumer queue to validate quick concurrency."),
			flags,
			[this]() { return this->acquireParallelAsQueue(10000); });
		addTest(partest::TestInfo("acquireWithTryAcquire", "Validate non-blocking acquire with wait times."),
			flags,
			[this]() { return this->acquireWithTryAcquire(); });
		addTest(partest::TestInfo("thunderingHerdQueue", "Validate large numbers of threads with many concurrent operations."),
			flags,
			[this]() { return this->thunderingHerdQueue(1000, 10); });
	}

	// Validate try_acquire and release functionality of the counting_semaphore class in a serial context.
	void acquireSerial()
	{
		subtest(partest::TestInfo("AcquireDefault", "Validate semaphore with default initialization."), [&]()
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
		subtest(partest::TestInfo("AcquireMaxValue", "Validate semaphore with max set, and count set to max."), [&]()
		{
			partest::counting_semaphore<> sem(partest::counting_semaphore<10>::max());
			ASSERT_EQUAL(sem.count_snapshot(), 10);
			ASSERT_TRUE(sem.try_acquire()); // Should be able to acquire since count is at max
			ASSERT_EQUAL(sem.count_snapshot(), 9);
			sem.release(); // Release the sem, increasing count to 10
			ASSERT_EQUAL(sem.count_snapshot(), 10);
		});
	}

	// Validate acquire and release functionality in a multithreaded context.
	void acquireParallel()
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
	void acquireParallelAsQueue(unsigned iterations = 10000)
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

	void acquireWithTryAcquire()
	{
		// Test thread will pause for waitTime before releasing semaphores
		std::chrono::milliseconds waitTime = std::chrono::milliseconds(1000);
		// Time to wait for (hopefully) successful acquires
		std::chrono::milliseconds durationSuccess = waitTime + std::chrono::milliseconds(500);
		// Time to wait for (hopefully) unsuccessful acquires
		std::chrono::milliseconds durationFailure = waitTime - std::chrono::milliseconds(500);

		std::chrono::milliseconds durationZero = std::chrono::milliseconds(0);
		std::chrono::milliseconds durationError = std::chrono::milliseconds(-500);

		subtest(partest::TestInfo("TryAcquireSucess", "Validate that semaphore can be acquired after specified wait."), [&]()
		{
			std::thread forThread, untilThread;
			partest::counting_semaphore<> forSem(0);
			partest::counting_semaphore<> untilSem(0);
			std::atomic<bool> acquiredFor(false);
			std::atomic<bool> acquiredUntil(false);

			forThread = std::thread([&](){
				acquiredFor = forSem.try_acquire_for(durationSuccess);
			});
			untilThread = std::thread([&](){
				acquiredUntil = untilSem.try_acquire_until(std::chrono::steady_clock::now() + durationSuccess);
			});
			std::this_thread::sleep_for(waitTime);

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

		subtest(partest::TestInfo("TryAcquireFailure", "Validate that semaphore won't be acquired if wait time expires."), [&]()
		{
			std::thread forThread, untilThread;
			partest::counting_semaphore<> forSem(0);
			partest::counting_semaphore<> untilSem(0);
			std::atomic<bool> acquiredFor(false);
			std::atomic<bool> acquiredUntil(false);

			forThread = std::thread([&](){
				acquiredFor = forSem.try_acquire_for(durationFailure);
			});
			untilThread = std::thread([&](){
				acquiredUntil = untilSem.try_acquire_until(std::chrono::steady_clock::now() + durationFailure);
			});
			std::this_thread::sleep_for(waitTime);

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

		subtest(partest::TestInfo("TryAcquireError", "Validate that semaphore returns false if passed negative time."), [&]()
		{
			std::thread forThread, untilThread;
			partest::counting_semaphore<> forSem(0);
			partest::counting_semaphore<> untilSem(0);
			std::atomic<bool> acquiredFor(false);
			std::atomic<bool> acquiredUntil(false);

			forThread = std::thread([&](){
				acquiredFor = forSem.try_acquire_for(durationError);
			});
			untilThread = std::thread([&](){
				acquiredUntil = untilSem.try_acquire_until(std::chrono::steady_clock::now() + durationError);
			});
			std::this_thread::sleep_for(waitTime);

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

		subtest(partest::TestInfo("TryAcquireZeroPass", "Validate that semaphore returns true if unblocked with zero duration."), [&]()
		{
			std::thread forThread;
			partest::counting_semaphore<> forSem(0);
			std::atomic<bool> acquiredFor(false);

			forSem.release();

			forThread = std::thread([&](){
				acquiredFor = forSem.try_acquire_for(durationZero);
			});

			forThread.join();
			// Ensure flag is set
			ASSERT_TRUE(acquiredFor);
			// Ensure semaphore has count of 0 after join
			ASSERT_EQUAL(forSem.count_snapshot(), 0);
		});

		subtest(partest::TestInfo("TryAcquireZeroFail", "Validate that semaphore returns false if locked with zero duration."), [&]()
		{
			std::thread forThread;
			partest::counting_semaphore<> forSem(0);
			std::atomic<bool> acquiredFor(false);

			forThread = std::thread([&](){
				acquiredFor = forSem.try_acquire_for(durationZero);
			});
			std::this_thread::sleep_for(waitTime);
			forSem.release();

			forThread.join();

			// Ensure flag is set
			ASSERT_FALSE(acquiredFor);
			// Ensure semaphore has count of 1 after join
			ASSERT_EQUAL(forSem.count_snapshot(), 1);
		});
	}

	void thunderingHerdQueue(unsigned iterationsPerThread, unsigned threadsPerChannel = 10)
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