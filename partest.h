#ifndef PARTEST_H
#define PARTEST_H

// Public interface for the Partest testing framework

#include "source/partestcommon.h"
#include "source/partestbase.h"
#include "source/partestrunner.h"

namespace partest
{
	inline partest::PartestRunner &testRunner()
	{
		return partest::PartestRunner::getInstance();
	}

	/**
	* Add a test class to the global test runner.
	* 
	* @param test A unique pointer to the PartestBase instance representing the test class to add.
	*/
	void addTestClass(std::unique_ptr<partest::PartestBase> test)
	{
		testRunner().addTest(std::move(test));
	}

	/**
	* Run all registered tests in the global test runner, in the order they were added.
	*/
	void runAllTests()
	{
		testRunner().runAllTests();
	}

	/**
	* Run a specific test class by name in the global test runner.
	*/
	void runTestClassWithName(PARTEST_STRING_PARAM name)
	{
		testRunner().runTestWithName(name);
	}

	/**
	* Display the test tree structure for all registered tests in the global test runner.
	*/
	void displayAllTests()
	{
		testRunner().printAllTestTrees();
	}

	unsigned getTopLevelFailures()
	{
		return testRunner().getTopLevelFailures();
	}

	unsigned getAssertionFailureCount()
	{
		return testRunner().getAllAssertionFailures();
	}
};

#endif