#ifndef PARTEST_EXCEPTIONS_H
#define PARTEST_EXCEPTIONS_H

#include <stdexcept>
#include <string>
#include <cstdio>
#include <partest/common.h>
#include <partest/types.h>

namespace partest
{
	/**
	* Exception raised when an assertion or test fails with stopOnFail enabled.
	*/
	class AssertionFailure {};

	/**
	* Exception raised when a test frame fails to allocate memory, to differentiate OOM sources between the test frame, user code, and general framework code.
	*/
	class TestAllocationFailure {};

	/**
	* Exception raised when the framework fails to allocate memory, but the source cannot be reliably attributed to a specific test frame.
	*/
	class FrameworkAllocationFailure {};

	/**
	* Exception raised for test integrity failures. Indicates a serious issue with the test itself.
	* These will be raised by the framework when an invalid state is detected within the test hierarchy.
	*/
	class TestIntegrityFailure : public std::runtime_error
	{
	public:
		/**
		* Constructor for TestIntegrityFailure.
		* @param message A message describing the integrity failure.
		*/
	#if PARTEST_CPP_VERSION >= 17
		TestIntegrityFailure(std::string_view message) : TestIntegrityFailure(std::string(message)) {}
	#endif
		TestIntegrityFailure(const std::string &message) : TestIntegrityFailure(message.c_str()) {}
		TestIntegrityFailure(const char *message) : std::runtime_error(message) {}
	};

	class TestFrame;
	enum class TestStatus : uint8_t;

	class FrameworkAllocationFailure {};

	// Kill the entire application if the framework fails to allocate memory.
	// This is a last-resort measure to prevent undefined behavior from propagating through the test framework.
	[[noreturn]] inline void abortOnFrameworkAllocationError(BadAllocSource source, const char *testName)
	{
		// Preallocate a buffer for the error message to avoid further allocation failures.
		const char *sourceAsString = "Unknown framework operation";
		char messageBuffer[256] = "";
		switch(source)
		{
		case BadAllocSource::TestCreation:
			sourceAsString = "test registration";
			break;
		case BadAllocSource::TestInitialization:
			sourceAsString = "initialization";
			break;
		case BadAllocSource::TestExecution:
			sourceAsString = "execution";
			break;
		case BadAllocSource::TestFinalization:
			sourceAsString = "finalization";
			break;
		case BadAllocSource::AssertionHandling:
			sourceAsString = "assertion handling";
			break;
		case BadAllocSource::LogRecording:
			sourceAsString = "log recording";
			break;
		case BadAllocSource::UserMessageForwarding:
			sourceAsString = "user message forwarding";
			break;
		case BadAllocSource::TestInfoUpdating:
			sourceAsString = "test info updating";
			break;
		}

		snprintf(messageBuffer, sizeof(messageBuffer), "Fatal error: Failed to allocate memory during %s for test '%s'. The test framework cannot continue and will abort.", sourceAsString, testName ? testName : "<unknown>");
		fputs(messageBuffer, stderr);
		// TODO: Maybe use _Exit(something) instead to control the return value?
		// Is this worth switching to, and would I lose anything by doing so? Abort dumps core, however when this is invoked the stack has already been unwound and the test context is gone, so it may not be useful.
		std::abort();
	}

	/**
	* Call ONLY from within an exception context.
	* Returns a string representation of the exception, if one can be derived.
	*/
	inline std::string stringFromCurrentException()
	{
		try
		{
			throw;
		}
		catch(const AssertionFailure &e)
		{
			return "AssertionFailure";
		}
		catch(const TestAllocationFailure &)
		{
			return "TestAllocationFailure";
		}
		catch(const FrameworkAllocationFailure &)
		{
			return "FrameworkAllocationFailure";
		}
		catch(const std::exception &e)
		{
			return e.what();
		}
		catch(const char *s)
		{
			return std::string("const char*: ") + (s ? s : "(null)");
		}
		catch(const std::string &s)
		{
			return "std::string: " + s;
		}
		catch(int v)
		{
			return "int: " + std::to_string(v);
		}
		catch(...)
		{
			return "unknown exception";
		}
	}

	inline const char* cstringFromCurrentException()
	{
		try
		{
			throw;
		}
		catch(const AssertionFailure &e)
		{
			return "AssertionFailure";
		}
		catch(const TestAllocationFailure &)
		{
			return "TestAllocationFailure";
		}
		catch(const FrameworkAllocationFailure &)
		{
			return "FrameworkAllocationFailure";
		}
		catch(const std::exception &e)
		{
			return e.what();
		}
		catch(const char *s)
		{
			return s ? s : "(null)";
		}
		catch(const std::string &s)
		{
			return s.c_str();
		}
		catch(int v)
		{
			static thread_local char intBuffer[32]; // Buffer for integer to string conversion
			snprintf(intBuffer, sizeof(intBuffer), "int: %d", v);
			return intBuffer;
		}
		catch(...)
		{
			return "unknown exception";
		}
	}
}
#endif
