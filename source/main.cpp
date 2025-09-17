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