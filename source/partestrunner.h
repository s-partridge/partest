#ifndef PARTESTRUNNER_H
#define PARTESTRUNNER_H

#include <vector>

#include "partestbase.h"

namespace partest
{
	class PartestRunner
	{
	private:
		std::vector<PartestBase *> m_tests; // Vector of tests to run
		PartestRunner() noexcept = default;
	public:
		// Delete copy and move constructors and assignment operators to enforce singleton pattern
		PartestRunner(const PartestRunner &) = delete;
		PartestRunner &operator=(const PartestRunner &) = delete;
		PartestRunner(PartestRunner &&) = delete;
		PartestRunner &operator=(PartestRunner &&) = delete;

		~PartestRunner()
		{
			for(PartestBase *test : m_tests)
			{
				if(test != nullptr)
				{
					delete test;
				}
			}
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
		* @param test A pointer to the PartestBase instance representing the test to add.
		*/
		void addTest(std::unique_ptr<PartestBase> test)
		{
			m_tests.push_back(test.release());
		}

		/**
		* Run all added tests in sequence.
		*/
		void runAllTests()
		{
			for(PartestBase *test : m_tests)
			{
				test->run();
			}
		}

		/**
		* Run a specific test by name.
		* 
		* @param name The name of the test to run.
		*/
		void runTestWithName(PARTEST_STRING_PARAM name)
		{
			for(PartestBase *test : m_tests)
			{
				if(test->getName() == name)
				{
					test->run();
					return;
				}
			}
			std::cerr << "Error: No test found with name '" << name << "'." << std::endl;
		}

		void printAllTestTrees() const
		{
			for(PartestBase *test : m_tests)
			{
				test->printLogs(LogLevel::Info, 10);
			}
		/*	for(PartestBase *test : m_tests)
			{
				test->printTestTree();
			}*/
		}

		unsigned getTopLevelFailures() const
		{
			unsigned failureCount = 0;
			for(PartestBase *test : m_tests)
			{
				failureCount += test->getTestFailureCount();
			}

			return failureCount;
		}

		unsigned getAllAssertionFailures()
		{
			unsigned failureCount = 0;
			for(PartestBase *test : m_tests)
			{
				failureCount += test->getAssertionFailureCount();
			}

			return failureCount;
		}
	};
};

#endif // PARTESTRUNNER_H