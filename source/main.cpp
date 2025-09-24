// Entry point for testing the partest framework

#include <iostream>
#include <vector>

#include "partest.h"

class TestTest : public partest::PartestBase
{
public:
	TestTest() : PartestBase("TestTest", "A test class to test the partest framework itself.")
	{		
		// Example of adding a test
		partest::TestInfo metadata("ExampleTest", "An example test that always passes.");
		
		partest::TestFlags flags = partest::TEST_FLAGS_SKIP;

		addTest(metadata, flags, [this]() { return this->exampleTest(3); });
		addTest(metadata, flags, [this]() { return this->exampleTest(6); });

		addTest(partest::TestInfo("FailingTest", "A test that always fails."),
			flags,
			[this]() { return this->exampleFailingTest(); });
		addTest(partest::TestInfo("MixedTest", "A test that has mixed results."),
			flags,
			[this]() { return this->exampleMixedTest(); });
		addTest(partest::TestInfo("PassedTest", "A test that always passes."),
			flags,
			[this]() { return this->examplePassedTest(); });
		addTest(partest::TestInfo("NestedNestedTest", "A test with nested subtests."),
			flags,
			[this]() { return this->exampleNestedNestedTest(); });
		addTest(partest::TestInfo("TestWithStopOnFail", "A test with stopOnFail enabled."), partest::TEST_FLAGS_INHERIT, [this]() { return this->exampleTestWithStopOnFail(); });
	}

	void exampleTest(int testValue)
	{
		std::cout << "Running example test..." << std::endl;

		SUBTEST(partest::TestInfo("Subtest1", "A subtest that checks if testValue is 3."), partest::TEST_FLAGS_INHERIT)
		{
			// Subtest logic here
			ASSERT_TRUE(testValue == 3);
		}END_SUBTEST();

		SUBTEST(partest::TestInfo("Subtest2", "A subtest that checks if testValue is 6."), partest::TEST_FLAGS_INHERIT)
		{
			// Subtest logic here
			ASSERT_TRUE(testValue == 6);
		}END_SUBTEST();
	}

	void exampleFailingTest()
	{
		ASSERT_TRUE(false); // This assertion will fail
	}

	void exampleMixedTest()
	{
		ASSERT_TRUE(true);  // This assertion will pass
		ASSERT_TRUE(false); // This assertion will fail
	}

	void examplePassedTest()
	{
		ASSERT_TRUE(true); // This assertion will pass
	}

	// This test should result in a MIXED status due to nested subtests
	void exampleNestedNestedTest()
	{
		std::cout << "Running example nested nested test..." << std::endl;
		
		std::cout << "Current test status: " << getCurrentFrame().result.status << std::endl;

		SUBTEST(partest::TestInfo("NestedSubtest1", "A nested subtest that always passes."), partest::TEST_FLAGS_INHERIT)
		{
			std::cout << "Running NestedSubtest1..." << std::endl;

			std::cout << "Subtest status initial: " << getCurrentFrame().result.status << std::endl;
			ASSERT_TRUE(true); // This assertion will pass
			std::cout << "Current subtest status: " << getCurrentFrame().result.status << std::endl;
			SUBTEST(partest::TestInfo("NestedSubtest1.1", "A nested subtest that always fails."), partest::TEST_FLAGS_INHERIT)
			{
				ASSERT_TRUE(false); // This assertion will fail
			}END_SUBTEST();

			std::cout << "Current subtest status: " << getCurrentFrame().result.status << std::endl;
			SUBTEST(partest::TestInfo("NestedSubtest1.2", "A nested subtest that always passes."), partest::TEST_FLAGS_INHERIT)
			{
				ASSERT_TRUE(true); // This assertion will pass
			}END_SUBTEST();

			std::cout << "Current subtest status: " << getCurrentFrame().result.status << std::endl;
		}END_SUBTEST();

		SUBTEST(partest::TestInfo("NestedSubtest2", "A nested subtest that always passes."), partest::TEST_FLAGS_INHERIT)
		{
			std::cout << "Running NestedSubtest1..." << std::endl;

			std::cout << "Subtest status initial: " << getCurrentFrame().result.status << std::endl;
			ASSERT_TRUE(true); // This assertion will pass
			std::cout << "Current subtest status: " << getCurrentFrame().result.status << std::endl;
		}END_SUBTEST();

		std::cout << "Current test status: " << getCurrentFrame().result.status << std::endl;
	}

	void exampleTestWithStopOnFail()
	{
		std::cout << "Running example test with stopOnFail..." << std::endl;

		partest::TestFlags flags = partest::TEST_FLAGS_INHERIT;
		flags.stopOnFail = partest::FlagState::ENABLED;

		SUBTEST(partest::TestInfo("Subtest1", "A subtest that checks if 1 + 1 == 2."), flags)
		{
			// This assertion will pass
			ASSERT_TRUE(1 + 1 == 2);
		}END_SUBTEST();
		SUBTEST(partest::TestInfo("Subtest2", "A subtest that checks if 2 + 2 == 5."), flags)
		{
			SUBTEST(partest::TestInfo("NestedSubtest", "A nested subtest that checks if 2 + 2 == 4."), flags)
			{
				// This assertion will fail
				ASSERT_TRUE(2 + 2 == 5);
			}END_SUBTEST();

			// This assertion will pass, if it is hit, which it shouldn't be if stopOnFail is ENABLED.
			ASSERT_TRUE(2 + 2 == 4);
		}END_SUBTEST();
		SUBTEST(partest::TestInfo("Subtest3", "A subtest that checks if 3 + 3 == 6."), flags)
		{
			// This assertion will pass, but may not be reached if stopOnFail is ENABLED in Subtest2
			ASSERT_TRUE(3 + 3 == 6);
		}END_SUBTEST();
	}

	void setup() override
	{
		std::cout << "Setting up TestTest..." << std::endl;
	}

	void teardown() override
	{
		std::cout << "Tearing down TestTest..." << std::endl;
	}
};

int main()
{
	std::cout << "Partest framework initialized." << std::endl;
	
	TestTest test;

	test.runTests();

	const std::vector<partest::TestResult> &results = test.getResults();
	std::cout << "All tests completed." << std::endl;

	for(const partest::TestResult &result : results)
	{
		std::cout << result.status << " " << result.message << std::endl;
	}

	return 0;
}