#ifndef PARTEST_H
#define PARTEST_H

// Public interface for the Partest testing framework

#include <partest/common.h>
#include <partest/testbase.h>
#include <partest/runner.h>
#include <partest/simplelogger.h>
#include <partest/junitlogger.h>
#include <partest/fileops.h>

namespace partest
{
	inline partest::TestRunner &testRunner()
	{
		return partest::TestRunner::getInstance();
	}

	inline void initializeSuite(int argc, const char **argv)
	{
		std::string xmlPath = makeAbsolutePath("testResults.xml");
		testRunner().addReporter(partest::make_unique<SimpleLogger>());
		testRunner().addReporter(partest::make_unique<JUnitLogger>("testResults.xml"));

		if(!maybeOpenFile(xmlPath, std::ios::out))
			testRunner().recordLog(LogLevel::Error, LOG_TYPE_DEFAULT, "Error: Unable to open file for JUnit reporting: '" + xmlPath + "'.\n");
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

	inline unsigned getTopLevelFailures()
	{
		return testRunner().getTopLevelFailures();
	}

	inline unsigned getAssertionFailureCount()
	{
		return testRunner().getAllAssertionFailures();
	}
};

#endif