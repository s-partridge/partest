#ifndef PARTEST_ASSERT_H
#define PARTEST_ASSERT_H

#include <atomic>
#include <string>

#include <partest/common.h>
#include <partest/assertresult.h>
/**
* Basic assertion macro for use within tests. Must be called within a TestFrame context.
*/
#define ASSERT_TRUE(actual) commitAssertion(partest::handleAssertBoolean((actual), true, ASSERT_TRUE_STR, #actual, __FILE__, __LINE__))
#define ASSERT_FALSE(actual) commitAssertion(partest::handleAssertBoolean((actual), false, ASSERT_FALSE_STR, #actual, __FILE__, __LINE__))

/**
* Basic assertion macros for equality checks. Must be called within a TestFrame context.
*/
#define ASSERT_EQUAL(actual, expected) commitAssertion(partest::handleAssertEqual((actual), (expected), ASSERT_EQUAL_STR, #actual "==" #expected, #actual , #expected, __FILE__, __LINE__))
#define ASSERT_NOT_EQUAL(actual, expected) commitAssertion(partest::handleAssertNotEqual((actual), (expected), ASSERT_NOT_EQUAL_STR, #actual "!=" #expected, #actual , #expected, __FILE__, __LINE__))

/**
* Assertion macros for approximate equality checks. Must be called within a TestFrame context.
*/
#define ASSERT_APPROX_EQUAL(actual, expected, epsilon) handleAssertApproxEqual((actual), (expected), (tolerance), ASSERT_APPROX_EQUAL_STR, #actual "==" #expected ", +/-" #epsilon, #actual, #expected, #epsilon, __FILE__, __LINE__)
#define ASSERT_APPROX_NOT_EQUAL(actual, expected, epsilon) handleAssertApproxNotEqual((actual), (expected), (tolerance), ASSERT_APPROX_NOT_EQUAL_STR, #actual "==" #expected ", +/-" #epsilon, #actual, #expected, #epsilon, __FILE__, __LINE__)

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
PARTEST_INLINE_VAR_17 constexpr const char *ASSERT_TRUE_STR = "ASSERT_TRUE";
PARTEST_INLINE_VAR_17 constexpr const char *ASSERT_FALSE_STR = "ASSERT_FALSE";
PARTEST_INLINE_VAR_17 constexpr const char *ASSERT_EQUAL_STR = "ASSERT_EQUAL";
PARTEST_INLINE_VAR_17 constexpr const char *ASSERT_NOT_EQUAL_STR = "ASSERT_NOT_EQUAL";

PARTEST_INLINE_VAR_17 constexpr const char *ASSERT_APPROX_EQUAL_STR = "ASSERT_APPROX_EQUAL";
PARTEST_INLINE_VAR_17 constexpr const char *ASSERT_APPROX_NOT_EQUAL_STR = "ASSERT_APPROX_NOT_EQUAL";

PARTEST_INLINE_VAR_17 constexpr const char *ASSERT_GREATER_STR = "ASSERT_GREATER";
PARTEST_INLINE_VAR_17 constexpr const char *ASSERT_LESS_STR = "ASSERT_LESS";
PARTEST_INLINE_VAR_17 constexpr const char *ASSERT_GREATER_EQUAL_STR = "ASSERT_GREATER_EQUAL";
PARTEST_INLINE_VAR_17 constexpr const char *ASSERT_LESS_EQUAL_STR = "ASSERT_LESS_EQUAL";

PARTEST_INLINE_VAR_17 constexpr const char *ASSERT_META_EXPECTED = "expected";
PARTEST_INLINE_VAR_17 constexpr const char *ASSERT_META_ACTUAL = "actual";

namespace partest
{
	/////////////
	// Helpers //
	/////////////
	template <typename CharT,
		typename std::enable_if<
			traits::is_char_type<CharT>::value
		, int>::type = 0>
	bool cstringsEqual(const CharT *lhs, const CharT *rhs)
	{
		if(lhs == rhs)
			return true;

		if(lhs == nullptr || rhs == nullptr)
			return false;

		size_t lhsLen = std::char_traits<CharT>::length(lhs);
		size_t rhsLen = std::char_traits<CharT>::length(rhs);

		if(lhsLen != rhsLen)
			return false;

		return std::char_traits<CharT>::compare(lhs, rhs, lhsLen) == 0;
	}

	template <typename CharT>
	bool cstringsNotEqual(const CharT *lhs, const CharT *rhs)
	{
		if(lhs == rhs)
			return false;

		if(lhs == nullptr || rhs == nullptr)
			return true;

		size_t lhsLen = std::char_traits<CharT>::length(lhs);
		size_t rhsLen = std::char_traits<CharT>::length(rhs);

		if(lhsLen != rhsLen)
			return true;

		return std::char_traits<CharT>::compare(lhs, rhs) != 0;
	}

	////////////////
	// Assertions //
	////////////////
	AssertionResult handleAssertBoolean(bool actual, bool assertTrue, const char *type,
		const char *fullExpr,
		const char *file, int line)
	{
		bool passed = (actual == assertTrue);
		AssertionResult result(passed, type, file, line);
		
		result.setMetadata(MetaKeys::Actual, maybeStringify(actual));
		result.setMetadata(MetaKeys::FullExpr, fullExpr);
		result.setMetadata(MetaKeys::Expected, maybeStringify(assertTrue));

		return result;
	}

	/**
	* Process an equality assertion. Generate an AssertionResult and log it. If the assertion fails, raise an exception if stopOnFail is enabled.
	* @param lhs The lhs value
	* @param rhs The rhs value
	* @param type The type of assertion (e.g., "ASSERT_EQUAL")
	* @param conditionStr The string representation of the condition being tested
	* @param file The file where the assertion is made
	* @param line The line number where the assertion is made
	*/
	template<typename T, typename U,
			typename std::enable_if<!(
				traits::is_cstring_type<T>::value ||
				traits::is_cstring_type<U>::value
			), int>::type = 0
		>
	AssertionResult handleAssertEqual(const T &actual, const U &expected, const char *type,
		const char *fullExpr, const char *actualExpr, const char *expectedExpr,
		const char *file, int line)
	{
		bool passed = (actual == expected);
		AssertionResult result(passed, type, file, line);

		result.setMetadata(MetaKeys::Actual, maybeStringify(actual));
		result.setMetadata(MetaKeys::Expected, maybeStringify(expected));
		result.setMetadata(MetaKeys::FullExpr, fullExpr);
		result.setMetadata(MetaKeys::ExprA, actualExpr);
		result.setMetadata(MetaKeys::ExprB, expectedExpr);

		return result;
	}

	// C-string specialization for string comparisons
		template <typename chartypeA, typename chartypeB,
		typename std::enable_if<
			std::is_same<
				typename std::remove_cv<chartypeA>::type,
				typename std::remove_cv<chartypeB>::type
			>::value
		, int>::type = 0>
	AssertionResult handleAssertEqual(const chartypeA *actual, const chartypeB* expected, const char *type,
		const char *fullExpr, const char *actualExpr, const char *expectedExpr,
		const char* file, int line)
	{
		bool passed = cstringsEqual(actual, expected);

		AssertionResult result(passed, type, file, line);

		result.setMetadata(MetaKeys::Actual, maybeStringify(actual));
		result.setMetadata(MetaKeys::Expected, maybeStringify(expected));
		result.setMetadata(MetaKeys::FullExpr, fullExpr);
		result.setMetadata(MetaKeys::ExprA, actualExpr);
		result.setMetadata(MetaKeys::ExprB, expectedExpr);

		return result;
	}

	// C-string specialization for comparisons with std::string
	template<typename chartype,
		typename std::enable_if<
			traits::is_char_type<typename std::remove_cv<chartype>::type>::value
		, int>::type = 0>
	AssertionResult handleAssertEqual(const chartype *actual, const std::basic_string<typename std::remove_cv<chartype>::type> &expected, const char *type,
		const char *fullExpr, const char *actualExpr, const char *expectedExpr,
		const char* file, int line)
	{
		return handleAssertEqual(actual, expected.c_str(), type, fullExpr, actualExpr, expectedExpr, file, line);
	}

	// C-string specialization for comparisons with std::string
	template<typename chartype,
		typename std::enable_if<
			traits::is_char_type<typename std::remove_cv<chartype>::type>::value
		, int>::type = 0>
	AssertionResult handleAssertEqual(const std::basic_string<typename std::remove_cv<chartype>::type> &actual, const chartype *expected, const char *type,
		const char *fullExpr, const char *actualExpr, const char *expectedExpr,
		const char* file, int line)
	{
		return handleAssertEqual(actual.c_str(), expected, type, fullExpr, actualExpr, expectedExpr, file, line);
	}

#if PARTEST_CPP_VERSION >= 17
	// C-string specialization for comparisons with std::string_view
	template<typename chartype,
		typename std::enable_if<
			traits::is_char_type<typename std::remove_cv<chartype>::type>::value
		, int>::type = 0>
	AssertionResult handleAssertEqual(const chartype *actual, const std::basic_string_view<typename std::remove_cv<chartype>::type> &expected, const char *type,
		const char *fullExpr, const char *actualExpr, const char *expectedExpr,
		const char* file, int line)
	{
		return handleAssertEqual(actual, std::basic_string<typename std::remove_cv<chartype>::type>(expected).c_str(), type, fullExpr, actualExpr, expectedExpr, file, line);
	}

	// C-string specialization for comparisons with std::string_view
	template<typename chartype,
		typename std::enable_if<
			traits::is_char_type<typename std::remove_cv<chartype>::type>::value
		, int>::type = 0>
	AssertionResult handleAssertEqual(const std::basic_string_view<typename std::remove_cv<chartype>::type> &actual, const chartype *expected, const char *type,
		const char *fullExpr, const char *actualExpr, const char *expectedExpr,
		const char* file, int line)
	{
		return handleAssertEqual(std::basic_string<typename std::remove_cv<chartype>::type>(actual).c_str(), expected, type, fullExpr, actualExpr, expectedExpr, file, line);
	}
#endif

	/**
	* @param actual The actual value
	* @param expected The expected value
	* @param type The type of assertion (e.g., "ASSERT_NOT_EQUAL")
	* @param conditionStr The string representation of the condition being tested
	* @param file The file where the assertion is made
	* @param line The line number where the assertion is made
	*/
	template<typename T, typename U,
			typename std::enable_if<!(
				traits::is_cstring_type<T>::value ||
				traits::is_cstring_type<U>::value
			), int>::type = 0
		>
	AssertionResult handleAssertNotEqual(const T &actual, const U &expected, const char *type,
		const char *fullExpr, const char *actualExpr, const char *expectedExpr,
		const char *file, int line)
	{
		bool passed = (actual != expected);
		AssertionResult result(passed, type, file, line);

		result.setMetadata(MetaKeys::Actual, maybeStringify(actual));
		result.setMetadata(MetaKeys::Expected, maybeStringify(expected));
		result.setMetadata(MetaKeys::FullExpr, fullExpr);
		result.setMetadata(MetaKeys::ExprA, actualExpr);
		result.setMetadata(MetaKeys::ExprB, expectedExpr);

		return result;
	}

	// C-string specialization for string comparisons
	template <typename chartypeA, typename chartypeB,
		typename std::enable_if<
			std::is_same<
				typename std::remove_cv<chartypeA>::type,
				typename std::remove_cv<chartypeB>::type
			>::value
		, int>::type = 0>
	inline AssertionResult handleAssertNotEqual(const chartypeA* actual, const chartypeB* expected, const char* type,
		const char *fullExpr, const char* actualExpr, const char *expectedExpr,
		const char* file, int line)
	{
		bool passed = !cstringsEqual(actual, expected);

		AssertionResult result(passed, type, file, line);

		result.setMetadata(MetaKeys::Actual, maybeStringify(actual));
		result.setMetadata(MetaKeys::Expected, maybeStringify(expected));
		result.setMetadata(MetaKeys::FullExpr, actualExpr);
		result.setMetadata(MetaKeys::ExprA, actualExpr);
		result.setMetadata(MetaKeys::ExprB, actualExpr);

		return result;
	}

	// C-string specialization for comparisons with std::string
	template<typename chartype,
		typename std::enable_if<
			traits::is_char_type<typename std::remove_cv<chartype>::type>::value
		, int>::type = 0>
	AssertionResult handleAssertNotEqual(const chartype *actual, const std::basic_string<typename std::remove_cv<chartype>::type> &expected, const char *type,
		const char *fullExpr, const char *actualExpr, const char *expectedExpr,
		const char* file, int line)
	{
		return handleAssertNotEqual(actual, expected.c_str(), type, fullExpr, actualExpr, expectedExpr, file, line);
	}

	// C-string specialization for comparisons with std::string
	template<typename chartype,
		typename std::enable_if<
			traits::is_char_type<typename std::remove_cv<chartype>::type>::value
		, int>::type = 0>
	AssertionResult handleAssertNotEqual(const std::basic_string<typename std::remove_cv<chartype>::type> &actual, const chartype *expected, const char *type,
		const char *fullExpr, const char *actualExpr, const char *expectedExpr,
		const char* file, int line)
	{
		return handleAssertNotEqual(actual.c_str(), expected, type, fullExpr, actualExpr, expectedExpr, file, line);
	}

#if PARTEST_CPP_VERSION >= 17
	// C-string specialization for comparisons with std::string_view
	template<typename chartype,
		typename std::enable_if<
			traits::is_char_type<typename std::remove_cv<chartype>::type>::value
		, int>::type = 0>
	AssertionResult handleAssertNotEqual(const chartype *actual, const std::basic_string_view<typename std::remove_cv<chartype>::type> &expected, const char *type,
		const char *fullExpr, const char *actualExpr, const char *expectedExpr,
		const char* file, int line)
	{

		return handleAssertNotEqual(actual, std::basic_string<typename std::remove_cv<chartype>::type>(expected).c_str(), type, fullExpr, actualExpr, expectedExpr, file, line);
	}

	// C-string specialization for comparisons with std::string_view
	template<typename chartype,
		typename std::enable_if<
			traits::is_char_type<typename std::remove_cv<chartype>::type>::value
		, int>::type = 0>
	AssertionResult handleAssertNotEqual(const std::basic_string_view<typename std::remove_cv<chartype>::type> &actual, const chartype *expected, const char *type,
		const char *fullExpr, const char *actualExpr, const char *expectedExpr,
		const char* file, int line)
	{
		return handleAssertNotEqual(std::basic_string<typename std::remove_cv<chartype>::type>(actual).c_str(), expected, type, fullExpr, actualExpr, expectedExpr, file, line);
	}
#endif

	/**
	* @param actual The actual value
	* @param expected The expected value
	* @param type The type of assertion (e.g., "ASSERT_APPROX_EQUAL")
	* @param conditionStr The string representation of the condition being tested
	* @param file The file where the assertion is made
	* @param line The line number where the assertion is made
	*/
	template<typename T, typename U>
	AssertionResult handleAssertApproxEqual(const T &actual, const U &expected,
		const U &epsilon, const char *type,
		const char *fullExpr, const char* actualExpr, const char *expectedExpr,
		const char *epsilonExpr, const char* file, int line)
	{
		U min = expected - epsilon;
		U max = expected + epsilon;
		bool passed = min <= actual && actual <= max;

		AssertionResult result(passed, type, file, line);

		result.setMetadata(MetaKeys::Actual, maybeStringify(actual));
		result.setMetadata(MetaKeys::Expected, maybeStringify(expected));
		result.setMetadata(MetaKeys::Epsilon, maybeStringify(epsilon));
		result.setMetadata(MetaKeys::FullExpr, fullExpr);
		result.setMetadata(MetaKeys::ExprA, actualExpr);
		result.setMetadata(MetaKeys::ExprB, expectedExpr);
		result.setMetadata(MetaKeys::ExprC, epsilonExpr);

		return result;
	}

	/**
	* @param actual The actual value
	* @param expected The expected value
	* @param type The type of assertion (e.g., "ASSERT_APPROX_NOT_EQUAL")
	* @param conditionStr The string representation of the condition being tested
	* @param file The file where the assertion is made
	* @param line The line number where the assertion is made
	*/
	template<typename T, typename U>
	AssertionResult handleAssertApproxNotEqual(const T &actual, const U &expected,
		const U &epsilon, const char *type,
		const char *fullExpr, const char* actualExpr, const char *expectedExpr,
		const char *epsilonExpr, const char* file, int line)
	{
		U min = expected - epsilon;
		U max = expected + epsilon;
		bool passed = actual < min || max < actual;

		AssertionResult result(passed, type, file, line);

		result.setMetadata(MetaKeys::Actual, maybeStringify(actual));
		result.setMetadata(MetaKeys::Expected, maybeStringify(expected));
		result.setMetadata(MetaKeys::Epsilon, maybeStringify(epsilon));
		result.setMetadata(MetaKeys::FullExpr, fullExpr);
		result.setMetadata(MetaKeys::ExprA, actualExpr);
		result.setMetadata(MetaKeys::ExprB, expectedExpr);
		result.setMetadata(MetaKeys::ExprC, epsilonExpr);

		return result;
	}
}

#endif //PARTESTASSERT_H