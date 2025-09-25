#ifndef PARTEST_H
#define PARTEST_H

// Public interface for the Partest testing framework

#include "source/partestbase.h"
#include "source/partestrunner.h"

namespace partest
{
	/**
	* Add a test class to the global test runner.
	* 
	* @param test A unique pointer to the PartestBase instance representing the test class to add.
	*/
	void addTestClass(std::unique_ptr<partest::PartestBase> test)
	{
		partest::PartestRunner::getInstance().addTest(std::move(test));
	}

	/**
	* Run all registered tests in the global test runner, in the order they were added.
	*/
	void runAllTests()
	{
		partest::PartestRunner::getInstance().runAllTests();
	}

	/**
	* Run a specific test class by name in the global test runner.
	*/
	void runTestClassWithName(const std::string &name)
	{
		partest::PartestRunner::getInstance().runTestWithName(name);
	}

	/**
	* Display the test tree structure for all registered tests in the global test runner.
	*/
	void displayAllTests()
	{
		partest::PartestRunner::getInstance().printAllTestTrees();
	}

	partest::PartestRunner &testRunner()
	{
		return partest::PartestRunner::getInstance();
	}
};

#endif