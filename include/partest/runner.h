#ifndef PARTEST_RUNNER_H
#define PARTEST_RUNNER_H

#include <vector>
#include <thread>

#include <partest/eventdispatcher.h>
#include <partest/simplelogger.h>
#include <partest/testbase.h>

namespace partest
{
	class PartestRunner
	{
	private:
		std::vector<TestBase *> m_tests; // Vector of tests to run
		EventDispatcherInterface *m_dispatcher;
		SimpleLogger m_logger;
		bool m_concurrent;

		PartestRunner(bool concurrent = true) : m_concurrent(concurrent), m_logger(std::cout)
		{
			if(concurrent)
			{
				m_dispatcher = new ConcurrentEventDispatcher();
			}
			else
			{
				m_dispatcher = new SerialEventDispatcher();
			}

			m_dispatcher->registerReporter(&m_logger);
		}
	public:
		// Delete copy and move constructors and assignment operators to enforce singleton pattern
		PartestRunner(const PartestRunner &) = delete;
		PartestRunner &operator=(const PartestRunner &) = delete;
		PartestRunner(PartestRunner &&) = delete;
		PartestRunner &operator=(PartestRunner &&) = delete;

		~PartestRunner()
		{
			for(TestBase *test : m_tests)
			{
				delete test;
			}

			delete m_dispatcher;
		}

		/**
		* Get the singleton instance of the PartestRunner.
		*/
		static PartestRunner &getInstance() noexcept
		{
			static PartestRunner instance;
			return instance;
		}


		/**
		* Add a test to the runner.
		* 
		* @param test A pointer to the TestBase instance representing the test to add.
		*/
		void addTest(std::unique_ptr<TestBase> test)
		{
			test->configureEventEmitter({m_dispatcher});
			m_tests.push_back(test.release());
		}

		/**
		* Run all added tests in sequence.
		*/
		void runAllTests()
		{
			std::thread dispatcherThread;
			if(m_concurrent)
				dispatcherThread = std::thread([this]() { this->m_dispatcher->dispatchEvents(); });

			for(TestBase *test : m_tests)
			{
				test->run();
			}

			m_dispatcher->killDispatcher();

			if(m_concurrent)
				dispatcherThread.join();
		}

		/**
		* Run a specific test by name.
		* 
		* @param name The name of the test to run.
		*/
		void runTestWithName(PARTEST_STRING_PARAM name)
		{
			std::thread dispatcherThread;
			if(m_concurrent)
				dispatcherThread = std::thread([this]() { this->m_dispatcher->dispatchEvents(); });

			bool ran = false;
			for(TestBase *test : m_tests)
			{
				if(test->getName() == name)
				{
					test->run();
					ran = true;
				}
			}

			m_dispatcher->pushEvent(EVENT_LOG, makeEventLog(0, 0, LogEntry(LogLevel::Error, PARTEST_LOG_TYPE_DEFAULT, "Error: No test found with name '" + PARTEST_STRING_PARAM_TO_STRING(name) + "'.\n")));

			m_dispatcher->killDispatcher();

			if(m_concurrent)
				dispatcherThread.join();
		}

		void printAllTestTrees() const
		{
			for(TestBase *test : m_tests)
			{
				test->printLogs(LogLevel::Info, 10);
			}
		/*	for(TestBase *test : m_tests)
			{
				test->printTestTree();
			}*/
		}

		unsigned getTopLevelFailures() const
		{
			unsigned failureCount = 0;
			for(TestBase *test : m_tests)
			{
				failureCount += test->getTestFailureCount();
			}

			return failureCount;
		}

		unsigned getAllAssertionFailures() const
		{
			unsigned failureCount = 0;
			for(TestBase *test : m_tests)
			{
				failureCount += test->getAssertionFailureCount();
			}

			return failureCount;
		}
	};
};

#endif // PARTESTRUNNER_H