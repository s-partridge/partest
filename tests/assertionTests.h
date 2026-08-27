#ifndef ASSERTION_TESTS_H
#define ASSERTION_TESTS_H

#include <partest/assert.h>
#include <partest/testbase.h>

class AssertionTests : public partest::TestBase
{
	static constexpr const char *PASSING_TEST = "Test Passing Assertions";
	static constexpr const char *PASSING_STRING_TEST = "Test Passing String Assertions";
	static constexpr const char *FAILING_TEST = "Test Failing Assertions";
	class IsolatedTests : public partest::TestBase
	{
	public:
		IsolatedTests() : TestBase("IsolatedTests", "Internal validation for assertions, run separately from suite") 
		{
			
			partest::TestFlags noStopOnFail = partest::TestFlags::defaultInherit().withStopOnFail(partest::FlagState::Disabled);

			addTest(PASSING_TEST, "Validate that all basic assert macros pass when handling true conditions.",
			noStopOnFail,
			PARTEST_CTX(this) { return this->testAssertionsPass(ctx); });
			addTest(PASSING_STRING_TEST, "Validate that all string-specialized macros pass when handling true conditions.",
			noStopOnFail,
			PARTEST_CTX(this) { return this->testStringAssertionsPass(ctx); });
			addTest(FAILING_TEST, "Validate that all assert macros fail when handling false conditions.",
			noStopOnFail,
			PARTEST_CTX(this) { return this->testAssertionsFail(ctx); });
		}

		void testCharArraysPass(TestContext &ctx)
		{
			ctx.subtest("Test Char Arrays", PARTEST_CTX() {
				std::string equalString = "stringA";
				std::string unequalString = "stringB";
				constexpr char firstConstArray[] = "stringA";
				constexpr char equalConstArray[] = "stringA";
				constexpr char unequalConstArray[] = "stringB";
				char equalArray[] = "stringA";
				char unequalArray[] = "stringB";

				ASSERT_NOT_EQUAL((void *)firstConstArray, (void *)equalConstArray);
				// Equality
				ASSERT_EQUAL(firstConstArray, equalConstArray);
				ASSERT_EQUAL(firstConstArray, equalString);
				ASSERT_EQUAL(equalString, firstConstArray);
				// Inequality
				ASSERT_NOT_EQUAL(firstConstArray, unequalConstArray);
				ASSERT_NOT_EQUAL(firstConstArray, unequalString);
				ASSERT_NOT_EQUAL(unequalString, firstConstArray);
				// Const with non-const
				ASSERT_EQUAL(firstConstArray, equalArray);
				ASSERT_EQUAL(equalArray, firstConstArray);
			});

//			ctx.subtest("Test WChar Arrays", PARTEST_CTX() {
//				std::wstring equalString = L"stringA";
//				std::wstring unequalString = L"stringB";
//				const wchar_t firstConstArray[] = L"stringA";
//				const wchar_t equalConstArray[] = L"stringA";
//				const wchar_t unequalConstArray[] = L"stringB";
//				wchar_t equalArray[] = L"stringA";
//				wchar_t unequalArray[] = L"stringB";
//
//				ASSERT_NOT_EQUAL((void *)firstConstArray, (void *)equalConstArray);
//				// Equality
//				ASSERT_EQUAL(firstConstArray, equalConstArray);
//				ASSERT_EQUAL(firstConstArray, equalString);
//				ASSERT_EQUAL(equalString, firstConstArray);
//				// Inequality
//				ASSERT_NOT_EQUAL(firstConstArray, unequalConstArray);
//				ASSERT_NOT_EQUAL(firstConstArray, unequalString);
//				ASSERT_NOT_EQUAL(unequalString, firstConstArray);
//				// Const with non-const
//				ASSERT_EQUAL(firstConstArray, equalArray);
//				ASSERT_EQUAL(equalArray, firstConstArray);
//			});
//
//			ctx.subtest("Test Char16 Arrays", PARTEST_CTX() {
//				std::u16string equalString = u"stringA";
//				std::u16string unequalString = u"stringB";
//				const char16_t firstConstArray[] = u"stringA";
//				const char16_t equalConstArray[] = u"stringA";
//				const char16_t unequalConstArray[] = u"stringB";
//				char16_t equalArray[] = u"stringA";
//				char16_t unequalArray[] = u"stringB";
//
//				ASSERT_NOT_EQUAL((void *)firstConstArray, (void *)equalConstArray);
//				// Equality
//				ASSERT_EQUAL(firstConstArray, equalConstArray);
//				ASSERT_EQUAL(firstConstArray, equalString);
//				ASSERT_EQUAL(equalString, firstConstArray);
//				// Inequality
//				ASSERT_NOT_EQUAL(firstConstArray, unequalConstArray);
//				ASSERT_NOT_EQUAL(firstConstArray, unequalString);
//				ASSERT_NOT_EQUAL(unequalString, firstConstArray);
//				// Const with non-const
//				ASSERT_EQUAL(firstConstArray, equalArray);
//				ASSERT_EQUAL(equalArray, firstConstArray);
//			});
//
//			ctx.subtest("Test Char32 Arrays", PARTEST_CTX() {
//				std::u32string equalString = U"stringA";
//				std::u32string unequalString = U"stringB";
//				const char32_t firstConstArray[] = U"stringA";
//				const char32_t equalConstArray[] = U"stringA";
//				const char32_t unequalConstArray[] = U"stringB";
//				char32_t equalArray[] = U"stringA";
//				char32_t unequalArray[] = U"stringB";
//
//				ASSERT_NOT_EQUAL((void *)firstConstArray, (void *)equalConstArray);
//				// Equality
//				ASSERT_EQUAL(firstConstArray, equalConstArray);
//				ASSERT_EQUAL(firstConstArray, equalString);
//				ASSERT_EQUAL(equalString, firstConstArray);
//				// Inequality
//				ASSERT_NOT_EQUAL(firstConstArray, unequalConstArray);
//				ASSERT_NOT_EQUAL(firstConstArray, unequalString);
//				ASSERT_NOT_EQUAL(unequalString, firstConstArray);
//				// Const with non-const
//				ASSERT_EQUAL(firstConstArray, equalArray);
//				ASSERT_EQUAL(equalArray, firstConstArray);
//			});
//
//#if PARTEST_CPP_20
//			ctx.subtest("Test Char8 Arrays", PARTEST_CTX() {
//				std::u8string equalString = u8"stringA";
//				std::u8string unequalString = u8"stringB";
//				const char8_t firstConstArray[] = u8"stringA";
//				const char8_t equalConstArray[] = u8"stringA";
//				const char8_t unequalConstArray[] = u8"stringB";
//				char8_t equalArray[] = u8"stringA";
//				char8_t unequalArray[] = u8"stringB";
//
//				ASSERT_NOT_EQUAL((void *)firstConstArray, (void *)equalConstArray);
//				// Equality
//				ASSERT_EQUAL(firstConstArray, equalConstArray);
//				ASSERT_EQUAL(firstConstArray, equalString);
//				ASSERT_EQUAL(equalString, firstConstArray);
//				// Inequality
//				ASSERT_NOT_EQUAL(firstConstArray, unequalConstArray);
//				ASSERT_NOT_EQUAL(firstConstArray, unequalString);
//				ASSERT_NOT_EQUAL(unequalString, firstConstArray);
//				// Const with non-const
//				ASSERT_EQUAL(firstConstArray, equalArray);
//				ASSERT_EQUAL(equalArray, firstConstArray);
//			});
//#endif
		}

		template <typename chartype, size_t LenA, size_t LenB>
		void testCharStringsPass(TestContext &ctx, const chartype (&stringA)[LenA], const chartype (&stringB)[LenB])
		{
			// Tests with matching strings
			const chartype *firstConstPointer = stringA;
			const chartype *secondConstPointer = stringA;
			const chartype *diffConstPointer = stringB;

			const chartype *const firstDoubleConstPointer = stringA;
			const chartype *const secondDoubleConstPointer = stringA;
			const chartype *const diffDoubleConstPointer = stringB;

			//chartype firstArray[] = { stringA[0], stringA[1], stringA[2], stringA[3], stringA[4], stringA[5], stringA[6], stringA[7] };
			//chartype secondArray[] = { stringA[0], stringA[1], stringA[2], stringA[3], stringA[4], stringA[5], stringA[6], stringA[7] };
			//chartype diffArray[] = { stringA[0], stringA[1], stringA[2], stringA[3], stringA[4], stringA[5], stringA[6], stringA[7] };

			//const chartype firstConstArray[] = { stringA[0], stringA[1], stringA[2], stringA[3], stringA[4], stringA[5], stringA[6], stringA[7] };
			//const chartype secondConstArray[] = { stringA[0], stringA[1], stringA[2], stringA[3], stringA[4], stringA[5], stringA[6], stringA[7] };
			//const chartype diffConstArray[] = { stringA[0], stringA[1], stringA[2], stringA[3], stringA[4], stringA[5], stringA[6], stringA[7] };

			chartype firstArrayFixed[8] = { stringA[0], stringA[1], stringA[2], stringA[3], stringA[4], stringA[5], stringA[6], stringA[7] };
			chartype secondArrayFixed[8] = { stringA[0], stringA[1], stringA[2], stringA[3], stringA[4], stringA[5], stringA[6], stringA[7] };
			chartype diffArrayFixed[8] = { stringB[0], stringB[1], stringB[2], stringB[3], stringB[4], stringB[5], stringB[6], stringB[7] };

			const chartype firstConstArrayFixed[8] = { stringA[0], stringA[1], stringA[2], stringA[3], stringA[4], stringA[5], stringA[6], stringA[7] };
			const chartype secondConstArrayFixed[8] = { stringA[0], stringA[1], stringA[2], stringA[3], stringA[4], stringA[5], stringA[6], stringA[7] };
			const chartype diffConstArrayFixed[8] = { stringB[0], stringB[1], stringB[2], stringB[3], stringB[4], stringB[5], stringB[6], stringB[7] };

			std::basic_string<chartype> firstString = stringA;
			std::basic_string<chartype> secondString = stringA;
			std::basic_string<chartype> diffString = stringB;

			const std::basic_string<chartype> firstConstString = stringA;
			const std::basic_string<chartype> secondConstString = stringA;
			const std::basic_string<chartype> diffConstString = stringB;

#if PARTEST_CPP_VERSION >= 17

			std::basic_string_view<chartype> firstStringView = stringA;
			std::basic_string_view<chartype> secondStringView = stringA;
			std::basic_string_view<chartype> diffStringView = stringB;

			const std::basic_string_view<chartype> firstConstStringView = stringA;
			const std::basic_string_view<chartype> secondConstStringView = stringA;
			const std::basic_string_view<chartype> diffConstStringView = stringB;
#endif

			// All pairs match with themselves
			ASSERT_EQUAL(firstConstPointer, firstConstPointer);
			ASSERT_EQUAL(firstDoubleConstPointer, firstDoubleConstPointer);
			ASSERT_EQUAL(firstArrayFixed, firstArrayFixed);
			ASSERT_EQUAL(firstConstArrayFixed, firstConstArrayFixed);
			ASSERT_EQUAL(firstString, firstString);
			ASSERT_EQUAL(firstConstString, firstConstString);

#if PARTEST_CPP_VERSION >= 17
			ASSERT_EQUAL(firstStringView, firstStringView);
			ASSERT_EQUAL(firstConstStringView, firstConstStringView);
#endif

			// With others of same type
			ASSERT_EQUAL(firstConstPointer, secondConstPointer);
			ASSERT_EQUAL(firstDoubleConstPointer, secondDoubleConstPointer);
			
			//This validates that cstrings are being compared by value rather than address
			ASSERT_NOT_EQUAL((void *)firstArrayFixed, (void *)secondArrayFixed);
			ASSERT_EQUAL(firstArrayFixed, secondArrayFixed);

			ASSERT_NOT_EQUAL((void *)firstConstArrayFixed, (void *)secondConstArrayFixed);
			ASSERT_EQUAL(firstConstArrayFixed, secondConstArrayFixed);

			ASSERT_EQUAL(firstString, secondString);
			ASSERT_EQUAL(firstConstString, secondConstString);

#if PARTEST_CPP_VERSION >= 17
			ASSERT_EQUAL(firstStringView, secondStringView);
			ASSERT_EQUAL(firstConstStringView, secondConstStringView);
#endif
			/////////////////////////////
			// With others on either side

			///////////////
			// const char * with const char* const
			ASSERT_EQUAL(firstConstPointer, secondDoubleConstPointer);
			ASSERT_EQUAL(firstDoubleConstPointer, secondConstPointer);
			// with fixed arr
			ASSERT_EQUAL(firstConstPointer, secondArrayFixed);
			ASSERT_EQUAL(firstArrayFixed, secondConstPointer);
			// with const fixed arr
			ASSERT_EQUAL(firstConstPointer, secondConstArrayFixed);
			ASSERT_EQUAL(firstConstArrayFixed, secondConstPointer);
			// with string
			ASSERT_EQUAL(firstConstPointer, secondString);
			ASSERT_EQUAL(firstString, secondConstPointer);
			// with const string
			ASSERT_EQUAL(firstConstPointer, secondConstString);
			ASSERT_EQUAL(firstConstString, secondConstPointer);

			/////////////////////
			// const char * const with fixed arr
			ASSERT_EQUAL(firstDoubleConstPointer, secondArrayFixed);
			ASSERT_EQUAL(firstArrayFixed, secondDoubleConstPointer);
			// with const fixed arr
			ASSERT_EQUAL(firstDoubleConstPointer, secondConstArrayFixed);
			ASSERT_EQUAL(firstConstArrayFixed, secondDoubleConstPointer);
			// with string
			ASSERT_EQUAL(firstDoubleConstPointer, secondString);
			ASSERT_EQUAL(firstString, secondDoubleConstPointer);
			// with const string
			ASSERT_EQUAL(firstDoubleConstPointer, secondConstString);
			ASSERT_EQUAL(firstConstString, secondDoubleConstPointer);

			////////////
			// fixed arr with const fixed arr
			ASSERT_EQUAL(firstArrayFixed, secondConstArrayFixed);
			ASSERT_EQUAL(firstConstArrayFixed, secondArrayFixed);
			// with string
			ASSERT_EQUAL(firstArrayFixed, secondString);
			ASSERT_EQUAL(firstString, secondArrayFixed);
			// with const string
			ASSERT_EQUAL(firstArrayFixed, secondConstString);
			ASSERT_EQUAL(firstConstString, secondArrayFixed);

			//////////////////
			// const fixed arr with string
			ASSERT_EQUAL(firstConstArrayFixed, secondString);
			ASSERT_EQUAL(firstString, secondConstArrayFixed);
			// with const string
			ASSERT_EQUAL(firstConstArrayFixed, secondConstString);
			ASSERT_EQUAL(firstConstString, secondConstArrayFixed);

			/////////
			// string with const string
			ASSERT_EQUAL(firstString, secondConstString);
			ASSERT_EQUAL(firstConstString, secondString);

#if PARTEST_CPP_VERSION >= 17
			/////////////
			// stringview with const char *
			ASSERT_EQUAL(firstStringView, secondConstPointer);
			ASSERT_EQUAL(firstConstPointer, secondStringView);
			// with const char * const
			ASSERT_EQUAL(firstStringView, secondDoubleConstPointer);
			ASSERT_EQUAL(firstDoubleConstPointer, secondStringView);
			// with fixed arr
			ASSERT_EQUAL(firstStringView, secondArrayFixed);
			ASSERT_EQUAL(firstArrayFixed, secondStringView);
			// with const fixed arr
			ASSERT_EQUAL(firstStringView, secondConstArrayFixed);
			ASSERT_EQUAL(firstConstArrayFixed, secondStringView);
			// with string
			ASSERT_EQUAL(firstStringView, secondString);
			ASSERT_EQUAL(firstString, secondStringView);
			// with const string
			ASSERT_EQUAL(firstStringView, secondConstString);
			ASSERT_EQUAL(firstConstString, secondStringView);
			// with const stringview
			ASSERT_EQUAL(firstStringView, secondConstStringView);
			ASSERT_EQUAL(firstConstStringView, secondStringView);

			///////////////////
			// const stringview with const char *
			ASSERT_EQUAL(firstConstStringView, secondConstPointer);
			ASSERT_EQUAL(firstConstPointer, secondConstStringView);
			// with const char * const
			ASSERT_EQUAL(firstConstStringView, secondDoubleConstPointer);
			ASSERT_EQUAL(firstDoubleConstPointer, secondConstStringView);
			// with fixed arr
			ASSERT_EQUAL(firstConstStringView, secondArrayFixed);
			ASSERT_EQUAL(firstArrayFixed, secondConstStringView);
			// with const fixed arr
			ASSERT_EQUAL(firstConstStringView, secondConstArrayFixed);
			ASSERT_EQUAL(firstConstArrayFixed, secondConstStringView);
			// with string
			ASSERT_EQUAL(firstConstStringView, secondString);
			ASSERT_EQUAL(firstString, secondConstStringView);
			// with const string
			ASSERT_EQUAL(firstConstStringView, secondConstString);
			ASSERT_EQUAL(firstConstString, secondConstStringView);
#endif

			///////////////////////////
			// Should Fail
			// With others of same type
			ASSERT_NOT_EQUAL(firstConstPointer, diffConstPointer);
			ASSERT_NOT_EQUAL(firstDoubleConstPointer, diffDoubleConstPointer);
			ASSERT_NOT_EQUAL(firstArrayFixed, diffArrayFixed);
			ASSERT_NOT_EQUAL(firstConstArrayFixed, diffConstArrayFixed);
			ASSERT_NOT_EQUAL(firstString, diffString);
			ASSERT_NOT_EQUAL(firstConstString, diffConstString);

#if PARTEST_CPP_VERSION >= 17
			ASSERT_NOT_EQUAL(firstStringView, diffStringView);
			ASSERT_NOT_EQUAL(firstConstStringView, diffConstStringView);
#endif
			/////////////////////////////////
			// Should Fail
			// With others of different types

			///////////////
			// const char * with const char* const
			ASSERT_NOT_EQUAL(firstConstPointer, diffDoubleConstPointer);
			ASSERT_NOT_EQUAL(diffDoubleConstPointer, firstConstPointer);
			// with fixed arr
			ASSERT_NOT_EQUAL(firstConstPointer, diffArrayFixed);
			ASSERT_NOT_EQUAL(diffArrayFixed, firstConstPointer);
			// with const fixed arr
			ASSERT_NOT_EQUAL(firstConstPointer, diffConstArrayFixed);
			ASSERT_NOT_EQUAL(diffConstArrayFixed, firstConstPointer);
			// with string
			ASSERT_NOT_EQUAL(firstConstPointer, diffString);
			ASSERT_NOT_EQUAL(diffString, firstConstPointer);
			// with const string
			ASSERT_NOT_EQUAL(firstConstPointer, diffConstString);
			ASSERT_NOT_EQUAL(diffConstString, firstConstPointer);

			/////////////////////
			// const char * const with fixed arr
			ASSERT_NOT_EQUAL(firstDoubleConstPointer, diffArrayFixed);
			ASSERT_NOT_EQUAL(diffArrayFixed, firstDoubleConstPointer);
			// with const fixed arr
			ASSERT_NOT_EQUAL(firstDoubleConstPointer, diffConstArrayFixed);
			ASSERT_NOT_EQUAL(diffConstArrayFixed, firstDoubleConstPointer);
			// with string
			ASSERT_NOT_EQUAL(firstDoubleConstPointer, diffString);
			ASSERT_NOT_EQUAL(diffString, firstDoubleConstPointer);
			// with const string
			ASSERT_NOT_EQUAL(firstDoubleConstPointer, diffConstString);
			ASSERT_NOT_EQUAL(diffConstString, firstDoubleConstPointer);

			////////////
			// fixed arr with const fixed arr
			ASSERT_NOT_EQUAL(firstArrayFixed, diffConstArrayFixed);
			ASSERT_NOT_EQUAL(diffConstArrayFixed, firstArrayFixed);
			// with string
			ASSERT_NOT_EQUAL(firstArrayFixed, diffString);
			ASSERT_NOT_EQUAL(diffString, firstArrayFixed);
			// with const string
			ASSERT_NOT_EQUAL(firstArrayFixed, diffConstString);
			ASSERT_NOT_EQUAL(diffConstString, firstArrayFixed);

			//////////////////
			// const fixed arr with string
			ASSERT_NOT_EQUAL(firstConstArrayFixed, diffString);
			ASSERT_NOT_EQUAL(firstString, firstConstArrayFixed);
			// with const string
			ASSERT_NOT_EQUAL(diffString, diffConstString);
			ASSERT_NOT_EQUAL(diffConstString, firstConstArrayFixed);

			/////////
			// string with const string
			ASSERT_NOT_EQUAL(firstString, diffConstString);
			ASSERT_NOT_EQUAL(diffConstString, firstString);

#if PARTEST_CPP_VERSION >= 17
			/////////////
			// stringview with const char *
			ASSERT_NOT_EQUAL(firstStringView, diffConstPointer);
			ASSERT_NOT_EQUAL(diffConstPointer, firstStringView);
			// with const char * const
			ASSERT_NOT_EQUAL(firstStringView, diffDoubleConstPointer);
			ASSERT_NOT_EQUAL(diffDoubleConstPointer, firstStringView);
			// with fixed arr
			ASSERT_NOT_EQUAL(firstStringView, diffArrayFixed);
			ASSERT_NOT_EQUAL(diffArrayFixed, firstStringView);
			// with const fixed arr
			ASSERT_NOT_EQUAL(firstStringView, diffConstArrayFixed);
			ASSERT_NOT_EQUAL(diffConstArrayFixed, firstStringView);
			// with string
			ASSERT_NOT_EQUAL(firstStringView, diffString);
			ASSERT_NOT_EQUAL(diffString, firstStringView);
			// with const string
			ASSERT_NOT_EQUAL(firstStringView, diffConstString);
			ASSERT_NOT_EQUAL(diffConstString, firstStringView);
			// with const stringview
			ASSERT_NOT_EQUAL(firstStringView, diffConstStringView);
			ASSERT_NOT_EQUAL(diffConstStringView, firstStringView);

			///////////////////
			// const stringview with const char *
			ASSERT_NOT_EQUAL(firstConstStringView, diffConstPointer);
			ASSERT_NOT_EQUAL(diffConstPointer, firstConstStringView);
			// with const char * const
			ASSERT_NOT_EQUAL(firstConstStringView, diffDoubleConstPointer);
			ASSERT_NOT_EQUAL(diffDoubleConstPointer, firstConstStringView);
			// with fixed arr
			ASSERT_NOT_EQUAL(firstConstStringView, diffArrayFixed);
			ASSERT_NOT_EQUAL(diffArrayFixed, firstConstStringView);
			// with const fixed arr
			ASSERT_NOT_EQUAL(firstConstStringView, diffConstArrayFixed);
			ASSERT_NOT_EQUAL(diffConstArrayFixed, firstConstStringView);
			// with string
			ASSERT_NOT_EQUAL(firstConstStringView, diffString);
			ASSERT_NOT_EQUAL(diffString, firstConstStringView);
			// with const string
			ASSERT_NOT_EQUAL(firstConstStringView, diffConstString);
			ASSERT_NOT_EQUAL(diffConstString, firstConstStringView);
#endif
		}

		void testStringAssertionsPass(TestContext &ctx)
		{
			ctx.subtest("Char String Assertions", PARTEST_CTX(this) {
				this->testCharStringsPass<char>(ctx, "stringA", "stringB");
			});
//
//			ctx.subtest("Wide Char String Assertions", PARTEST_CTX(this) {
//				this->testCharStringsPass<wchar_t>(ctx, L"stringA", L"stringB");
//			});
//
//			ctx.subtest("Char16 String Assertions", PARTEST_CTX(this) {
//				this->testCharStringsPass<char16_t>(ctx, u"stringA", u"stringB");
//			});
//
//			ctx.subtest("Char32 String Assertions", PARTEST_CTX(this) {
//				this->testCharStringsPass<char32_t>(ctx, U"stringA", U"stringB");
//			});
//
//#if PARTEST_CPP_VERSION >= 20
//			ctx.subtest("Char8 String Assertions", PARTEST_CTX(this) {
//				this->testCharStringsPass<char8_t>(ctx, u8"stringA", u8"stringB");
//			});
//#endif
		}

		void testStringAssertionsFail(TestContext &ctx)
		{
			std::string testStr = "bonus";
			const char *testPtr = "pointer";

			// Loaded into stack, guaranteed to have unique memory addresses.
			char charArrayA[] = "pointer";
			char charArrayB[] = "pointer";

			// String equality variants
			ASSERT_NOT_EQUAL("hello", "hello");
			ASSERT_NOT_EQUAL(testStr, "bonus");
			ASSERT_NOT_EQUAL("bonus", testStr);
			ASSERT_NOT_EQUAL(testPtr, testPtr);

			ASSERT_EQUAL("hello", "hellr");
			ASSERT_EQUAL(testStr, "bonuz");
			ASSERT_EQUAL("bonuz", testStr);
		}

		void testAssertionsPass(TestContext &ctx)
		{
			// Boolean
			ASSERT_TRUE(true);
			ASSERT_FALSE(false);
			
			// Equality
			ASSERT_EQUAL(1, 1);
			ASSERT_NOT_EQUAL(1, 2);

			// Comparison
			ASSERT_GREATER(2, 1);
			ASSERT_GREATER_EQUAL(2, 1);
			ASSERT_GREATER_EQUAL(2, 2);
			ASSERT_LESS(1, 2);
			ASSERT_LESS_EQUAL(1, 2);
			ASSERT_LESS_EQUAL(2, 2);

			// Approx equal
			ASSERT_APPROX_EQUAL(9, 10, 1);
			ASSERT_APPROX_EQUAL(10, 10, 1);
			ASSERT_APPROX_EQUAL(11, 10, 1);

			ASSERT_APPROX_EQUAL(1.1, 1.1, 0.0);
			ASSERT_APPROX_EQUAL(1.1, 1.1, 0.1);
			ASSERT_APPROX_EQUAL(1.1 - 0.1, 1.1, 0.1);
			ASSERT_APPROX_EQUAL(1.1 + 0.1, 1.1, 0.1);

			ASSERT_APPROX_NOT_EQUAL(9, 10, 0);
			ASSERT_APPROX_NOT_EQUAL(11, 10, 0);
			ASSERT_APPROX_NOT_EQUAL(1.05, 1.1, 0.01);
			ASSERT_APPROX_NOT_EQUAL(1.15, 1.1, 0.01);
			// Exceptions

			ASSERT_THROWS(std::runtime_error, throw std::runtime_error("Oh no!"));
			unsigned int x;
			ASSERT_NOTHROW(while(x < 10) ++x);
		}
		
		void testAssertionsFail(TestContext &ctx)
		{
			// Boolean
			ASSERT_TRUE(false);
			ASSERT_FALSE(true);

			// Equality
			ASSERT_EQUAL(1, 2);
			ASSERT_NOT_EQUAL(1, 1);

			// Comparison
			ASSERT_GREATER(1, 2);
			ASSERT_GREATER(2, 2);
			ASSERT_GREATER_EQUAL(1, 2);
			ASSERT_LESS(2, 1);
			ASSERT_LESS(2, 2);
			ASSERT_LESS_EQUAL(2, 1);

			// Approx equal
			ASSERT_APPROX_NOT_EQUAL(9, 10, 1);
			ASSERT_APPROX_NOT_EQUAL(10, 10, 1);
			ASSERT_APPROX_NOT_EQUAL(11, 10, 1);

			ASSERT_APPROX_NOT_EQUAL(1.1, 1.1, 0.0);
			ASSERT_APPROX_NOT_EQUAL(1.1, 1.1, 0.1);
			ASSERT_APPROX_NOT_EQUAL(1.1 - 0.1, 1.1, 0.1);
			ASSERT_APPROX_NOT_EQUAL(1.1 + 0.1, 1.1, 0.1);

			ASSERT_APPROX_EQUAL(9, 10, 0);
			ASSERT_APPROX_EQUAL(11, 10, 0);
			ASSERT_APPROX_EQUAL(1.05, 1.1, 0.01);
			ASSERT_APPROX_EQUAL(1.15, 1.1, 0.01);

			unsigned int x;
			ASSERT_THROWS(std::runtime_error, while(x < 10) ++x);
			ASSERT_NOTHROW(throw std::runtime_error("Oh no!"));
		}
	};

	IsolatedTests m_innerTests;

public:
	AssertionTests() : TestBase("AssertionTests", "Validation class for the Partest framework.")
	{
		// Example of adding a test
		partest::TestFlags flags = partest::TEST_FLAGS_INHERIT;

		addTest("Test Assertions", "Validate that all assert macros correctly pass or fail validation.",
			partest::TEST_FLAGS_INHERIT,
			PARTEST_CTX(this) { return this->testAssertions(ctx); });
	}

	void testAssertions(TestContext &ctx)
	{
		m_innerTests.run();

		// Sanity check for subtest names
		ASSERT_TRUE(m_innerTests.containsTest(PASSING_TEST));
		ASSERT_TRUE(m_innerTests.containsTest(FAILING_TEST));

		size_t failureCount = m_innerTests.getAssertionFailureCount(PASSING_TEST);
		// If no assertions fail, this subtest passed.
		ASSERT_EQUAL(failureCount, 0); // Ensure that no assertions have failed in this subtest

		failureCount = m_innerTests.getAssertionFailureCount(FAILING_TEST);
		size_t expectedCount = m_innerTests.getAssertionCount(FAILING_TEST);
		// If any assertions passed, this subtest failed
		ASSERT_EQUAL(failureCount, expectedCount);
	}
};

#endif