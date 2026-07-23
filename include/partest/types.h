#ifndef PARTEST_TYPES_H
#define PARTEST_TYPES_H

#include <iostream>
#include <cstdint>
#include <string>

#include <partest/common.h>

namespace partest
{
	/**
	* Enum type representing the state of a test.
	*
	* Awaiting - The test has not yet started.
	* Running - The test is currently running.
	* Completed - The test has completed successfully.
	* Aborted - The test was aborted due to an error or failure.
	*/
	enum class TestStatus : uint8_t
	{
		Awaiting = 0,
		Running,
		Completed,
		Aborted,
		Skipped
	};

	/**
	* Enum type representing the result of a test.
	* 
	* NoResult - The test has not yet finished, or was skipped.
	* Failed - The test failed.
	* Passed - The test passed.
	* Mixed - The test had mixed results (some assertions passed, some failed).
	*/
	enum class TestResult : uint8_t
	{
		NoResult = 0,
		Failed,
		Passed,
		Mixed
	};

	/**
	* Enum type representing the state of a flag.
	* 
	* Inherit - The flag state is inherited from a higher level (e.g., global or suite level).
	* Enabled - The flag is explicitly enabled.
	* Disabled - The flag is explicitly disabled.
	* Masked - The flag state is not changed (used for internal purposes).
	*/
	enum class FlagState : uint8_t
	{
		Disabled = 0,
		Enabled,
		Inherit,
		Masked
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

		PARTEST_CONSTEXPR_11 TestFlags() noexcept : skip(FlagState::Disabled), stopOnFail(FlagState::Inherit), stopSubtestOnFail(FlagState::Inherit), verbose(FlagState::Inherit) {}
		PARTEST_CONSTEXPR_11 TestFlags(FlagState skip, FlagState stopOnFail, FlagState stopSubtestOnFail, FlagState verbose) noexcept : skip(skip), stopOnFail(stopOnFail), stopSubtestOnFail(stopSubtestOnFail), verbose(verbose) {}

		/**
		* Get a TestFlags instance with all flags set to Disabled
		*/
		static PARTEST_CONSTEXPR_11 TestFlags defaultDisabled() noexcept { return TestFlags(FlagState::Disabled, FlagState::Disabled, FlagState::Disabled, FlagState::Disabled); }
		/**
		* Get a TestFlags instance with all flags set to Inherit
		*/
		static PARTEST_CONSTEXPR_11 TestFlags defaultInherit() noexcept { return TestFlags(FlagState::Inherit, FlagState::Inherit, FlagState::Inherit, FlagState::Inherit); }

		/**
		* Get a TestFlags instance with all flags set to Masked. Used for internal purposes.
		*/
		static PARTEST_CONSTEXPR_11 TestFlags defaultMasked() noexcept { return TestFlags(FlagState::Masked, FlagState::Masked, FlagState::Masked, FlagState::Masked); }

		/**
		* Get a TestFlags instance with skip = true, all other flags set to Disabled
		*/
		static PARTEST_CONSTEXPR_11 TestFlags defaultSkip() noexcept { return TestFlags(FlagState::Enabled, FlagState::Disabled, FlagState::Disabled, FlagState::Disabled); }

		/**
		* Default copy assignment operator.
		*/
		PARTEST_CONSTEXPR_14 TestFlags &operator=(const TestFlags &other) noexcept = default;

		/**
		* Set flags from another TestFlags instance, ignoring Masked values
		* 
		* @param other The TestFlags instance to copy flags from. Expects Masked values to be ignored.
		*/
		PARTEST_CONSTEXPR_14 void setFlags(const TestFlags &other) noexcept
		{
			if(other.skip != FlagState::Masked)
				skip = other.skip;
			if(other.stopOnFail != FlagState::Masked)
				stopOnFail = other.stopOnFail;
			if(other.stopSubtestOnFail != FlagState::Masked)
				stopSubtestOnFail = other.stopSubtestOnFail;
			if(other.verbose != FlagState::Masked)
				verbose = other.verbose;
		}

		/**
		* Get effective flags by resolving Inherit values from parent flags
		* If a flag is set to Inherit, it takes the value from the parentFlags instance.
		* 
		* @param parentFlags The parent TestFlags instance to inherit from
		* @return A new TestFlags instance with all Inherit values resolved
		*/
		PARTEST_CONSTEXPR_14 TestFlags mergeWithParentFlags(const TestFlags &parentFlags) const noexcept
		{
			// Start with a copy of the current flags
			TestFlags effectiveFlags = *this;
			if(effectiveFlags.skip == FlagState::Inherit)
				effectiveFlags.skip = parentFlags.skip;
			if(effectiveFlags.stopOnFail == FlagState::Inherit)
				effectiveFlags.stopOnFail = parentFlags.stopOnFail;
			if(effectiveFlags.stopSubtestOnFail == FlagState::Inherit)
				effectiveFlags.stopSubtestOnFail = parentFlags.stopSubtestOnFail;
			if(effectiveFlags.verbose == FlagState::Inherit)
				effectiveFlags.verbose = parentFlags.verbose;
			return effectiveFlags;
		}

		/**
		* Check if all flags are resolved (i.e., none are set to Inherit or Masked)
		*/
		PARTEST_CONSTEXPR_11 bool isResolved() const noexcept
		{
			return skip < FlagState::Inherit && stopOnFail < FlagState::Inherit && stopSubtestOnFail < FlagState::Inherit && verbose < FlagState::Inherit;
		}
	};

	/**
	* Parameters passed to an individual test
	*/
	struct TestInfo
	{
		std::string name; // Name of the test
		std::string description; // Description of the test
		PARTEST_CONSTEXPR_20 TestInfo() : name(), description() {}
		PARTEST_CONSTEXPR_20 TestInfo(PARTEST_STRING_PARAM name, PARTEST_STRING_PARAM description = "") : name(name), description(description) {}

		/**
		* Get a TestInfo instance with default (empty) values
		*/
		static PARTEST_CONSTEXPR_20 TestInfo defaultInfo() { return TestInfo(); }
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
		PARTEST_CONSTEXPR_11 TestState() noexcept : m_status(TestStatus::Awaiting), m_result(TestResult::NoResult) {}
		PARTEST_CONSTEXPR_11 TestState(TestStatus status) noexcept : m_status(status), m_result(TestResult::NoResult) {}

		/**
		* Get a TestResult instance with default values (Awaiting status and empty message)
		*/
		static PARTEST_CONSTEXPR_11 TestState defaultState() noexcept { return TestState(TestStatus::Awaiting); }

		/**
		* Convenience function to get the current status of the test.
		* 
		* @return The current TestStatus of the test.
		*/
		PARTEST_CONSTEXPR_11 TestStatus getStatus() const noexcept { return m_status; }
		/**
		* Convenience function to get the current result of the test.
		* 
		* @return The current TestResult of the test.
		*/
		PARTEST_CONSTEXPR_11 TestResult getResult() const noexcept { return m_result; }
		
		/**
		* Check whether the test has completed.
		* 
		* @return true if the test has completed, false otherwise.
		*/
		PARTEST_CONSTEXPR_11 bool hasFinishedRunning() const noexcept { return m_status == TestStatus::Completed || m_status == TestStatus::Aborted; }
		/**
		* Check whether the test has failed or has mixed results (some assertions passed, some failed).
		* 
		* @return true if the test has failed or has mixed results, false otherwise.
		*/
		PARTEST_CONSTEXPR_11 bool hasFailures() const noexcept { return m_result == TestResult::Failed || m_result == TestResult::Mixed; }

		/**
		* Update the test status.
		* @param status The new status to set for the test.
		*/
		PARTEST_CONSTEXPR_14 void updateStatus(const TestStatus &status) noexcept { m_status = status; }

		/**
		* Update the test result based on a new assertion result.
		* @param assertResult The result of the new assertion to incorporate into the test result.
		*/
		PARTEST_CONSTEXPR_14 void updateResult(const TestResult &assertResult) noexcept
		{
			switch(assertResult)
			{
			case TestResult::NoResult:
				m_result = TestResult::NoResult;
				break;
			case TestResult::Passed:
				if(m_result == TestResult::NoResult)
					m_result = TestResult::Passed;
				else if(m_result == TestResult::Failed)
					m_result = TestResult::Mixed;
				// Mixed remains unchanged
				break;
			case TestResult::Failed:
				if(m_result == TestResult::NoResult)
					m_result = TestResult::Failed;
				else if(m_result == TestResult::Passed)
					m_result = TestResult::Mixed;
				// Mixed remains unchanged
				break;
			case TestResult::Mixed:
				m_result = TestResult::Mixed;
				break;
			default:
				m_result = TestResult::NoResult;
			}
		}

		friend std::ostream &operator<<(std::ostream &out, const TestState &state);
	};

	inline PARTEST_CONSTEXPR_14 const char* to_string(const TestStatus &status)
	{
		switch(status)
		{
		case TestStatus::Awaiting:
			return "AWAITING";
		case TestStatus::Running:
			return "RUNNING";
		case TestStatus::Completed:
			return "COMPLETED";
		case TestStatus::Aborted:
			return "ABORTED";
		case TestStatus::Skipped:
			return "SKIPPED";
		default:
			return "INVALID STATUS VALUE";
		}
	}

	inline PARTEST_CONSTEXPR_14 const char* to_string(const TestResult &result)
	{
		switch(result)
		{
		case TestResult::NoResult:
			return "NO_RESULT";
		case TestResult::Passed:
			return "PASSED";
		case TestResult::Failed:
			return "FAILED";
		case TestResult::Mixed:
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
			status = TestStatus::Awaiting;
		else if(statusString == "RUNNING")
			status = TestStatus::Running;
		else if(statusString == "COMPLETED")
			status = TestStatus::Completed;
		else if(statusString == "ABORTED")
			status = TestStatus::Aborted;
		else if(statusString == "SKIPPED")
			status = TestStatus::Skipped;
		else
			status = TestStatus::Awaiting; // Default to Awaiting for unknown strings
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
		case TestStatus::Awaiting:
			statusString = "AWAITING";
			break;
		case TestStatus::Running:
			statusString = "RUNNING";
			break;
		case TestStatus::Completed:
			statusString = "COMPLETED";
			break;
		case TestStatus::Aborted:
			statusString = "ABORTED";
			break;
		case TestStatus::Skipped:
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
			result = TestResult::NoResult;
		else if(statusString == "PASSED")
			result = TestResult::Passed;
		else if(statusString == "FAILED")
			result = TestResult::Failed;
		else if(statusString == "MIXED")
			result = TestResult::Mixed;
		else
			result = TestResult::NoResult; // Default to NoResult for unknown strings
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
		case TestResult::NoResult:
			statusString = "NO_RESULT";
			break;
		case TestResult::Passed:
			statusString = "PASSED";
			break;
		case TestResult::Failed:
			statusString = "FAILED";
			break;
		case TestResult::Mixed:
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
			state = FlagState::Inherit;
		else if(stateString == "ENABLED")
			state = FlagState::Enabled;
		else if(stateString == "DISABLED")
			state = FlagState::Disabled;
		else if(stateString == "MASKED")
			state = FlagState::Masked;
		else
			state = FlagState::Inherit; // Default to Inherit for unknown strings
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
		case FlagState::Inherit:
			out << "INHERIT";
			break;
		case FlagState::Enabled:
			out << "ENABLED";
			break;
		case FlagState::Disabled:
			out << "DISABLED";
			break;
		case FlagState::Masked:
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
	PARTEST_CONSTEXPR_11 const TestFlags TEST_FLAGS_DISABLED = TestFlags::defaultDisabled();
	// Inherit all flags from parent test
	PARTEST_CONSTEXPR_11 const TestFlags TEST_FLAGS_INHERIT = TestFlags::defaultInherit();
	// Mask all flags (used for internal purposes)
	PARTEST_CONSTEXPR_11 const TestFlags TEST_FLAGS_MASKED = TestFlags::defaultMasked();
	// Set flags to skip the test
	PARTEST_CONSTEXPR_11 const TestFlags TEST_FLAGS_SKIP = TestFlags::defaultSkip();
}
#endif // PARTESTTYPES_H