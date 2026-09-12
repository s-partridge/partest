#ifndef PARTEST_EXCEPTIONS_H
#define PARTEST_EXCEPTIONS_H

#include <stdexcept>
#include <string>
#include <cstdio>
#include <partest/common.h>

namespace partest
{
	/**
	* Exception class for assertion failures. Includes file and line information for easier debugging.
	* Used internally by ASSERT macros.
	*/
	class AssertionFailure
	{
		const std::string m_what;
		const std::string m_file;
		const int m_line;
	public:
		/**
		* Constructor for AssertionFailure.
		* 
		* @param file The file where the assertion failed. Typically provided by the __FILE__ macro.
		* @param line The line number where the assertion failed. Typically provided by the __LINE__ macro.
		* @param message A message describing the assertion failure.
		*/
	#if PARTEST_CPP_VERSION >= 17
		AssertionFailure(PARTEST_STRING_PARAM file, int line, std::string_view message) : m_what(message), m_file(file), m_line(line) {}
	#endif

		AssertionFailure(PARTEST_STRING_PARAM file, int line, const std::string &message) : m_what(message), m_file(file), m_line(line) {}

		AssertionFailure(PARTEST_STRING_PARAM file, int line, const char *message) : m_what(message), m_file(file), m_line(line) {}

		/**
		* Get the message for the assertion
		*/
		const char *what() const noexcept { return m_what.c_str(); }

		/**
		* Get the file where the assertion failed.
		* 
		* @return The file name as a C-style string.
		*/
		const char *file() const noexcept { return m_file.c_str(); }
		
		/**
		* Get the line number where the assertion failed.
		* 
		* @return The line number as an integer.
		*/
		int line() const noexcept { return m_line; }
	};

	/**
	* Exception class for test integrity failures. Indicates a serious issue with the test itself.
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

	/**
	* Enum class representing the source of a bad allocation, used for errors that can't be propagated reliably.
	* A bad_alloc within the framework is most likely unrecoverable, unlike potential memory issues within a user test.
	* This type distinguishes between framework boundaries and user code, allowing for more precise error reporting and handling.
	*/
	enum class BadAllocSource : uint8_t
	{
		Unknown = 0,
		TestCreation = 1,
		TestInitialization = 2,
		TestExecution = 3,
		TestFinalization = 4,
		AssertionHandling = 5,
		LogRecording = 6,
		UserMessageForwarding = 7,
		TestInfoUpdating = 8
	};

	class TestFrame;
	enum class TestStatus : uint8_t;

	class FrameworkAllocationFailure : public std::bad_alloc
	{
		BadAllocSource m_source;
		TestStatus m_testStatus;
		const TestFrame *m_testFrame;
	public:
		FrameworkAllocationFailure(BadAllocSource source, TestStatus testStatus, const TestFrame *testFrame) : std::bad_alloc(), m_source(source), m_testStatus(testStatus), m_testFrame(testFrame) {}

		BadAllocSource source() const noexcept { return m_source; }
		TestStatus testStatus() const noexcept { return m_testStatus; }
		const TestFrame *testFrame() const noexcept { return m_testFrame; }
	};

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
			return e.what();
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
			return e.what();
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
