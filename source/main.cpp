// Entry point for testing the partest framework

#include <iostream>
#include <vector>

#include "../partest.h"

class TestTest : public partest::PartestBase
{
public:
	TestTest() : PartestBase("TestTest", "Validation class for the Partest framework.")
	{		
		// Example of adding a test
		partest::TestFlags flags = partest::TEST_FLAGS_INHERIT;

		addTest(partest::TestInfo("AssertValidation", "Validate that all assert macros are working correctly."),
			flags,
			[this]() { return this->assertValidation(); });

		addTest(partest::TestInfo("FailingTest", "A test that always fails."),
			flags,
			[this]() { return this->failingTest(); });
		addTest(partest::TestInfo("MixedTest", "A test that has mixed results."),
			flags,
			[this]() { return this->mixedTest(); });
		addTest(partest::TestInfo("PassedTest", "A test that always passes."),
			flags,
			[this]() { return this->passedTest(); });
		addTest(partest::TestInfo("NestedNestedTest", "A test with nested subtests."),
			partest::TEST_FLAGS_INHERIT,
			[this]() { return this->nestedNestedTest(); });

		partest::TestInfo metadata("ParameterizedTest", "Validate that parameters are passed correctly.");
		flags = partest::TEST_FLAGS_SKIP;
		addTest(metadata, flags, [this]() { return this->parameterizedTest(3); });
		addTest(metadata, flags, [this]() { return this->parameterizedTest(6); });

		flags.skip = partest::FlagState::Disabled;
		flags.stopOnFail = partest::FlagState::Enabled;

		addTest(partest::TestInfo("TestWithStopOnFail", "A test with stopOnFail enabled."),
			flags,
			[this]() { return this->testWithStopOnFail(); });
	}

	void sampleTest()
	{
		addLog(partest::LogLevel::Debug, PARTEST_LOG_TYPE_TEST, "This is a sample log message.");
		ASSERT_TRUE(true); // This assertion will pass
	}

	void failingTest()
	{
		ASSERT_TRUE(false); // This assertion will fail
	}

	void mixedTest()
	{
		ASSERT_TRUE(true);  // This assertion will pass
		ASSERT_TRUE(false); // This assertion will fail
	}

	void passedTest()
	{
		ASSERT_TRUE(true); // This assertion will pass
	}

	void parameterizedTest(int testValue)
	{
		addLog(partest::LogLevel::Debug, PARTEST_LOG_TYPE_TEST, "Running example test...");
		subtest(partest::TestInfo("Subtest1", "A subtest that checks if testValue is 3."), [&]()
		{
			// Subtest logic here
			ASSERT_TRUE(testValue == 3);
		});

		subtest(partest::TestInfo("Subtest2", "A subtest that checks if testValue is 6."), [&]()
		{
			// Subtest logic here
			ASSERT_TRUE(testValue == 6);
		});
	}

	void assertValidation()
	{
		subtest(partest::TestInfo("Assert Pass", "All assertions should be true"), [&]()
		{
			subtest(partest::TestInfo("Self-check Pass", "This subtest checks whether assertions are working correctly."), [&]()
			{
				// This subtest is just a self-check to ensure that assertions are functioning as expected.
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
		});

		subtest(partest::TestInfo("Assert Fail", "All assertions should be false"), [&]()
		{
			subtest(partest::TestInfo("Self-check Fail", "This subtest checks whether assertions are working correctly."), [&]()
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
		});
	}

	// This test should result in a Mixed status due to nested subtests
	void nestedNestedTest()
	{
		subtest(partest::TestInfo("NestedSubtest1", "A nested subtest that always passes."), partest::TEST_FLAGS_INHERIT, [this]()
		{
			ASSERT_TRUE(true); // This assertion will pass
			
			subtest(partest::TestInfo("NestedSubtest 1.1"), [this]()
			{
				ASSERT_TRUE(false); // This assertion will fail
			});

			subtest(partest::TestInfo("NestedSubtest1.2", "A nested subtest that always fails."), partest::TEST_FLAGS_INHERIT, [this]()
			{
				ASSERT_TRUE(false); // This assertion will fail
			});

			subtest(partest::TestInfo("NestedSubtest1.3", "A nested subtest that always passes."), partest::TEST_FLAGS_INHERIT, [this]()
			{
				ASSERT_TRUE(true); // This assertion will pass
			});
		});

		subtest(partest::TestInfo("NestedSubtest2", "A nested subtest that always passes."), partest::TEST_FLAGS_INHERIT, [this]()
		{
			ASSERT_TRUE(true); // This assertion will pass
		});
	}

	void testWithStopOnFail()
	{
		partest::TestFlags stopFlags = partest::TEST_FLAGS_INHERIT;
		stopFlags.stopOnFail = partest::FlagState::Enabled;

		subtest(partest::TestInfo("Subtest1", "A subtest that checks if 1 + 1 == 2."), stopFlags, [&]()
		{
			// This assertion will pass
			ASSERT_TRUE(1 + 1 == 2);
		});

		subtest(partest::TestInfo("Subtest2", "A subtest that checks if 2 + 2 == 5."), stopFlags, [&]()
		{
			subtest(partest::TestInfo("NestedSubtest", "A nested subtest that checks if 2 + 2 == 4."), stopFlags, [&]()
			{
				// This assertion will fail
				ASSERT_TRUE(2 + 2 == 5);
			});

			// This assertion will pass, if it is hit, which it shouldn't be if stopOnFail is Enabled
			std::cout << "Error: This assertion should not run if stopOnFail is ENABLED and a previous assertion failed." << std::endl;
			ASSERT_TRUE(2 + 2 == 4);
		});

		subtest(partest::TestInfo("Subtest3", "A subtest that checks if 3 + 3 == 6."), stopFlags, [&]()
		{
			std::cout << "Error: This subtest should not run if stopOnFail is ENABLED and a previous assertion failed." << std::endl;
			// This assertion will pass, but may not be reached if stopOnFail is Enabled in Subtest2
			ASSERT_TRUE(3 + 3 == 6);
		});
	}

	void setup() override
	{
		addLog(partest::LogLevel::Debug, PARTEST_LOG_TYPE_TEST, "Setting up TestTest...");
	}

	void teardown() override
	{
		addLog(partest::LogLevel::Debug, PARTEST_LOG_TYPE_TEST, "Tearing down TestTest...");
	}
};

int main()
{
	std::cout << "Partest framework initialized." << std::endl;
	
	partest::addTestClass(partest::make_unique<TestTest>());
	partest::runAllTests();
	partest::displayAllTests();
	unsigned assertions = partest::getAssertionFailureCount();
	unsigned result = partest::getTopLevelFailures();

	std::cout << "Total assertion failures: " << assertions << std::endl;
	std::cout << "Total top-level failures: " << result << std::endl;

	return 0;
}