#ifndef PARTEST_ASSERT_H
#define PARTEST_ASSERT_H

#include <atomic>
#include <map>
#include <string>

#include "partestcommon.h"

/**
* Basic assertion macro for use within tests. Must be called within a TestFrame context.
*/
#define ASSERT_TRUE(condition) handleAssertBoolean((condition), true, ASSERT_TRUE_STR, #condition, __FILE__, __LINE__)
#define ASSERT_FALSE(condition) handleAssertBoolean((condition), false, ASSERT_FALSE_STR, #condition, __FILE__, __LINE__)

/**
* Basic assertion macros for equality checks. Must be called within a TestFrame context.
*/
#define ASSERT_EQUAL(expected, actual) handleAssertEqual((expected), (actual), ASSERT_EQUAL_STR, #expected ", " #actual, __FILE__, __LINE__)
#define ASSERT_NOT_EQUAL(expected, actual) handleAssertNotEqual((expected), (actual), ASSERT_NOT_EQUAL_STR, #expected ", " #actual, __FILE__, __LINE__)

/**
* Assertion macros for approximate equality checks. Must be called within a TestFrame context.
*/
// #define ASSERT_APPROX_EQUAL(expected, actual, tolerance) handleAssertApproxEqual((expected), (actual), (tolerance), ASSERT_APPROX_EQUAL_STR, #expected ", " #actual ", " #tolerance, __FILE__, __LINE__)
// #define ASSERT_APPROX_NOT_EQUAL(expected, actual, tolerance) handleAssertApproxNotEqual((expected), (actual), (tolerance), ASSERT_APPROX_NOT_EQUAL_STR, #expected ", " #actual ", " #tolerance, __FILE__, __LINE__)

/**
* Assertion macros for relational checks. Must be called within a TestFrame context.
*/
#define ASSERT_GREATER(lhs, rhs) handleAssertBoolean((lhs) > (rhs), true, ASSERT_GREATER_STR, #lhs " > " #rhs, __FILE__, __LINE__)
#define ASSERT_LESS(lhs, rhs) handleAssertBoolean((lhs) < (rhs), true, ASSERT_LESS_STR, #lhs " < " #rhs, __FILE__, __LINE__)
#define ASSERT_GREATER_EQUAL(lhs, rhs) handleAssertBoolean((lhs) >= (rhs), true, ASSERT_GREATER_EQUAL_STR, #lhs " >= " #rhs, __FILE__, __LINE__)
#define ASSERT_LESS_EQUAL(lhs, rhs) handleAssertBoolean((lhs) <= (rhs), true, ASSERT_LESS_EQUAL_STR, #lhs " <= " #rhs, __FILE__, __LINE__)

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
	struct AssertionResult
	{
	private:
		std::map<std::string, std::string> m_metadata; // Custom metadata associated with this assertion result
	public:
		using MetadataIterator = std::map<std::string, std::string>::const_iterator;

		/**
		* Get a globally incrementing counter. Used internally to assign IDs to newly created test frames.
		* 
		* @return the next value for assertCount
		*/
		static unsigned int getNextAssertID() noexcept {
		
			static std::atomic<unsigned int> assertCount(0);
			return assertCount++;
		}

		unsigned int assertID;

		// Whether the assertion passed or failed
		bool passed;
		// The string name of the assertion type (e.g., "ASSERT_TRUE", "ASSERT_EQUAL")
		// This is provided by ASSERT_TRUE_STR, ASSERT_EQUAL_STR, etc.
		std::string assertType;
		// The condition that was evaluated. This is typically the text of the expression passed to the assertion macro
		// E.g., for ASSERT_TRUE(x > 0), this would be "x > 0"; for ASSERT_EQUAL(a, b), this would be "a, b"
		std::string condition;
		// Custom message associated with the assertion, if any
		std::string message;
		// The file where the assertion was made. Typically provided by the __FILE__ macros
		std::string file;
		// The line number where the assertion was made. Typically provided by the __LINE__ macros
		int line;

		
		AssertionResult() : AssertionResult(false, "", "", "", "", 0) {}

		AssertionResult(
			bool passed,
			PARTEST_STRING_PARAM assertType,
			PARTEST_STRING_PARAM condition,
			PARTEST_STRING_PARAM file,
			int line)
			: AssertionResult(passed, assertType, condition, "", file, line) {}

		/**
		* Constructor for AssertionResult. Handles both equality and non-equality assertions.
		* 
		* @param passed Whether the assertion passed or failed.
		* @param assertType The name of the assertion type (e.g., "ASSERT_TRUE", "ASSERT_EQUAL").
		* @param condition The condition that was evaluated. This is typically the text of the expression passed to the assertion macro.
		* @param message Custom message associated with the assertion, if any.
		* @param file The file where the assertion was made. Typically provided by the __FILE__ macro.
		* @param line The line number where the assertion was made. Typically provided by the __LINE__ macro.
		*/

		AssertionResult(
			bool passed,
			PARTEST_STRING_PARAM assertType,
			PARTEST_STRING_PARAM condition,
			PARTEST_STRING_PARAM message,
			PARTEST_STRING_PARAM file,
			int line)
				: assertID(getNextAssertID()), passed(passed), assertType(assertType),
				condition(condition), message(message), file(file), line(line) {}

		virtual ~AssertionResult() = default;

		/**
		* Set custom metadata key-value pair for this assertion result
		* 
		* @param key The metadata key
		* @param value The metadata value
		*/
		void setMetadata(PARTEST_STRING_PARAM key, PARTEST_STRING_PARAM value) { m_metadata[PARTEST_STRING_PARAM_TO_STRING(key)] = value; }

		template<typename T>
		void setMetadata(PARTEST_STRING_PARAM key, const T &value) { m_metadata[PARTEST_STRING_PARAM_TO_STRING(key)] = maybeStringify(value); }

		/**
		* Check whether metadata exists for the given key
		* 
		* @param key The metadata key
		* @return true if a value exists for `key`, false otherwise
		*/
		bool hasMetadata(PARTEST_STRING_PARAM key) const noexcept { return m_metadata.find(PARTEST_STRING_PARAM_TO_STRING(key)) != m_metadata.end(); }

		/**
		* Get the value of a metadata key for this assertion result
		* 
		* @param key The metadata key
		* @return The metadata value, or an empty string if the key does not exist
		*/
		std::string getMetadata(PARTEST_STRING_PARAM key) const
		{
			MetadataIterator it = m_metadata.find(PARTEST_STRING_PARAM_TO_STRING(key));
			if(it != m_metadata.end())
			{
				return it->second;
			}
			return "";
		}

		MetadataIterator metadataBegin() const noexcept { return m_metadata.cbegin(); }
		MetadataIterator metadataEnd() const noexcept { return m_metadata.cend(); }
	};
}

#endif //PARTESTASSERT_H