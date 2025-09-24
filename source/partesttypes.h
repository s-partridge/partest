#ifndef PARTESTTYPES_H
#define PARTESTTYPES_H

#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>

namespace partest
{
	/**
	* Exception class for assertion failures. Includes file and line information for easier debugging.
	* Used internally by ASSERT macros.
	*/
	class AssertionFailure : public std::runtime_error
	{
		const char *m_file;
		int m_line;
	public:
		/**
		* Constructor for AssertionFailure.
		* 
		* @param file The file where the assertion failed. Typically provided by the __FILE__ macro.
		* @param line The line number where the assertion failed. Typically provided by the __LINE__ macro.
		* @param message A message describing the assertion failure.
		*/
		AssertionFailure(const char *file, int line, const std::string &message)
			: std::runtime_error(message), m_file(file), m_line(line) {}

		/**
		* Get the file where the assertion failed.
		* 
		* @return The file name as a C-style string.
		*/
		const char *file() const noexcept { return m_file; }
		
		/**
		* Get the line number where the assertion failed.
		* 
		* @return The line number as an integer.
		*/
		int line() const noexcept { return m_line; }
	};

	/**
	* Exception class for test integrity failures. Indicates a serious issue with the test itself.
	* These will be raised by the framework when it detects an invalid state within the test hierarchy.
	*/
	class TestIntegrityFailure : public std::runtime_error
	{
	public:
		/**
		* Constructor for TestIntegrityFailure.
		* @param message A message describing the integrity failure.
		*/
		TestIntegrityFailure(const std::string &message) : std::runtime_error(message) {}
	};

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
	enum TestStatus : uint8_t
	{
		AWAITING = 0,
		RUNNING,
		PASSED,
		FAILED,
		MIXED,
		SKIPPED
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

		TestFlags() : skip(DISABLED), stopOnFail(INHERIT), stopSubtestOnFail(INHERIT), verbose(INHERIT) {}
		TestFlags(FlagState skip, FlagState stopOnFail, FlagState stopSubtestOnFail, FlagState verbose) : skip(skip), stopOnFail(stopOnFail), stopSubtestOnFail(stopSubtestOnFail), verbose(verbose) {}

		/**
		* Get a TestFlags instance with all flags set to DISABLED
		*/
		static inline TestFlags defaultDisabled() { return TestFlags(DISABLED, DISABLED, DISABLED, DISABLED); }
		/**
		* Get a TestFlags instance with all flags set to INHERIT
		*/
		static inline TestFlags defaultInherit() { return TestFlags(INHERIT, INHERIT, INHERIT, INHERIT); }
		
		/**
		* Get a TestFlags instance with all flags set to MASKED. Used for internal purposes.
		*/
		static inline TestFlags defaultMasked() { return TestFlags(MASKED, MASKED, MASKED, MASKED); }

		/**
		* Default copy assignment operator.
		*/
		inline TestFlags &operator=(const TestFlags &other) = default;

		/**
		* Set flags from another TestFlags instance, ignoring MASKED values
		* 
		* @param other The TestFlags instance to copy flags from. Expects MASKED values to be ignored.
		*/
		inline void setFlags(const TestFlags &other)
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
		inline TestFlags getEffectiveFlags(const TestFlags &parentFlags) const
		{
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
		inline bool isResolved() const
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
		TestInfo() : name(""), description("") {}
		TestInfo(const std::string &name, const std::string &description) : name(name), description(description) {}

		/**
		* Get a TestInfo instance with default (empty) values
		*/
		static inline TestInfo defaultInfo() { return TestInfo("", ""); }
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

		// Message providing additional information about the test result
		std::string message;

		// Constructors
		TestResult() : status(AWAITING), message("") {}
		TestResult(TestStatus status, const std::string &message) : status(status), message(message) {}

		/**
		* Get a TestResult instance with default values (AWAITING status and empty message)
		*/
		static inline TestResult defaultResult() { return TestResult(AWAITING, ""); }

		void updateStatus(const TestStatus &assertResult)
		{
			// Update the status based on the new assertion result
			if(status == AWAITING)
			{
				status = assertResult;
			}
			// If already passed or failed, update to mixed if the new result differs
			else if(status == PASSED && assertResult == FAILED
				 || status == FAILED && assertResult == PASSED)
			{
				status = MIXED;
			}
			// Mixed results remain mixed, skipped tests remain skipped
			else if(status == MIXED || status == SKIPPED)
			{}
			// If currently running, set to the new result
			else if(status == RUNNING)
			{
				status = assertResult;
			}
			// Unknown state, set to AWAITING
			else
			{
				status = AWAITING;
			}
		}

		inline std::ostream &operator<<(std::ostream &out) const
		{
			out << "' - Status: " << status << " - Message: " << message;
			return out;
		}
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
		case ENABLED:
			stateString = "ENABLED";
			break;
		case DISABLED:
			stateString = "DISABLED";
			break;
		default:
			stateString = "INHERIT";
			break;
		}
		out << stateString;
		return out;
	}

	/**
	* Default flag constants for easy reference
	*/
	// Enable all flags
	const TestFlags TEST_FLAGS_DISABLED = TestFlags::TestFlags(FlagState::DISABLED, FlagState::DISABLED, FlagState::DISABLED, FlagState::DISABLED);
	// Inherit all flags from parent test
	const TestFlags TEST_FLAGS_INHERIT = TestFlags::TestFlags(FlagState::INHERIT, FlagState::INHERIT, FlagState::INHERIT, FlagState::INHERIT);
	// Mask all flags (used for internal purposes)
	const TestFlags TEST_FLAGS_MASKED = TestFlags::TestFlags(FlagState::MASKED, FlagState::MASKED, FlagState::MASKED, FlagState::MASKED);
	// Set flags to skip the test
	const TestFlags TEST_FLAGS_SKIP = TestFlags::TestFlags(FlagState::ENABLED, FlagState::INHERIT, FlagState::INHERIT, FlagState::INHERIT);
}
#endif // PARTESTTYPES_H