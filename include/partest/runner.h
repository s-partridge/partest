#ifndef PARTEST_RUNNER_H
#define PARTEST_RUNNER_H

#include <vector>
#include <thread>

#include <partest/eventdispatcher.h>
#include <partest/simplelogger.h>
#include <partest/testframe.h>
#include <partest/testbase.h>

namespace partest
{
	class TestRunner
	{
	private:
		std::vector<TestBase *> m_tests; // Vector of tests to run
		std::vector<EventReporterInterface *> m_reporters;
		EventDispatcherInterface *m_dispatcher;
		bool m_concurrent;

		TestRunner(bool concurrent = true) : m_concurrent(concurrent)
		{
			if(concurrent)
			{
				m_dispatcher = new ConcurrentEventDispatcher();
			}
			else
			{
				m_dispatcher = new SerialEventDispatcher();
			}
		}
	public:
		// Delete copy and move constructors and assignment operators to enforce singleton pattern
		TestRunner(const TestRunner &) = delete;
		TestRunner &operator=(const TestRunner &) = delete;
		TestRunner(TestRunner &&) = delete;
		TestRunner &operator=(TestRunner &&) = delete;

		~TestRunner()
		{
			delete m_dispatcher;

			for(EventReporterInterface *reporter: m_reporters)
			{
				delete reporter;
			}

			for(TestBase *test : m_tests)
			{
				delete test;
			}
		}

		/**
		* Get the singleton instance of TestRunner.
		*/
		static TestRunner &getInstance() noexcept
		{
			static TestRunner instance;
			return instance;
		}

		void addReporter(std::unique_ptr<EventReporterInterface> reporter)
		{
			m_dispatcher->registerReporter(reporter.get());
			m_reporters.push_back(reporter.release());
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

			if(!ran)
				recordLog(LogLevel::Error, LOG_TYPE_DEFAULT, "Error: No test found with name '" + PARTEST_STRING_PARAM_TO_STRING(name) + "'.\n");

			m_dispatcher->killDispatcher();

			if(m_concurrent)
				dispatcherThread.join();
		}

		bool recordLog(LogLevel level, PARTEST_STRING_PARAM logType, PARTEST_STRING_PARAM message)
		{
			return m_dispatcher->pushEvent(makeEventLog(TestFrameView::getNullTestFrameView(), LogEntry(level, logType, message)));
		}

		void readAllTests(TestFrameReaderInterface *reader)
		{
			for(TestBase *test : m_tests)
			{
				test->readTestTree(reader);
			}
		}

		// TODO: Finish once PostMortemReporters are implemented
		void printAllTestTrees() const
		{
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