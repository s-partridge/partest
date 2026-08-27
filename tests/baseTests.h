#ifndef BASETESTS_H
#define BASETESTS_H

#include <partest/testbase.h>

class TestBaseTests : public partest::TestBase
{
public:
	TestBaseTests() : TestBase("TestBaseTests", "Core validation for the Partest base class.")
	{		
		// Example of adding a test
		partest::TestFlags flags = partest::TEST_FLAGS_INHERIT;

		addTest(partest::TestInfo("EmptyTests", "Tests for behavior with zero assertions."),
			flags,
			PARTEST_CTX(this){ return this->emptyTests(ctx); });
		addTest(partest::TestInfo("PassedTest", "A test that always passes."),
			flags,
			PARTEST_CTX(this){ return this->passedTest(ctx); });
		addTest(partest::TestInfo("FailingTest", "A test that always fails."),
			flags.withExpectFailure(),
			PARTEST_CTX(this){ return this->failingTest(ctx); });
		addTest(partest::TestInfo("MixedTest", "A test that has mixed results."),
			flags.withExpectFailure(),
			PARTEST_CTX(this){ return this->mixedTest(ctx); });
		addTest(partest::TestInfo("PassedTest", "A test that always passes."),
			flags,
			PARTEST_CTX(this){ return this->passedTest(ctx); });
		addTest(partest::TestInfo("NestedNestedTest", "A test with nested subtests."),
			partest::TEST_FLAGS_INHERIT,
			PARTEST_CTX(this){ return this->nestedNestedTest(ctx); });

		partest::TestInfo metadata("ParameterizedTest", "Validate that parameters are passed correctly.");
		flags = partest::TEST_FLAGS_SKIP;
		addTest(metadata, flags, PARTEST_CTX(this){ return this->parameterizedTest(ctx, 3); });
		addTest(metadata, flags, PARTEST_CTX(this){ return this->parameterizedTest(ctx, 6); });

		flags.skip = partest::FlagState::Disabled;
		flags.stopOnFail = partest::FlagState::Enabled;

		addTest(partest::TestInfo("TestWithStopOnFail", "A test with stopOnFail enabled."),
			flags.withExpectFailure(),
			PARTEST_CTX(this){ return this->testWithStopOnFail(ctx); });
	}

	void sampleTest(TestContext &ctx)
	{
		ctx.recordLog(partest::LogLevel::Debug, partest::LOG_TYPE_TEST, "This is a sample log message.");
		ASSERT_TRUE(true); // This assertion will pass
	}

	void emptyTests(TestContext &ctx)
	{
		ctx.subtest("PassesOnEmpty", "This subtest should report passed with zero assertions", 
		PARTEST_CTX()
		{
		});

		ctx.subtest("FailsOnEmptyWithExpectFailure", "This subtest should report failure with zero assertions", partest::TEST_FLAGS_INHERIT.withExpectFailure(),
		PARTEST_CTX()
		{
			// With expectFailure set, an empty test should unexpectedly pass.
			ctx.subtest("PassUnexpectedly", "This subtest should report failure with zero assertions",
				partest::TEST_FLAGS_INHERIT.withExpectFailure(),
			PARTEST_CTX()
			{
			});
			// Which counts as an actual failure in the parent test. Base subtest should pass with ExpectedFailure result
			// because it recorded an actual failure at nested scope.
		});
	}

	void failingTest(TestContext &ctx)
	{
		ASSERT_TRUE(false); // This assertion will fail
	}

	void mixedTest(TestContext &ctx)
	{
		ASSERT_TRUE(true);  // This assertion will pass
		ASSERT_TRUE(false); // This assertion will fail
	}

	void passedTest(TestContext &ctx)
	{
		ASSERT_TRUE(true); // This assertion will pass
	}

	void parameterizedTest(TestContext &ctx, int testValue)
	{
		ctx.recordLog(partest::LogLevel::Debug, partest::LOG_TYPE_TEST, "Running example test...");
		ctx.subtest(partest::TestInfo("Subtest 1", "A subtest that checks if testValue is 3."), PARTEST_CTX(&)
		{
			// Subtest logic here
			ASSERT_TRUE(testValue == 3);
		});

		ctx.subtest(partest::TestInfo("Subtest 2", "A subtest that checks if testValue is 6."),
			PARTEST_CTX(&)
		{
			// Subtest logic here
			ASSERT_TRUE(testValue == 6);
		});
	}

	// This test should result in a Mixed status due to nested subtests
	void nestedNestedTest(TestContext &ctx)
	{
		ctx.subtest(partest::TestInfo("NestedSubtest 1", "A nested subtest that always passes."), partest::TEST_FLAGS_INHERIT.withExpectFailure(), PARTEST_CTX()
		{
			ASSERT_TRUE(true); // This assertion will pass
			
			ctx.subtest(partest::TestInfo("NestedSubtest 1.1"), PARTEST_CTX()
			{
				ASSERT_TRUE(false); // This assertion will fail
			});

			ctx.subtest(partest::TestInfo("NestedSubtest 1.2", "A nested subtest that always fails."), partest::TEST_FLAGS_INHERIT, PARTEST_CTX()
			{
				ASSERT_TRUE(false); // This assertion will fail
			});

			ctx.subtest(partest::TestInfo("NestedSubtest 1.3", "A nested subtest that always passes."), partest::TEST_FLAGS_INHERIT, PARTEST_CTX()
			{
				ASSERT_TRUE(true); // This assertion will pass
			});
		});

		ctx.subtest(partest::TestInfo("NestedSubtest 2", "A nested subtest that always passes."), partest::TEST_FLAGS_INHERIT, PARTEST_CTX()
		{
			ASSERT_TRUE(true); // This assertion will pass
		});
	}

	void testWithStopOnFail(TestContext &ctx)
	{
		partest::TestFlags stopFlags = partest::TEST_FLAGS_INHERIT;
		stopFlags.stopOnFail = partest::FlagState::Enabled;

		ctx.subtest(partest::TestInfo("Subtest 1", "A subtest that checks if 1 + 1 == 2."), stopFlags, PARTEST_CTX()
		{
			// This assertion will pass
			ASSERT_TRUE(1 + 1 == 2);
		});

		ctx.subtest(partest::TestInfo("Subtest 2", "A subtest that checks if 2 + 2 == 5."), stopFlags, PARTEST_CTX(&)
		{
			ctx.subtest(partest::TestInfo("Nested Subtest", "A nested subtest that checks if 2 + 2 == 4."), stopFlags, PARTEST_CTX()
			{
				// This assertion will fail
				ASSERT_TRUE(2 + 2 == 5);
			});

			// This assertion will pass, if it is hit, which it shouldn't be if stopOnFail is Enabled
			ctx.recordLog(partest::LogLevel::Error, partest::LOG_TYPE_TEST, "Error: This assertion should not run if stopOnFail is ENABLED and a previous assertion failed.");
			ASSERT_TRUE(2 + 2 == 4);
		});

		ctx.subtest(partest::TestInfo("Subtest 3", "A subtest that checks if 3 + 3 == 6."), stopFlags, PARTEST_CTX()
		{
			// This assertion will pass, but should not be reached if stopOnFail is Enabled in Subtest2
			ctx.recordLog(partest::LogLevel::Error, partest::LOG_TYPE_TEST, "Error: This subtest should not run if stopOnFail is ENABLED and a previous assertion failed.");
			ASSERT_TRUE(3 + 3 == 6);
		});
	}

	void setup(TestContext &ctx) override
	{
		ctx.recordLog(partest::LogLevel::Debug, partest::LOG_TYPE_TEST, "Setting up PartestBaseTest...");
	}

	void teardown(TestContext &ctx) override
	{
		ctx.recordLog(partest::LogLevel::Debug, partest::LOG_TYPE_TEST, "Tearing down PartestBaseTest...");
	}
};

#endif