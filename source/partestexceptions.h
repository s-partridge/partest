#ifndef PARTESTEXCEPTIONS_H
#define PARTESTEXCEPTIONS_H

#include <stdexcept>
#include "partestcommon.h"

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
		AssertionFailure(const char *file, int line, PARTEST_STRING_PARAM message)
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
		TestIntegrityFailure(PARTEST_STRING_PARAM message) : std::runtime_error(message) {}
	};
}
#endif