// Author: Samuel Partridge
// 
// Partest is a lightweight C++ testing framework designed for simplicity and ease of use.
// It allows developers to define and run tests with minimal boilerplate code, making it ideal for quick validation of code functionality.
// Header-only implementation for easy integration into existing projects.
#ifndef PARTEST_H
#define PARTEST_H

#include <iostream>
#include <string>
#include <vector>
#include <functional>

#include "partesttypes.h"

namespace partest
{
	class TestFrame
	{		
		std::vector<TestFrame *> subtests; // Vector of sub-tests
		TestFrame *m_parent = nullptr; // Pointer to the parent test frame

	public:
		TestFrame() {};
		TestFrame(const TestFrame &) = delete; // Disable copy constructor
		TestFrame &operator=(const TestFrame &) = delete; // Disable copy assignment

		~TestFrame()
		{
			for(TestFrame *subtest : subtests)
				if(subtest != nullptr)
				{
					delete subtest;
				}
		}

		TestParams parameters; // Test parameters including flags
		TestResult result; // Result of the test

		/**
		* Add a subtest to the current test frame.
		* 
		* @param subtest Pointer to the subtest to be added
		*/
		void addSubtest(std::unique_ptr<TestFrame> &subtest)
		{
			subtests.push_back(subtest.release());
			subtests.back()->m_parent = this;
		}

		/**
		* Get the parent test frame.
		*/
		const TestFrame *getParent() const { return m_parent; }
	};

	/**
	* Base class for all partest tests.
	*
	*/
	class PartestBase
	{
	private:
		std::string m_name; // Name of the test class
		std::string m_description; // Description of the test class

		RunnerState m_currentState; // Current state of the test runner

		// Vector of test functions and their parameters
		std::vector<std::pair<TestParams, std::function<TestStatus()>>> m_tests;
		std::unique_ptr<TestFrame> m_testTree; // Dynamically growing tree of test frames
		std::vector<TestResult> m_results; // Vector of test results

		TestFrame *m_currentFrame; // Pointer to the current test frame
	protected:

		/** 
		* Adds a test function to the list of tests to be executed. Expected to be called in the constructor of derived classes.
		* 
		* Example usage:
		*    addTest(testParamsA, [this]() { return this->testFunction(parameterListA) });
		*    addTest(testParamsB, [this]() { return this->testFunction(parameterListB) });
		* 
		* @param params Test parameters including name and flags
		* @param testFunc The test function to be executed, which should return a TestStatus. Typically a lambda that calls a member function with specific parameters
		*/
		void addTest(const TestParams &params, const std::function<TestStatus()> &testFunc)
		{
			m_tests.emplace_back(params, testFunc);

			std::unique_ptr<TestFrame> newFrame = std::make_unique<TestFrame>();
			newFrame->parameters = params;
			newFrame->result = TestResult(AWAITING, params.name, "Test is awaiting execution.");

			m_testTree->addSubtest(std::move(newFrame));
		}

		void setRunnerState(const RunnerState &state) { m_currentState = state; }

		void addResult(const TestStatus &status, const std::string &testName)
		{
			std::string message = "Test '" + testName;
			switch(status)
			{
			case PASSED:
				message += "' passed successfully.";
				break;
			case MIXED:
				message +=  "' had mixed results.";
				break;
			case FAILED:
				message += "' failed.";
				break;
			case SKIPPED:
				message += "' was skipped.";
				break;
			default:
				message += "' is awaiting execution.";
				break;
			}
			m_results.emplace_back(status, message);
		}
	public:
		PartestBase(const std::string &name, const std::string &description) : m_name(name), m_description(description)
		{
			// Initialize the root test frame. This frame is not associated with any specific test but serves as the root of the test tree.
			// Its primary purpose is to contain information such as the overall test suite name and description in the same collection as the individual tests.
			m_testTree = std::make_unique<TestFrame>();
			m_testTree->parameters.name = name;
			m_testTree->parameters.description = description;
			m_testTree->result = TestResult(AWAITING, name, "Test is awaiting execution.");

			m_currentFrame = m_testTree.get();
		}
		virtual ~PartestBase() = default;

		std::string getName() const { return m_name; }
		std::string getDescription() const { return m_description; }

		void setName(const std::string &name) { m_name = name; }
		void setDescription(const std::string &description) { m_description = description; }

		/**
		* Setup function to be overridden by derived classes
		*/
		virtual void setup() {}

		/**
		* Teardown function to be overridden by derived classes
		*/
		virtual void teardown() {}

		bool initializeSubtest(const TestParams &newTestParams)
		{
			std::unique_ptr<TestFrame>newSubtest = std::make_unique<TestFrame>();
			newSubtest->parameters = newTestParams;
			newSubtest->result = TestResult(AWAITING, newTestParams.name, "Subtest is awaiting execution.");

			m_currentFrame->addSubtest(std::move(newSubtest));
			return true;
		}

		bool finalizeSubtest()
		{
			return false;
		}

		void runTests()
		{
			// Call setup function
			setup();
			
			// Iterate through all registered tests
			for(const std::pair<const TestParams &, std::function<TestStatus()>> &test : m_tests)
			{
				m_currentState.setFlags(test.first);
				addResult(test.second(), test.first.name); // Execute the test function and aggregate the result
			}

			// Call teardown function
			teardown();
		}

		/**
		* Get the results of all executed tests.
		* 
		* @return A constant reference to a vector of TestResult objects representing the results of all executed tests.
		*/
		const std::vector<TestResult> &getResults() const { return m_results; }
	};

} // namespace partest

#endif // PARTEST_H