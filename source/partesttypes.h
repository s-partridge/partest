#ifndef PARTESTTYPES_H
#define PARTESTTYPES_H

#include <iostream>
#include <string>
#include <vector>

#include "partestcommon.h"

namespace partest
{
	/**
	* Enum type representing the state of a test.
	*
	* AWAITING - The test has not yet started.
	* RUNNING - The test is currently running.
	* COMPLETED - The test has completed successfully.
	* ABORTED - The test was aborted due to an error or failure.
	*/
	enum TestStatus : uint8_t
	{
		AWAITING = 0,
		RUNNING,
		COMPLETED,
		ABORTED,
		SKIPPED
	};

	/**
	* Enum type representing the result of a test.
	* 
	* NO_RESULT - The test has not yet finished, or was skipped.
	* FAILED - The test failed.
	* PASSED - The test passed.
	* MIXED - The test had mixed results (some assertions passed, some failed).
	*/
	enum TestResult : uint8_t
	{
		NO_RESULT = 0,
		FAILED,
		PASSED,
		MIXED
	};

	/**
	* Enum type representing the state of a flag.
	* 
	* INHERIT - The flag state is inherited from a higher level (e.g., global or suite level).
	* ENABLED - The flag is explicitly enabled.
	* DISABLED - The flag is explicitly disabled.
	* MASKED - The flag state is not changed (used for internal purposes).
	*/
	enum FlagState : uint8_t
	{
		DISABLED = 0,
		ENABLED,
		INHERIT,
		MASKED
	};

	/**
	* Flags that can be set for individual tests
	*/
	struct TestFlags
	{
		FlagState skip : 2; // Whether to skip the test
		FlagState stopOnFail : 2; // Whether to stop execution on failure
		FlagState stopSubtestOnFail : 2; // Whether to stop subtest execution on failure
		FlagState verbose : 2; // Whether to run the test in verbose mode

		PARTEST_CONSTEXPR_11 TestFlags() noexcept : skip(DISABLED), stopOnFail(INHERIT), stopSubtestOnFail(INHERIT), verbose(INHERIT) {}
		PARTEST_CONSTEXPR_11 TestFlags(FlagState skip, FlagState stopOnFail, FlagState stopSubtestOnFail, FlagState verbose) noexcept : skip(skip), stopOnFail(stopOnFail), stopSubtestOnFail(stopSubtestOnFail), verbose(verbose) {}

		/**
		* Get a TestFlags instance with all flags set to DISABLED
		*/
		static PARTEST_CONSTEXPR_11 TestFlags defaultDisabled() noexcept { return TestFlags(DISABLED, DISABLED, DISABLED, DISABLED); }
		/**
		* Get a TestFlags instance with all flags set to INHERIT
		*/
		static PARTEST_CONSTEXPR_11 TestFlags defaultInherit() noexcept { return TestFlags(INHERIT, INHERIT, INHERIT, INHERIT); }
		
		/**
		* Get a TestFlags instance with all flags set to MASKED. Used for internal purposes.
		*/
		static PARTEST_CONSTEXPR_11 TestFlags defaultMasked() noexcept { return TestFlags(MASKED, MASKED, MASKED, MASKED); }

		/**
		* Default copy assignment operator.
		*/
		PARTEST_CONSTEXPR_20 TestFlags &operator=(const TestFlags &other) noexcept = default;

		/**
		* Set flags from another TestFlags instance, ignoring MASKED values
		* 
		* @param other The TestFlags instance to copy flags from. Expects MASKED values to be ignored.
		*/
		PARTEST_CONSTEXPR_20 void setFlags(const TestFlags &other) noexcept
		{
			if(other.skip != MASKED)
				skip = other.skip;
			if(other.stopOnFail != MASKED)
				stopOnFail = other.stopOnFail;
			if(other.stopSubtestOnFail != MASKED)
				stopSubtestOnFail = other.stopSubtestOnFail;
			if(other.verbose != MASKED)
				verbose = other.verbose;
		}

		/**
		* Get effective flags by resolving INHERIT values from parent flags
		* If a flag is set to INHERIT, it takes the value from the parentFlags instance.
		* 
		* @param parentFlags The parent TestFlags instance to inherit from
		* @return A new TestFlags instance with all INHERIT values resolved
		*/
		PARTEST_CONSTEXPR_14 TestFlags mergeWithParentFlags(const TestFlags &parentFlags) const noexcept
		{
			// Start with a copy of the current flags
			TestFlags effectiveFlags = *this;
			if(effectiveFlags.skip == INHERIT)
				effectiveFlags.skip = parentFlags.skip;
			if(effectiveFlags.stopOnFail == INHERIT)
				effectiveFlags.stopOnFail = parentFlags.stopOnFail;
			if(effectiveFlags.stopSubtestOnFail == INHERIT)
				effectiveFlags.stopSubtestOnFail = parentFlags.stopSubtestOnFail;
			if(effectiveFlags.verbose == INHERIT)
				effectiveFlags.verbose = parentFlags.verbose;
			return effectiveFlags;
		}

		/**
		* Check if all flags are resolved (i.e., none are set to INHERIT or MASKED)
		*/
		PARTEST_CONSTEXPR_11 bool isResolved() const noexcept
		{
			return skip < INHERIT && stopOnFail < INHERIT && stopSubtestOnFail < INHERIT && verbose < INHERIT;
		}
	};

	/**
	* Parameters passed to an individual test
	*/
	struct TestInfo
	{
		std::string name; // Name of the test
		std::string description; // Description of the test
		PARTEST_CONSTEXPR_20 TestInfo() : name(""), description("") {}
		PARTEST_CONSTEXPR_20 TestInfo(PARTEST_STRING_PARAM name, PARTEST_STRING_PARAM description = "") : name(name), description(description) {}

		/**
		* Get a TestInfo instance with default (empty) values
		*/
		static PARTEST_CONSTEXPR_20 TestInfo defaultInfo() { return TestInfo("", ""); }
	};

	/**
	* Struct representing the result of a test.
	* 
	* status - The status of the test.
	* message - A message providing additional information about the test result.
	*/
	class TestState
	{
		TestStatus m_status; // Status of the test
		TestResult m_result; // Result of the test
	public:
		// Constructors
		PARTEST_CONSTEXPR_11 TestState() noexcept : m_status(AWAITING), m_result(NO_RESULT) {}
		PARTEST_CONSTEXPR_11 TestState(TestStatus status) noexcept : m_status(status), m_result(NO_RESULT) {}

		/**
		* Get a TestResult instance with default values (AWAITING status and empty message)
		*/
		static PARTEST_CONSTEXPR_11 TestState defaultState() noexcept { return TestState(AWAITING); }

		PARTEST_CONSTEXPR_11 TestStatus getStatus() const noexcept { return m_status; }
		PARTEST_CONSTEXPR_11 TestResult getResult() const noexcept { return m_result; }

		/**
		* 
		*/
		PARTEST_CONSTEXPR_20 void updateStatus(const TestStatus &status) noexcept { m_status = status; }

		/**
		* Update the test result based on a new assertion result.
		* @param assertResult The result of the new assertion to incorporate into the test result.
		*/
		PARTEST_CONSTEXPR_20 void updateResult(const TestResult &assertResult) noexcept
		{
			switch(assertResult)
			{
			case NO_RESULT:
				m_result = NO_RESULT;
				break;
			case PASSED:
				if(m_result == NO_RESULT)
					m_result = PASSED;
				else if(m_result == FAILED)
					m_result = MIXED;
				// MIXED remains unchanged
				break;
			case FAILED:
				if(m_result == NO_RESULT)
					m_result = FAILED;
				else if(m_result == PASSED)
					m_result = MIXED;
				// MIXED remains unchanged
				break;
			case MIXED:
				m_result = MIXED;
				break;
			default:
				m_result = NO_RESULT;
			}
		}

		friend std::ostream &operator<<(std::ostream &out, const TestState &state);
	};

	PARTEST_CONSTEXPR_20 std::string to_string(const TestStatus &status)
	{
		switch(status)
		{
		case AWAITING:
			return "AWAITING";
		case RUNNING:
			return "RUNNING";
		case COMPLETED:
			return "COMPLETED";
		case ABORTED:
			return "ABORTED";
		case SKIPPED:
			return "SKIPPED";
		default:
			return "INVALID STATUS VALUE";
		}
	}

	PARTEST_CONSTEXPR_20 std::string to_string(const TestResult &result)
	{
		switch(result)
		{
		case NO_RESULT:
			return "NO_RESULT";
		case PASSED:
			return "PASSED";
		case FAILED:
			return "FAILED";
		case MIXED:
			return "MIXED";
		default:
			return "INVALID RESULT VALUE";
		}
	}

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
		else if(statusString == "COMPLETED")
			status = COMPLETED;
		else if(statusString == "ABORTED")
			status = ABORTED;
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
			statusString = "COMPLETED";
			break;
		case ABORTED:
			statusString = "ABORTED";
			break;
		case SKIPPED:
			statusString = "SKIPPED";
			break;
		default:
			statusString = "INVALID STATUS VALUE";
		}
		out << statusString;
		return out;
	}

	/**
	* Overloaded stream extraction operator for TestResult enum.
	*/
	inline std::istream &operator>>(std::istream &in, TestResult &result)
	{
		std::string statusString;
		in >> statusString;
		if(statusString	== "NO_RESULT")
			result = NO_RESULT;
		else if(statusString == "PASSED")
			result = PASSED;
		else if(statusString == "FAILED")
			result = FAILED;
		else if(statusString == "MIXED")
			result = MIXED;
		else
			result = NO_RESULT; // Default to NO_RESULT for unknown strings
		return in;
	}

	/**
	* Overloaded stream insertion operator for TestResult enum.
	*/
	inline std::ostream &operator<<(std::ostream &out, const TestResult &result)
	{
		std::string statusString;
		switch(result)
		{
		case NO_RESULT:
			statusString = "NO_RESULT";
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
		default:
			statusString = "INVALID RESULT VALUE";
		}
		out << statusString;
		return out;
	}

	/**
	* Overloaded stream extraction operator for FlagState enum.
	*/
	inline std::istream &operator>>(std::istream &in, FlagState &state)
	{
		std::string stateString;
		in >> stateString;
		if(stateString == "INHERIT")
			state = INHERIT;
		else if(stateString == "ENABLED")
			state = ENABLED;
		else if(stateString == "DISABLED")
			state = DISABLED;
		else if(stateString == "MASKED")
			state = MASKED;
		else
			state = INHERIT; // Default to INHERIT for unknown strings
		return in;
	}

	/**
	* Overloaded stream insertion operator for FlagState enum.
	*/
	inline std::ostream &operator<<(std::ostream &out, const FlagState &state)
	{
		std::string stateString;
		switch(state)
		{
		case INHERIT:
			out << "INHERIT";
			break;
		case ENABLED:
			out << "ENABLED";
			break;
		case DISABLED:
			out << "DISABLED";
			break;
		case MASKED:
			out << "MASKED";
			break;
		default:
			out << "INVALID FLAG VALUE";
		}
		return out;
	}

	/**
	* Overloaded stream extraction operator for TestState.
	*/
	inline std::ostream &operator<<(std::ostream &out, const TestState &state)
	{
		out << "Status: " << state.m_status << " - Result: " << state.m_result;
		return out;
	}

	/**
	* Default flag constants for easy reference
	*/
	// Disable all flags
	PARTEST_CONSTEXPR_11 const TestFlags TEST_FLAGS_DISABLED = TestFlags(FlagState::DISABLED, FlagState::DISABLED, FlagState::DISABLED, FlagState::DISABLED);
	// Inherit all flags from parent test
	PARTEST_CONSTEXPR_11 const TestFlags TEST_FLAGS_INHERIT = TestFlags(FlagState::INHERIT, FlagState::INHERIT, FlagState::INHERIT, FlagState::INHERIT);
	// Mask all flags (used for internal purposes)
	PARTEST_CONSTEXPR_11 const TestFlags TEST_FLAGS_MASKED = TestFlags(FlagState::MASKED, FlagState::MASKED, FlagState::MASKED, FlagState::MASKED);
	// Set flags to skip the test
	PARTEST_CONSTEXPR_11 const TestFlags TEST_FLAGS_SKIP = TestFlags(FlagState::ENABLED, FlagState::INHERIT, FlagState::INHERIT, FlagState::INHERIT);
}
#endif // PARTESTTYPES_H