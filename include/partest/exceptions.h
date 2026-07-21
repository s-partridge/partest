#ifndef PARTEST_EXCEPTIONS_H
#define PARTEST_EXCEPTIONS_H

#include <stdexcept>
#include <string>
#include <partest/common.h>

namespace partest
{
	/**
	* Exception class for assertion failures. Includes file and line information for easier debugging.
	* Used internally by ASSERT macros.
	*/
	class AssertionFailure : public std::runtime_error
	{
		std::string m_file;
		int m_line;
	public:
		/**
		* Constructor for AssertionFailure.
		* 
		* @param file The file where the assertion failed. Typically provided by the __FILE__ macro.
		* @param line The line number where the assertion failed. Typically provided by the __LINE__ macro.
		* @param message A message describing the assertion failure.
		*/
	#if PARTEST_CPP_VERSION >= 17
		AssertionFailure(PARTEST_STRING_PARAM file, int line, std::string_view message) : AssertionFailure(file, line, std::string(message)) {}
	#endif

		AssertionFailure(PARTEST_STRING_PARAM file, int line, const std::string &message) : AssertionFailure(file, line, message.c_str()) {}

		AssertionFailure(PARTEST_STRING_PARAM file, int line, const char *message) : std::runtime_error(message), m_file(file), m_line(line) {}

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
}
#endif