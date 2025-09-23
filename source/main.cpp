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
		
		partest::TestFlags flags = partest::TestFlags::defaultInherit();
		flags.verbose = partest::FlagState::ENABLED;

		addTest(metadata, flags, [this]() { return this->exampleTest(3); });
		addTest(metadata, flags, [this]() { return this->exampleTest(6); });

		addTest(partest::TestInfo("FailingTest", "A test that always fails."), partest::TestFlags::defaultInherit(), [this]() { return this->exampleFailingTest(); });
		addTest(partest::TestInfo("MixedTest", "A test that has mixed results."), partest::TestFlags::defaultInherit(), [this]() { return this->exampleMixedTest(); });
		addTest(partest::TestInfo("PassedTest", "A test that always passes."), partest::TestFlags::defaultInherit(), [this]() { return this->examplePassedTest(); });
		addTest(partest::TestInfo("NestedNestedTest", "A test with nested subtests."), partest::TestFlags::defaultInherit(), [this]() { return this->exampleNestedNestedTest(); });
	}

	void exampleTest(int testValue)
	{
		std::cout << "Running example test..." << std::endl;

		SUBTEST(partest::TestInfo("Subtest1", "A subtest that checks if testValue is 3."), partest::TestFlags::defaultInherit())
		{
			// Subtest logic here
			ASSERT_TRUE(testValue == 3);
		}

		SUBTEST(partest::TestInfo("Subtest2", "A subtest that checks if testValue is 6."), partest::TestFlags::defaultInherit())
		{
			// Subtest logic here
			ASSERT_TRUE(testValue == 6);
		}
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

		SUBTEST(partest::TestInfo("NestedSubtest1", "A nested subtest that always passes."), partest::TestFlags::defaultInherit())
		{
			std::cout << "Running NestedSubtest1..." << std::endl;

			std::cout << "Subtest status initial: " << getCurrentFrame().result.status << std::endl;
			ASSERT_TRUE(true); // This assertion will pass
			std::cout << "Current subtest status: " << getCurrentFrame().result.status << std::endl;
			SUBTEST(partest::TestInfo("NestedSubtest1.1", "A nested subtest that always fails."), partest::TestFlags::defaultInherit())
			{
				ASSERT_TRUE(false); // This assertion will fail
			}
			std::cout << "Current subtest status: " << getCurrentFrame().result.status << std::endl;
			SUBTEST(partest::TestInfo("NestedSubtest1.2", "A nested subtest that always passes."), partest::TestFlags::defaultInherit())
			{
				ASSERT_TRUE(true); // This assertion will pass
			}
			std::cout << "Current subtest status: " << getCurrentFrame().result.status << std::endl;
		}

		SUBTEST(partest::TestInfo("NestedSubtest2", "A nested subtest that always passes."), partest::TestFlags::defaultInherit())
		{
			std::cout << "Running NestedSubtest1..." << std::endl;

			std::cout << "Subtest status initial: " << getCurrentFrame().result.status << std::endl;
			ASSERT_TRUE(true); // This assertion will pass
			std::cout << "Current subtest status: " << getCurrentFrame().result.status << std::endl;
		}
		std::cout << "Current test status: " << getCurrentFrame().result.status << std::endl;
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