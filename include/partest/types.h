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
		SettingUp,
		Running,
		TearingDown,
		Completed,
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

	enum class TestOutcome : uint8_t
	{
		NoResult = 0,
		Failed,
		Passed,
		Mixed,
		ExpectedFailure,
		UnexpectedPass,
		Aborted,
		Skipped
	};

	// TestOutcome and TestResult are semantically related but carry different information.
	// TestOutcome represents the presentation of the test result to the user, while TestResult represents the actual result of the test.
	// For simplicity of implementation, the ordering of the enum values is the same, so that a simple cast can be used to convert between them.
	static_assert(static_cast<uint8_t>(TestOutcome::NoResult) == static_cast<uint8_t>(TestResult::NoResult), "TestOutcome and TestResult enum values must match");
	static_assert(static_cast<uint8_t>(TestOutcome::Mixed) == static_cast<uint8_t>(TestResult::Mixed), "TestOutcome and TestResult enum values must match");
	static_assert(static_cast<uint8_t>(TestOutcome::Failed) == static_cast<uint8_t>(TestResult::Failed), "TestOutcome and TestResult enum values must match");
	static_assert(static_cast<uint8_t>(TestOutcome::Passed) == static_cast<uint8_t>(TestResult::Passed), "TestOutcome and TestResult enum values must match");

	/**
	* Enum type representing the mode of failure for a test, if there is one.
	*/
	enum class FailureMode : uint8_t
	{
		None = 0,
		FrameworkOutOfMemory,
		UserOutOfMemory,
		NoTestFunction,
		DanglingThread,
		Exception,
		Timeout,
		KilledByParent
	};

	
	/**
	* Enum class representing the source of a failed memory allocation, used in concert with FailureMode::OutOfMemory
	*/
	enum class BadAllocSource : uint8_t
	{
		Unknown = 0,
		TestCreation = 1,
		TestPreInitialization = 2,
		TestInitialization = 3,
		TestExecution = 4,
		TestFinalization = 5,
		AssertionHandling = 6,
		LogRecording = 7,
		UserMessageForwarding = 8,
		TestInfoUpdating = 9,
		DesyncedTestState = 10
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
#if defined(_MSC_VER)
    #pragma warning(push)
    #pragma warning(disable : 4201) // nonstandard extension used: nameless struct/union
#elif defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpedantic"
#endif
		union
		{
			uint16_t raw;			// For unified access
			struct
			{
				FlagState skip : 2; // Whether to skip the test
				FlagState stopOnFail : 2; // Whether to stop execution on failure
				FlagState stopSubtestOnFail : 2; // Whether to stop subtest execution on failure
				FlagState expectFailure : 2; // Whether to stop subtest execution on failure
				FlagState verbose : 2; // Whether to run the test in verbose mode
				uint8_t padding : 6; // Padding bits to fill out unused space
			};
		};
#if defined(_MSC_VER)
    #pragma warning(pop)
#elif defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic pop
#endif

		PARTEST_CONSTEXPR_11 TestFlags() noexcept
			: skip(FlagState::Inherit), stopOnFail(FlagState::Inherit),
			  stopSubtestOnFail(FlagState::Inherit), expectFailure(FlagState::Disabled),
			  verbose(FlagState::Inherit), padding(0) {}
		PARTEST_CONSTEXPR_11 TestFlags(
			FlagState skip, FlagState stopOnFail,
			FlagState stopSubtestOnFail, FlagState expectFailure,
			FlagState verbose) noexcept
			: skip(skip), stopOnFail(stopOnFail),
			  stopSubtestOnFail(stopSubtestOnFail), expectFailure(expectFailure),
			  verbose(verbose), padding(0) {}

		/**
		* Get a TestFlags instance with all flags set to Disabled
		*/
		static PARTEST_CONSTEXPR_11 TestFlags defaultDisabled() noexcept { return TestFlags(FlagState::Disabled, FlagState::Disabled, FlagState::Disabled, FlagState::Disabled, FlagState::Disabled); }
		/**
		* Get a TestFlags instance with all flags set to Inherit
		*/
		static PARTEST_CONSTEXPR_11 TestFlags defaultInherit() noexcept { return TestFlags(FlagState::Inherit, FlagState::Inherit, FlagState::Inherit, FlagState::Disabled, FlagState::Inherit); }

		/**
		* Get a TestFlags instance with all flags set to Masked. Used for internal purposes.
		*/
		static PARTEST_CONSTEXPR_11 TestFlags defaultMasked() noexcept { return TestFlags(FlagState::Masked, FlagState::Masked, FlagState::Masked, FlagState::Masked, FlagState::Masked); }

		/**
		* Get a TestFlags instance with skip = true, all other flags set to Disabled
		*/
		static PARTEST_CONSTEXPR_11 TestFlags defaultSkip() noexcept { return TestFlags(FlagState::Enabled, FlagState::Disabled, FlagState::Disabled, FlagState::Disabled, FlagState::Disabled); }

		/**
		* Get a copy of the flag set with stopOnFail set
		* 
		* @pararm enabled The new value for stopOnFail. Defaults to `Enabled`
		* @returns a copy of the current set of flags, with stopOnFail explicitly set
		*/
		PARTEST_CONSTEXPR_14 TestFlags withStopOnFail(FlagState enabled = FlagState::Enabled) const noexcept
		{
			TestFlags newFlags = *this;
			newFlags.stopOnFail = enabled;
			return newFlags;
		}

		/**
		* Get a copy of the flag set with expectFailure set
		* 
		* @pararm enabled The new value for expectFailure. Defaults to `Enabled`
		* @returns a copy of the current set of flags, with expectFailure explicitly set
		*/
		PARTEST_CONSTEXPR_14 TestFlags withExpectFailure(FlagState enabled = FlagState::Enabled) const noexcept
		{
			TestFlags newFlags = *this;
			newFlags.expectFailure = enabled;
			return newFlags;
		}

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
			if(other.expectFailure != FlagState::Masked)
				expectFailure = other.expectFailure;
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
			if(effectiveFlags.expectFailure == FlagState::Inherit)
				effectiveFlags.expectFailure = parentFlags.expectFailure;
			if(effectiveFlags.verbose == FlagState::Inherit)
				effectiveFlags.verbose = parentFlags.verbose;
			return effectiveFlags;
		}

		/**
		* Check if all flags are resolved (i.e., none are set to Inherit or Masked)
		*/
		PARTEST_CONSTEXPR_11 bool isResolved() const noexcept
		{
			// Each flag is two bits wide. The upper bit of each flag is one 1 when it has not been resolved to Enabled/Disabled.
			// Bitwise `and` against a mask of 0b10 on each flag validates that the upper bit is unset.
			// Equivalent 
			return (raw & 0xAAAA) == 0;
			// Valid values are disabled, enabled, inherit, and masked, beginning at `disabled = 0`. Any value greater than `enabled` is not resolved.
			/*return skip < FlagState::Inherit
				&& stopOnFail < FlagState::Inherit
				&& stopSubtestOnFail < FlagState::Inherit
				&& expectFailure < FlagState::Inherit
				&& verbose < FlagState::Inherit;*/			
		}
	};

	/**
	* Equality operators for TestFlags
	*/
	inline PARTEST_CONSTEXPR_11 bool operator==(const TestFlags& lhs, const TestFlags& rhs)
	{
		return lhs.raw == rhs.raw;
	}

	inline PARTEST_CONSTEXPR_11 bool operator!=(const TestFlags& lhs, const TestFlags& rhs)
	{
		return lhs.raw != rhs.raw;
	}

	/**
	* Parameters passed to an individual test
	*/
	struct TestInfo
	{
		std::string name; // Name of the test
		std::string description; // Description of the test
		std::string file; // File where the test is defined
		unsigned int line; // Line number where the test is defined
		PARTEST_CONSTEXPR_20 TestInfo() : name(), description(), file(), line(0) {}
		PARTEST_CONSTEXPR_20 TestInfo(PARTEST_STRING_PARAM name, PARTEST_STRING_PARAM description = "", PARTEST_STRING_PARAM file = "", unsigned int line = 0) : name(name), description(description), file(file), line(line) {}

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
		FailureMode m_failureMode; // Mode of failure, if any
		BadAllocSource m_badAllocSource; // Source of bad allocation, if any
		bool m_expectFailure;
	public:
		// Constructors
		PARTEST_CONSTEXPR_11 TestState(bool expectFailure = false) noexcept : m_status(TestStatus::Awaiting), m_result(TestResult::NoResult), m_failureMode(FailureMode::None), m_badAllocSource(BadAllocSource::Unknown), m_expectFailure(expectFailure) {}
		PARTEST_CONSTEXPR_11 TestState(TestStatus status, bool expectFailure = false) noexcept : m_status(status), m_result(TestResult::NoResult), m_failureMode(FailureMode::None), m_badAllocSource(BadAllocSource::Unknown), m_expectFailure(expectFailure) {}
		/**
		* Get a TestResult instance with default values (Awaiting status and empty message)
		*/
		static PARTEST_CONSTEXPR_11 TestState defaultState(bool expectFailure = false) noexcept { return TestState(TestStatus::Awaiting, expectFailure); }

		PARTEST_CONSTEXPR_11 TestStatus getStatus() const noexcept { return m_status; }
		PARTEST_CONSTEXPR_11 FailureMode getFailureMode() const noexcept { return m_failureMode; }
		PARTEST_CONSTEXPR_11 BadAllocSource getBadAllocSource() const noexcept { return m_badAllocSource; }
		PARTEST_CONSTEXPR_11 bool getExpectFailure() const noexcept { return m_expectFailure; }

		/**
		* Get the effective result of the test, considering whether expectFailure is set.
		* 
		* If expectFailure is set, expected values include ExpectedFailure and UnexpectedPass. If expectFailure is not set, returns Passed, Failed, Mixed, or NoResult.
		* 
		* @return The effective TestOutcome
		*/
		PARTEST_CONSTEXPR_14 TestOutcome getEffectiveResult() const noexcept
		{
			if(wasSkipped())
				return TestOutcome::Skipped;

			if(isAborting() || hasBeenAborted())
				return TestOutcome::Aborted;

			if(m_expectFailure)
			{
				switch(m_result)
				{
				case TestResult::NoResult:
					return TestOutcome::NoResult;
				case TestResult::Failed:
					return TestOutcome::ExpectedFailure;
				case TestResult::Passed:
					return TestOutcome::UnexpectedPass;
				case TestResult::Mixed:
					return TestOutcome::ExpectedFailure;
				}
			}

			return static_cast<TestOutcome>(m_result);
		}
		
		/**
		* Check whether the test has started. This will be true from the moment the test is invoked onward.
		* 
		* @return true if the test has started, false otherwise.
		*/
		PARTEST_CONSTEXPR_11 bool hasTestStarted() const noexcept { return m_status != TestStatus::Awaiting; }

		/**
		* Check whether the test is currently running. This should be true from the moment the test function itself is invoked, only until the test function returns, and does not include setup or teardown phases.
		* 
		* @return true if the test is currently running, false otherwise.
		*/
		PARTEST_CONSTEXPR_11 bool isRunning() const noexcept { return m_status == TestStatus::Running; }

		/**
		* Check whether the test is in any version of active, from setting up to tearing down or aborting. This is more comprehensive than isRunning(), which only checks for the Running state.
		* 
		* @return true if the test is in progress, false otherwise.
		*/
		PARTEST_CONSTEXPR_11 bool isInProgress() const noexcept { return m_status == TestStatus::SettingUp || m_status == TestStatus::Running || m_status == TestStatus::TearingDown; }

		/**
		* Check whether the test is in the process of deconstructing.
		*
		* @return true if the test status is TearingDown, false otherwise.
		*/
		PARTEST_CONSTEXPR_11 bool isDeconstructing() const noexcept { return m_status == TestStatus::TearingDown; }

		/**
		* Check whether the test is in the process of aborting due to a failure mode.
		*
		* @return true of the test status is TearingDown and the failure mode is not None, false otherwise.
		*/
		PARTEST_CONSTEXPR_11 bool isAborting() const noexcept { return m_status == TestStatus::TearingDown && m_failureMode != FailureMode::None; }

		/**
		* Check whether the test has completed.
		* 
		* @return true if the test has completed, false otherwise.
		*/
		PARTEST_CONSTEXPR_11 bool hasFinishedRunning() const noexcept { return m_status == TestStatus::Completed; }

		/**
		* Check whether the test has been aborted.
		*
		* @return true if the test status is Completed and the failure mode is not None, false otherwise.
		*/
		PARTEST_CONSTEXPR_11 bool hasBeenAborted() const noexcept { return m_status == TestStatus::Completed && m_failureMode != FailureMode::None; }

		/**
		* Check whether the test has passed.
		* 
		* @return true if the test has passed, false otherwise.
		*/
		PARTEST_CONSTEXPR_14 bool passed() const noexcept
		{
			TestOutcome effectiveResult = getEffectiveResult();
			return effectiveResult == TestOutcome::Passed
				|| effectiveResult == TestOutcome::ExpectedFailure;
		}

		/**
		* Check whether the test has failed or has mixed results (some assertions passed, some failed).
		* This will also return true if the test was aborted due to a failure mode (e.g., out of memory, exception, etc.).
		* 
		* @return true if the test has failed or has mixed results, or was aborted, false otherwise.
		*/
		PARTEST_CONSTEXPR_14 bool hasFailures() const noexcept
		{
			TestOutcome effectiveResult = getEffectiveResult();

			return effectiveResult == TestOutcome::Failed
				|| effectiveResult == TestOutcome::Mixed
				|| effectiveResult == TestOutcome::UnexpectedPass
				|| effectiveResult == TestOutcome::Aborted;
		}

		/**
		* Check whether the test passed when expectFailure was set
		* 
		* @returns true if the test passed unexpectedly, false otherwise
		*/
		PARTEST_CONSTEXPR_14 bool didPassUnexpectedly() const noexcept { return getEffectiveResult() == TestOutcome::UnexpectedPass; }

		/**
		* Check whether the test was skipped.
		* 
		* @return true if the test was skipped, false otherwise.
		*/
		PARTEST_CONSTEXPR_11 bool wasSkipped() const noexcept { return m_status == TestStatus::Skipped; }

		/**
		* Update the test status.
		* Will not update the status if the test is currently aborting or has been aborted, unless the new status is TearingDown or Completed.
		* 
		* @param status The new status to set for the test.
		* @return true if the status was updated, false otherwise.
		*/
		PARTEST_CONSTEXPR_14 bool updateStatus(const TestStatus &status) noexcept
		{
			if(status >= TestStatus::TearingDown || (!isAborting() && !hasBeenAborted()))
			{
				m_status = status;
				return true;
			}
			return false;
		}

		PARTEST_CONSTEXPR_14 void updateResultFromAssertion(bool passed) noexcept
		{
			switch(m_result)
			{
			case TestResult::NoResult:
				m_result = passed ? TestResult::Passed : TestResult::Failed;
				break;
			case TestResult::Failed:
				m_result = passed ? TestResult::Mixed : TestResult::Failed;
				break;
			case TestResult::Passed:
				m_result = passed ? TestResult::Passed : TestResult::Mixed;
				break;
			case TestResult::Mixed:
				break;
			// This state should never be reached.
			default:
				break;
			}
		}

		PARTEST_CONSTEXPR_14 void updateResult(const TestResult &result) noexcept
		{
			switch(m_result)
			{
			case TestResult::NoResult:
				if(result == TestResult::Passed)
					m_result = TestResult::Passed;
				else if(result == TestResult::Failed || result == TestResult::Mixed)
					m_result = result;
				// NoResult does nothing here
				break;
			case TestResult::Passed:
				if(result == TestResult::Failed || result == TestResult::Mixed)
					m_result = TestResult::Mixed;
				// Passed and NoResult do nothing here
				break;
			case TestResult::Mixed:
				// No future result further alters a mixed state
				break;
			case TestResult::Failed:
				if(result == TestResult::Passed || result == TestResult::Mixed)
					m_result = TestResult::Mixed;
				else if(result == TestResult::Failed)
					m_result = TestResult::Failed;
				// NoResult does nothing here
				break;
			// This state should never be reached
			default:
				break;
			}
		}

		PARTEST_CONSTEXPR_14 void updateFailureMode(FailureMode mode, BadAllocSource source = BadAllocSource::Unknown) noexcept
		{
			m_failureMode = mode;
			m_badAllocSource = source;
		}

		/**
		* Update the test result based on a new assertion result. Skipped tests have no results and are ignored.
		* Aborted tests are always considered failed, regardless of whether ExpectFailure is set.
		* Where ExpectFailure is set, the result of an aborted test has no meaning.
		* 
		* @param assertResult The result of the new assertion to incorporate into the test result.
		*/
		PARTEST_CONSTEXPR_14 void updateFromSubtestState(const TestState &subtestState) noexcept
		{
			if(subtestState.wasSkipped())
			{
				return;
			}
			else if(subtestState.isAborting() || subtestState.hasBeenAborted())
			{
				updateResult(TestResult::Failed);
			}
			else if(subtestState.m_expectFailure)
			{
				switch(subtestState.m_result)
				{
				case TestResult::NoResult:
					break;
				case TestResult::Failed:
					updateResult(TestResult::Passed);
					break;
				case TestResult::Passed:
					updateResult(TestResult::Failed);
					break;
				case TestResult::Mixed:
					// Mixed results in a test with expectFailure set indicate that it failed as expected, so we treat it as a pass for the parent test
					updateResult(TestResult::Passed);
					break;
				}
			}
			else
			{
				updateResult(subtestState.m_result);
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
		case TestStatus::SettingUp:
			return "SETTING_UP";
		case TestStatus::Running:
			return "RUNNING";
		case TestStatus::TearingDown:
			return "TEARING_DOWN";
		case TestStatus::Completed:
			return "COMPLETED";
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
			return "INVALID_RESULT_VALUE";
		}
	}

	inline PARTEST_CONSTEXPR_14 const char* to_string(const FailureMode &mode)
	{
		switch(mode)
		{
		case FailureMode::None:
			return "NONE";
		case FailureMode::NoTestFunction:
			return "NO_TEST_FUNCTION";
		case FailureMode::FrameworkOutOfMemory:
			return "FRAMEWORK_OUT_OF_MEMORY";
		case FailureMode::UserOutOfMemory:
			return "USER_OUT_OF_MEMORY";
		case FailureMode::Exception:
			return "EXCEPTION";
		case FailureMode::Timeout:
			return "TIMEOUT";
		case FailureMode::KilledByParent:
			return "KILLED_BY_PARENT";
		case FailureMode::DanglingThread:
			return "DANGLING_THREAD";
		default:
			return "INVALID_FAILURE_MODE";
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
		else if(statusString == "SETTING_UP")
			status = TestStatus::SettingUp;
		else if(statusString == "RUNNING")
			status = TestStatus::Running;
		else if(statusString == "TEARING_DOWN")
			status = TestStatus::TearingDown;
		else if(statusString == "COMPLETED")
			status = TestStatus::Completed;
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
		case TestStatus::SettingUp:
			statusString = "SETTING_UP";
			break;
		case TestStatus::Running:
			statusString = "RUNNING";
			break;
		case TestStatus::TearingDown:
			statusString = "TEARING_DOWN";
			break;
		case TestStatus::Completed:
			statusString = "COMPLETED";
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
	* Overloaded stream insertion operator for FailureMode enum.
	*/
	inline std::ostream &operator<<(std::ostream &out, const FailureMode &mode)
	{
		std::string modeString;
		switch(mode)
		{
		case FailureMode::None:
			modeString = "NONE";
			break;
		case FailureMode::NoTestFunction:
			modeString = "NO_TEST_FUNCTION";
			break;
		case FailureMode::FrameworkOutOfMemory:
			modeString = "FRAMEWORK_OUT_OF_MEMORY";
			break;
		case FailureMode::UserOutOfMemory:
			modeString = "USER_OUT_OF_MEMORY";
			break;
		case FailureMode::Exception:
			modeString = "EXCEPTION";
			break;
		case FailureMode::Timeout:
			modeString = "TIMEOUT";
			break;
		case FailureMode::KilledByParent:
			modeString = "KILLED_BY_PARENT";
			break;
		case FailureMode::DanglingThread:
			modeString = "DANGLING_THREAD";
			break;
		default:
			modeString = "INVALID FAILURE MODE";
		}
		out << modeString;
		return out;
	}

	/**
	* Overloaded stream extraction operator for FailureMode enum.
	*/
	inline std::istream &operator>>(std::istream &in, FailureMode &mode)
	{
		std::string modeString;
		in >> modeString;
		if(modeString == "NONE")
			mode = FailureMode::None;
		else if(modeString == "NO_TEST_FUNCTION")
			mode = FailureMode::NoTestFunction;
		else if(modeString == "FRAMEWORK_OUT_OF_MEMORY")
			mode = FailureMode::FrameworkOutOfMemory;
		else if(modeString == "USER_OUT_OF_MEMORY")
			mode = FailureMode::UserOutOfMemory;
		else if(modeString == "EXCEPTION")
			mode = FailureMode::Exception;
		else if(modeString == "TIMEOUT")
			mode = FailureMode::Timeout;
		else if(modeString == "KILLED_BY_PARENT")
			mode = FailureMode::KilledByParent;
		else if(modeString == "DANGLING_THREAD")
			mode = FailureMode::DanglingThread;
		else
			mode = FailureMode::None; // Default to None for unknown strings
		return in;
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
	* Overloaded stream extraction operator for TestFlags.
	*/
	inline std::ostream &operator<<(std::ostream &out, const TestFlags &flags)
	{
		out << "Skip: " << flags.skip
			<< "; StopOnFail: " << flags.stopOnFail
			<< "; StopOnSubtestFail: " << flags.stopSubtestOnFail
			<< "; ExpectFailure: " << flags.expectFailure
			<< "; Verbose: " << flags.verbose;
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
	PARTEST_INLINE_VAR_17 PARTEST_CONSTEXPR_11 const TestFlags TEST_FLAGS_DISABLED = TestFlags::defaultDisabled();
	// Inherit all flags from parent test, except ExpectFailure, which is disabled
	PARTEST_INLINE_VAR_17 PARTEST_CONSTEXPR_11 const TestFlags TEST_FLAGS_INHERIT = TestFlags::defaultInherit();
	// Mask all flags (used for internal purposes)
	PARTEST_INLINE_VAR_17 PARTEST_CONSTEXPR_11 const TestFlags TEST_FLAGS_MASKED = TestFlags::defaultMasked();
	// Set flags to skip the test
	PARTEST_INLINE_VAR_17 PARTEST_CONSTEXPR_11 const TestFlags TEST_FLAGS_SKIP = TestFlags::defaultSkip();
}
#endif // PARTESTTYPES_H
