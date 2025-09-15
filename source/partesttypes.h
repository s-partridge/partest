#ifndef PARTESTTYPES_H
#define PARTESTTYPES_H

#include <iostream>
#include <string>
#include <vector>

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

		TestResult(TestStatus status = TestStatus::AWAITING, std::string testName = "", const std::string &message = "") : status(status), testName(testName), message(message) {}

		inline std::ostream &operator<<(std::ostream &out) const
		{
			out << "Test '" << testName << "' - Status: " << status << " - Message: " << message;
			return out;
		}
	};
}
#endif // PARTESTTYPES_H