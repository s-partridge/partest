#ifndef PARTEST_RUNNER_H
#define PARTEST_RUNNER_H

#include <vector>
#include <thread>

#include <partest/eventdispatcher.h>
#include <partest/simplelogger.h>
#include <partest/testframe.h>
#include <partest/testbase.h>
#include <partest/cli.h>

namespace partest
{
	class TestRunner
	{
	private:
		std::vector<TestBase *> m_tests; // Vector of tests to run
		std::vector<EventReporterInterface *> m_reporters;
		EventDispatcherInterface *m_dispatcher;
		ValidArgs m_args;

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

		void runTestsByName()
		{
			const std::vector<TestNameURL> &testNames = m_args.getTestNames();
			bool foundAny = false;

			for(TestBase *test : m_tests)
			{
				// For now, only validate the top-level test name.
				// TODO: Expand to support hierarchical test names.
				for(const TestNameURL &name : testNames)
				{
					if(test->getName() == name.front())
					{
						test->run();
						foundAny = true;
					}
				}
			}
			if(!foundAny)
				recordLog(LogLevel::Error, LOG_TYPE_DEFAULT, "No tests found matching the provided names.\n");
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

			for(TestBase *test: m_tests)
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

		const ValidArgs &getArgs() const noexcept { return m_args; }

		bool parseCommandLineArgs(int argc, const char **argv)
		{
			m_args = parseArgs(argc, argv);
			return m_args.filtered();
		}

		/**
		* Add a reporter to the runner.
		* 
		* @param reporter A unique pointer to the EventReporterInterface instance representing the reporter to add.
		*/
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

			if(m_args.filtered() && !m_args.getTestNames().empty())
				runTestsByName();
			else
			{
				for(TestBase *test : m_tests)
				{
					test->run();
				}
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
			return m_dispatcher->pushEvent(makeEventLog(TestFrameView::getNullTestFrameView(), LogEntry(level, logType, message), std::chrono::system_clock::now()));
		}

		/**
		* Read all tests using the provided TestFrameReaderInterface. Test suites cannot be read until they have finished running, so this function will return false if any test has not yet completed.
		* 
		* @param reader A pointer to the TestFrameReaderInterface used to read the test frames.
		* @return true if all tests were read successfully, false otherwise.
		*/
		bool readAllTests(TestFrameReaderInterface *reader)
		{
			bool success = true;
			for(TestBase *test : m_tests)
			{
				success &= test->readTestTree(reader);
			}			
			return success;
		}

		// TODO: Finish once PostMortemReporters are implemented
		void printAllTestTrees() const
		{
		/*	for(TestBase *test : m_tests)
			{
				test->printTestTree();
			}*/
		}

		size_t getTopLevelFailures() const
		{
			size_t failureCount = 0;
			for(TestBase *test : m_tests)
			{
				failureCount += test->getTestFailureCount();
			}

			return failureCount;
		}

		size_t getAllAssertionFailures() const
		{
			size_t failureCount = 0;
			for(TestBase *test : m_tests)
			{
				failureCount += test->getAssertionCount(true);
			}

			return failureCount;
		}

		size_t getSkipCount() const
		{
			size_t skipCount = 0;
			for(TestBase *test : m_tests)
			{
				skipCount += test->getTestSkippedCount();
			}
			return skipCount;
		}
	};
};

#endif // PARTESTRUNNER_H