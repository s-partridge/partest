// Entry point for testing the partest framework
#include <iostream>
#include <vector>

#include <partest/bootstrap.h>
#include "assertionTests.h"
#include "semaphoreTests.h"
#include "dispatcherTests.h"

// For memory leak validation on MSVC
#if defined(_MSVC_LANG)
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#endif

class PartestBaseTest : public partest::TestBase
{
public:
	PartestBaseTest() : TestBase("PartestBaseTest", "Core validation for the Partest base class.")
	{		
		// Example of adding a test
		partest::TestFlags flags = partest::TEST_FLAGS_INHERIT;

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
		recordLog(partest::LogLevel::Debug, partest::LOG_TYPE_TEST, "This is a sample log message.");
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
		recordLog(partest::LogLevel::Debug, partest::LOG_TYPE_TEST, "Running example test...");
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
		recordLog(partest::LogLevel::Debug, partest::LOG_TYPE_TEST, "Setting up PartestBaseTest...");
	}

	void teardown() override
	{
		recordLog(partest::LogLevel::Debug, partest::LOG_TYPE_TEST, "Tearing down PartestBaseTest...");
	}
};

int main(int argc, const char **argv)
{
// For memory leak validation on MSVC
#if defined(_MSVC_LANG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	partest::initializeSuite(argc, argv);
	//partest::addTestClass(partest::make_unique<PartestBaseTest>());
	//partest::addTestClass(partest::make_unique<AssertionTests>());
	partest::addTestClass(partest::make_unique<SemaphoreTests>());
	partest::addTestClass(partest::make_unique<DispatcherTests>());

	partest::runAllTests();
	partest::displayAllTests();
	unsigned assertions = partest::getAssertionFailureCount();
	unsigned results = partest::getTopLevelFailures();

	std::cout << "Total assertion failures: " << assertions << std::endl;
	std::cout << "Total top-level failures: " << results << std::endl;

	return results;
}