#ifndef ASSERTION_TESTS_H
#define ASSERTION_TESTS_H

#include <partest/assert.h>
#include <partest/testbase.h>

class AssertionTests : public partest::TestBase
{
	static constexpr const char *PASSING_TEST = "Test Passing Assertions";
	static constexpr const char *FAILING_TEST = "Test Failing Assertions";
	class IsolatedTests : public partest::TestBase
	{
	public:
		IsolatedTests() : TestBase("IsolatedTests", "Internal validation for assertions, run separately from suite") 
		{
			addTest(PASSING_TEST, "Validate that all assert macros pass when handling true conditions.",
			partest::TEST_FLAGS_INHERIT,
			[this]() { return this->testAssertionsPass(); });
			addTest(FAILING_TEST, "Validate that all assert macros fail when handling false conditions.",
			partest::TEST_FLAGS_INHERIT,
			[this]() { return this->testAssertionsFail(); });
		}
		
		void testAssertionsPass()
		{
			std::string testStr = "bonus";
			const char *testPtr = "pointer";
			// Boolean
			ASSERT_TRUE(true);
			ASSERT_FALSE(false);
			
			// Equality
			ASSERT_EQUAL(1, 1);
			ASSERT_NOT_EQUAL(1, 2);

			// String equality variants
			ASSERT_EQUAL("hello", "hello");
			ASSERT_EQUAL(testStr, "bonus");
			ASSERT_EQUAL("bonus", testStr);
			ASSERT_EQUAL(testPtr, testPtr);

			ASSERT_NOT_EQUAL("hello", "hellr");
			ASSERT_NOT_EQUAL(testStr, "bonuz");
			ASSERT_NOT_EQUAL("bonuz", testStr);

			// Comparison
			ASSERT_GREATER(2, 1);
			ASSERT_GREATER_EQUAL(2, 1);
			ASSERT_GREATER_EQUAL(2, 2);
			ASSERT_LESS(1, 2);
			ASSERT_LESS_EQUAL(1, 2);
			ASSERT_LESS_EQUAL(2, 2);
		}
		
		void testAssertionsFail()
		{
			std::string testStr = "bonus";
			const char *testPtr = "pointer";

			// Boolean
			ASSERT_TRUE(false);
			ASSERT_FALSE(true);

			// Equality
			ASSERT_EQUAL(1, 2);
			ASSERT_NOT_EQUAL(1, 1);

			// String equality variants
			ASSERT_NOT_EQUAL("hello", "hello");
			ASSERT_NOT_EQUAL(testStr, "bonus");
			ASSERT_NOT_EQUAL("bonus", testStr);
			ASSERT_NOT_EQUAL(testPtr, testPtr);

			ASSERT_EQUAL("hello", "hellr");
			ASSERT_EQUAL(testStr, "bonuz");
			ASSERT_EQUAL("bonuz", testStr);

			// Comparison
			ASSERT_GREATER(1, 2);
			ASSERT_GREATER(2, 2);
			ASSERT_GREATER_EQUAL(1, 2);
			ASSERT_LESS(2, 1);
			ASSERT_LESS(2, 2);
			ASSERT_LESS_EQUAL(2, 1);
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
			[this]() { return this->testAssertions(); });
	}

	void testAssertions()
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