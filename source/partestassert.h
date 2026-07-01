#ifndef PARTEST_ASSERT_H
#define PARTEST_ASSERT_H

#include <atomic>
#include <map>
#include <string>

#include "partestcommon.h"
#include "partestassertresult.h"
/**
* Basic assertion macro for use within tests. Must be called within a TestFrame context.
*/
#define ASSERT_TRUE(condition) commitAssertion(partest::handleAssertBoolean((condition), true, ASSERT_TRUE_STR, #condition, __FILE__, __LINE__))
#define ASSERT_FALSE(condition) commitAssertion(partest::handleAssertBoolean((condition), false, ASSERT_FALSE_STR, #condition, __FILE__, __LINE__))

/**
* Basic assertion macros for equality checks. Must be called within a TestFrame context.
*/
#define ASSERT_EQUAL(expected, actual) commitAssertion(partest::handleAssertEqual((expected), (actual), ASSERT_EQUAL_STR, #expected ", " #actual, __FILE__, __LINE__))
#define ASSERT_NOT_EQUAL(expected, actual) commitAssertion(partest::handleAssertNotEqual((expected), (actual), ASSERT_NOT_EQUAL_STR, #expected ", " #actual, __FILE__, __LINE__))

/**
* Assertion macros for approximate equality checks. Must be called within a TestFrame context.
*/
// #define ASSERT_APPROX_EQUAL(expected, actual, tolerance) handleAssertApproxEqual((expected), (actual), (tolerance), ASSERT_APPROX_EQUAL_STR, #expected ", " #actual ", " #tolerance, __FILE__, __LINE__)
// #define ASSERT_APPROX_NOT_EQUAL(expected, actual, tolerance) handleAssertApproxNotEqual((expected), (actual), (tolerance), ASSERT_APPROX_NOT_EQUAL_STR, #expected ", " #actual ", " #tolerance, __FILE__, __LINE__)

/**
* Assertion macros for relational checks. Must be called within a TestFrame context.
*/
#define ASSERT_GREATER(lhs, rhs) commitAssertion(partest::handleAssertBoolean((lhs) > (rhs), true, ASSERT_GREATER_STR, #lhs " > " #rhs, __FILE__, __LINE__))
#define ASSERT_LESS(lhs, rhs) commitAssertion(partest::handleAssertBoolean((lhs) < (rhs), true, ASSERT_LESS_STR, #lhs " < " #rhs, __FILE__, __LINE__))
#define ASSERT_GREATER_EQUAL(lhs, rhs) commitAssertion(partest::handleAssertBoolean((lhs) >= (rhs), true, ASSERT_GREATER_EQUAL_STR, #lhs " >= " #rhs, __FILE__, __LINE__))
#define ASSERT_LESS_EQUAL(lhs, rhs) commitAssertion(partest::handleAssertBoolean((lhs) <= (rhs), true, ASSERT_LESS_EQUAL_STR, #lhs " <= " #rhs, __FILE__, __LINE__))

/**
* Stringified names for each assert type, used for filtering test results
*/
#define ASSERT_TRUE_STR "ASSERT_TRUE"
#define ASSERT_FALSE_STR "ASSERT_FALSE"
#define ASSERT_EQUAL_STR "ASSERT_EQUAL"
#define ASSERT_NOT_EQUAL_STR "ASSERT_NOT_EQUAL"


#define ASSERT_APPROX_EQUAL_STR "ASSERT_APPROX_EQUAL"
#define ASSERT_APPROX_NOT_EQUAL_STR "ASSERT_APPROX_NOT_EQUAL"

#define ASSERT_GREATER_STR "ASSERT_GREATER"
#define ASSERT_LESS_STR "ASSERT_LESS"
#define ASSERT_GREATER_EQUAL_STR "ASSERT_GREATER_EQUAL"
#define ASSERT_LESS_EQUAL_STR "ASSERT_LESS_EQUAL"

#define ASSERT_META_EXPECTED "expected"
#define ASSERT_META_ACTUAL "actual"

namespace partest
{
	////////////////
	// Assertions //
	////////////////
	template<typename T>
	AssertionResult handleAssertBoolean(const T &condition, bool assertTrue, const char *type, const char *conditionStr, const char *file, int line)
	{
		bool passed = (condition == assertTrue);
		AssertionResult result(passed, type, conditionStr, file, line);
		std::ostringstream message;

		if(passed)
		{
			message << type << "(" << conditionStr << ") passed.";
		}
		else
		{
			message << "Assertion failed at " << file << ":" << line
				<< ": " << type << "(" << conditionStr << ") was " << (assertTrue ? "false\n" : "true.\n");
		}
		result.message = message.str();
		return result;
	}

	/**
	* Process an equality assertion. Generate an AssertionResult and log it. If the assertion fails, raise an exception if stopOnFail is enabled.
	* @param expected The expected value
	* @param actual The actual value
	* @param type The type of assertion (e.g., "ASSERT_EQUAL")
	* @param conditionStr The string representation of the condition being tested
	* @param file The file where the assertion is made
	* @param line The line number where the assertion is made
	*/
	template <typename T, typename U>
	AssertionResult handleAssertEqual(const T &expected, const U &actual, const char *type, const char *conditionStr, const char *file, int line)
	{
		bool passed = (expected == actual);
		AssertionResult result(passed, type, conditionStr, file, line);
		std::ostringstream message;

		if(passed)
		{
			message << type << "(" << conditionStr << ") passed.";
		}
		else
		{
			message << "Assertion failed at " << file << ":" << line
				<< ": " << type << "(" << conditionStr << ")\n"
				<< "  Expected: " << maybeStringify(expected) << "\n"
				<< "  Actual:   " << maybeStringify(actual) << "\n";
		}
 		// Write the message to the AssertionResult for logging
 		result.message = message.str();
		return result;
	}

	// C-string specialization for string comparisons
	AssertionResult handleAssertEqual(const char* expected, const char* actual, const char *type, const char *conditionStr, const char* file, int line)
	{
		bool passed = (expected != nullptr && actual != nullptr && strcmp(expected, actual) == 0
			|| expected == nullptr && actual == nullptr);

		AssertionResult result(passed, type, conditionStr, file, line);
		std::ostringstream message;

		if (passed)
		{
			message << type << "(" << conditionStr << ") passed.";
		}
		else
		{
			message << "Assertion failed at " << file << ":" << line
				<< type << "(" << conditionStr << ")\n"
				<< "  Expected: \"" << expected << "\"\n"
				<< "  Actual:   \"" << actual << "\"\n";
		}
			
		result.message = message.str();
		return result;
	}

	/**
	* @param expected The expected value
	* @param actual The actual value
	* @param type The type of assertion (e.g., "ASSERT_EQUAL")
	* @param conditionStr The string representation of the condition being tested
	* @param file The file where the assertion is made
	* @param line The line number where the assertion is made
	*/
	template<typename T, typename U>
	AssertionResult handleAssertNotEqual(const T &expected, const U &actual, const char *type, const char *conditionStr, const char *file, int line)
	{
		bool passed = (expected != actual);
		AssertionResult result(passed, type, conditionStr, file, line);
		std::ostringstream message;

		if(passed)
		{
			message << type << "(" << conditionStr << ") passed.";
		}
		else
		{
			message << "Assertion failed at " << file << ":" << line
				<< type << "(" << conditionStr << ")\n"
				<< "\"" << expected << "\" should not have been " << expected << "\n";
		}

		result.message = message.str();
		return result;
	}

	// C-string specialization for string comparisons
	AssertionResult handleAssertNotEqual(const char* expected, const char* actual, const char* type, const char* conditionStr, const char* file, int line)
	{
		bool passed = (expected != nullptr && actual != nullptr && strcmp(expected, actual) != 0)
			|| (expected == nullptr && actual != nullptr)
			|| (expected != nullptr && actual == nullptr);

		AssertionResult result(passed, type, conditionStr, file, line);
		std::ostringstream message;

		if(passed)
		{
			message << type << "(" << conditionStr << ") passed.";
		}
		else
		{
			message << "Assertion failed at " << file << ":" << line
				<< ": " << type << "(" << conditionStr << ")\n"
				<< "  Expected: " << maybeStringify(expected) << "\n"
				<< "  Actual:   " << maybeStringify(actual) << "\n";
		}

		result.message = message.str();
		return result;
	}
}

#endif //PARTESTASSERT_H