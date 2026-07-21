#ifndef ASSERTION_TESTS_H
#define ASSERTION_TESTS_H

#include <partest/assert.h>
#include <partest/testbase.h>

class AssertionTests : public partest::TestBase
{
public:
	AssertionTests() : TestBase("AssertionTests", "Validation class for the Partest framework.")
	{		
		// Example of adding a test
		partest::TestFlags flags = partest::TEST_FLAGS_INHERIT;

		addTest(partest::TestInfo("Test Passing Assertions", "Validate that all assert macros pass when handling true conditions."),
			flags,
			[this]() { return this->testAssertionsPass(); });
		addTest(partest::TestInfo("Test Failing Assertions", "Validate that all assert macros fail when handling false conditions."),
			flags,
			[this]() { return this->testAssertionsFail(); });
	}

	void testAssertionsPass()
	{
		subtest(partest::TestInfo("Assert Pass", "All assertions should be true"), [&]()
		{
			ASSERT_TRUE(true);
			ASSERT_FALSE(false);
			ASSERT_EQUAL(1, 1);
			ASSERT_NOT_EQUAL(1, 2);
			ASSERT_GREATER(2, 1);
			ASSERT_GREATER_EQUAL(2, 1);
			ASSERT_GREATER_EQUAL(2, 2);
			ASSERT_LESS(1, 2);
			ASSERT_LESS_EQUAL(1, 2);
			ASSERT_LESS_EQUAL(2, 2);
		});

		unsigned failureCount = getCurrentFrame().getAssertionFailureCount();
		// If no assertions fail, this subtest passed.
		ASSERT_EQUAL(failureCount, 0); // Ensure that no assertions have failed in this subtest
	}

	void testAssertionsFail()
	{

		subtest(partest::TestInfo("Assert Fail", "All assertions should be false"), [&]()
		{
			// This subtest is just a self-check to ensure that assertions are functioning as expected.
			ASSERT_TRUE(false);
			ASSERT_FALSE(true);
			ASSERT_EQUAL(1, 2);
			ASSERT_NOT_EQUAL(1, 1);
			ASSERT_GREATER(1, 2);
			ASSERT_GREATER_EQUAL(1, 2);
			ASSERT_LESS(2, 1);
			ASSERT_LESS_EQUAL(2, 1);
		});
		
		// Hard-coded to the current number of assertions in this subtest.
		// IMPORTANT: If you add or remove assertions in this subtest, you must update this value accordingly.
		unsigned validationCount = 8;
		unsigned failureCount = getCurrentFrame().getAssertionFailureCount();
		// If any assertions fail, this subtest failed.
		ASSERT_EQUAL(failureCount, validationCount); // Ensure that at least one assertion has failed in this subtest
	}
};

#endif