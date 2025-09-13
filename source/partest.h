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

namespace partest
{
	/**
	* Enum type representing the status of a test.
	*
	* AWAITING - The test is awaiting execution.
	* RUNNING - The test is currently running.
	* PASSED - The test passed successfully.
	* FAILED - The test failed.
	* MIXED - The test contains both passed and failed sub-tests.
	* SKIPPED - The test was skipped.
	*/
	enum TestStatus
	{
		AWAITING = 0,
		RUNNING,
		PASSED,
		FAILED,
		MIXED,
		SKIPPED
	};

	/**
	* Overloaded stream extraction operator for TestStatus enum.
	*/
	inline std::istream &operator>>(std::istream &in, TestStatus &status)
	{
		std::string statusString;
		in >> statusString;
		if(statusString == "AWAITING")
			status = AWAITING;
		else if(statusString == "RUNNING")
			status = RUNNING;
		else if(statusString == "PASSED")
			status = PASSED;
		else if(statusString == "FAILED")
			status = FAILED;
		else if(statusString == "MIXED")
			status = MIXED;
		else if(statusString == "SKIPPED")
			status = SKIPPED;
		else
			status = AWAITING; // Default to AWAITING for unknown strings
		return in;
	}

	/**
	* Overloaded stream insertion operator for TestStatus enum.
	*/
	inline std::ostream &operator<<(std::ostream &out, const TestStatus &status)
	{
		std::string statusString;
		switch(status)
		{
		case AWAITING:
			statusString = "AWAITING";
			break;
		case RUNNING:
			statusString = "RUNNING";
			break;
		case PASSED:
			statusString = "PASSED";
			break;
		case FAILED:
			statusString = "FAILED";
			break;
		case MIXED:
			statusString = "MIXED";
			break;
		case SKIPPED:
			statusString = "SKIPPED";
			break;
		default:
			statusString = "UNKNOWN";
			break;
		}
		out << statusString;
		return out;
	}

	/**
	* Struct representing flags for a test.
	*/
	struct TestParams
	{
		std::string name; // Name of the test
		std::string description; // Description of the test
		bool skip = false; // Whether to skip the test
		bool stopOnFail = false; // Whether to stop execution on failure
		bool stopSubtestOnFail = false; // Whether to stop subtest execution on failure
		bool verbose = false; // Whether to run the test in verbose mode
	};

	struct RunnerState
	{
		bool stopOnFail = false; // Whether to stop execution on failure
		bool stopSubtestOnFail = false; // Whether to stop execution on failure
		bool verbose = false; // Whether to run tests in verbose mode

		/**
		* Set flags based on TestParams
		* 
		* @param params The TestParams object containing the flags to set
		*/
		void setFlags(const TestParams &params)
		{
			stopOnFail = params.stopOnFail;
			stopSubtestOnFail = params.stopSubtestOnFail;
			verbose = params.verbose;
		}
	};
	/**
	* Struct representing the result of a test.
	* 
	* status - The status of the test.
	* message - A message providing additional information about the test result.
	*/
	struct TestResult
	{
		// Status of the test
		TestStatus status;

		// Status of the test
		std::string testName;

		// Message providing additional information about the test result
		std::string message;

		TestResult(TestStatus status, std::string testName, const std::string &message) : status(status), testName(testName), message(message) {}

		inline std::ostream &operator<<(std::ostream &out) const
		{
			out << "Test '" << testName << "' - Status: " << status << " - Message: " << message;
			return out;
		}
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
		std::vector<std::pair<TestParams, std::function<TestStatus()>>> tests;
		std::vector<TestResult> results; // Vector of test results
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
			tests.emplace_back(params, testFunc);
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
			results.emplace_back(status, message);
		}
	public:
		PartestBase() = default;
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

		void runTests()
		{
			// Call setup function
			setup();
			
			// Iterate through all registered tests
			for(const std::pair<const TestParams &, std::function<TestStatus()>> &test : tests)
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
		const std::vector<TestResult> &getResults() const { return results; }
	};

} // namespace partest

#endif // TEST_H