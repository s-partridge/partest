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
		partest::TestParams params;
		params.name = "ExampleTest";
		params.description = "An example test that always passes.";
		params.verbose = true;
		addTest(params, [this]() { return this->exampleTest(); });
	}
	partest::TestStatus exampleTest()
	{
		std::cout << "Running example test..." << std::endl;
		return partest::PASSED;
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