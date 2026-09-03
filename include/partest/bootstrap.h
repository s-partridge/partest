#ifndef PARTEST_H
#define PARTEST_H

// Public interface for the Partest testing framework

#include <partest/common.h>
#include <partest/testbase.h>
#include <partest/runner.h>
#include <partest/simplelogger.h>
#include <partest/junitlogger.h>
#include <partest/fileops.h>
#include <partest/cli.h>

namespace partest
{
	inline partest::TestRunner &testRunner()
	{
		return partest::TestRunner::getInstance();
	}

	inline void initializeSuite(int argc, const char **argv)
	{
		testRunner().parseCommandLineArgs(argc, argv);
		ValidArgs args = testRunner().getArgs();

		std::string xmlPath;
		if(args.output() && !args.getOutputFile().empty())
		{
			xmlPath = makeAbsolutePath(args.getOutputFile());
		}
		else
		{
			xmlPath = makeAbsolutePath("testResults.xml");
			if(args.output())
				testRunner().recordLog(LogLevel::Warning, LOG_TYPE_DEFAULT, "No output file specified. Defaulting to '" + xmlPath + "'.\n");
		}

		if(args.filtered() && args.getTestNames().empty())
		{
			testRunner().recordLog(LogLevel::Warning, LOG_TYPE_DEFAULT, "Filter flag was specified, but no test suites were provided. Running in default configuration.\n");
		}

		testRunner().addReporter(partest::make_unique<SimpleLogger>(std::cout, false, partest::LogLevel::Warning));
		testRunner().addReporter(partest::make_unique<JUnitLogger>(xmlPath));

		if(!maybeOpenFile(xmlPath, std::ios::out))
			testRunner().recordLog(LogLevel::Error, LOG_TYPE_DEFAULT, "Unable to open file for JUnit reporting: '" + xmlPath + "'.\n");
	}

	/**
	* Add a test class to the global test runner.
	* 
	* @param test A unique pointer to the TestBase instance representing the test class to add.
	*/
	inline void addTestClass(std::unique_ptr<partest::TestBase> test)
	{
		testRunner().addTest(std::move(test));
	}

	/**
	* Run all registered tests in the global test runner, in the order they were added.
	*/
	inline void runAllTests()
	{
		testRunner().runAllTests();
	}

	/**
	* Run a specific test class by name in the global test runner.
	*/
	inline void runTestClassWithName(PARTEST_STRING_PARAM name)
	{
		testRunner().runTestWithName(name);
	}

	/**
	* Display the test tree structure for all registered tests in the global test runner.
	*/
	inline void displayAllTests()
	{
		testRunner().printAllTestTrees();
	}

	inline size_t getTopLevelFailures()
	{
		return testRunner().getTopLevelFailures();
	}

	inline size_t getSkipCount()
	{
		return testRunner().getSkipCount();
	}

	inline size_t getAssertionFailureCount()
	{
		return testRunner().getAllAssertionFailures();
	}
};

#endif