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
		PartestRunner() = default;
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
		static PartestRunner &getInstance()
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
				test->runTests();
			}
		}

		/**
		* Run a specific test by name.
		* 
		* @param name The name of the test to run.
		*/
		void runTestWithName(const std::string &name)
		{
			for(PartestBase *test : m_tests)
			{
				if(test->getName() == name)
				{
					test->runTests();
					return;
				}
			}
			std::cerr << "Error: No test found with name '" << name << "'." << std::endl;
		}

		void printAllTestTrees() const
		{
			for(PartestBase *test : m_tests)
			{
				test->printTestTree();
			}
		}
	};
};

#endif // PARTESTRUNNER_H