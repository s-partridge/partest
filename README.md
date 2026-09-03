# Partest

A header-only C++ testing framework designed for parameterized testing with subtest support.

## Overview

Partest provides a lightweight testing framework designed to be usable and easy to understand. The framework is header-only for easy integration and supports parameterized tests, subtests, and a comprehensive set of predefined assertions.

## Core Features

- **Parameterized Testing**: Run the same test logic with different input parameters
- **Subtests**: Organize related test cases within a single test function
- **Header-Only**: No build steps beyond including the framework in your project
- **Cross-Standard**: Supports building on C++11/14/17/20, optimized with newer language features when available
- **Cross-platform**: Actively tested on Windows and Linux, with validation for MSVC, GCC, and Clang
- **Thread-safe**: Tests are isolated and can be run concurrently without issue
- **Extensible**: Assertion system designed for potential future extension

## Current Status

This is an early implementation with manual setup requirements. The framework is functional but still under active development to improve usability and add automation features.

### Planned scope
 - Better event logging
 - JUnit post-test reporting
 - Extended set of Assertion types
 - Live test progress in console
 - Concurrent test execution
 - Command line arguments to control test execution, verbosity, and concurrency
 - Threaded dispatch for subtests
 - Parameterized testing: this is available for free via lambda closures, but could be supported explicitly with better constraints
 
### Implemented features

 - Core functionality complete: Tests can be created, configured, and run with assertions across test and subtest scope
 - Test fixtures: Test functions can optionally be invoked with setup and teardown functions. TestBase also provides top-level fixtures 
 - Arbitrary subtest depth: Tests can contain subtests. Subtests can contain subtests. The only practical limit is logging clarity
 - Event dispatcher: Events are broadcast to any number of reporters, with a public interface to implement new reporters as needed
 - Basic Assertion types: boolean, equality, and comparison assertions are specified, with a public interface to implement new Assertion types
 - Live test logging: Events are displayed sequentially in the console as they appear

## Usage

`partest/bootstrap.h` provides a functional interface to register tests, run the test suite, and produce reports.

Tests must be implemented within subclasses of TestBase.
Test functions should be registered within the constructor of a Test class.
Test functions are executed automatically when `runTests()` is called on a Test class.

Test functions can take the form of any invocable oject with the signature `void(TestContext &ctx)`. `ctx` encapsulates the scope of the currently running test; it provides access to assertions, scoped logging, and subtest invocation.

Test functions can take the form of any invocable object with signature `void()`, but are required to operate within the scope of the running Test class.

In practice, this means:
- Free functions with the correct signature may be added as tests directly. All others, including member functions, must be invoked through a lambda closure.

As a convenience, `PARTEST_CTX` is provided with the following expansion:
`#define PARTEST_CTX(...) [__VA_ARGS__](partest::TestContext &ctx)`
It is used in place of the conventional signature for a lambda closure.

Assertions are provided in the form of macros. Internally, each macro invokes assertion resolution as a member function of `ctx`.

### Test creation

```cpp
#include <partest/testbase.h>

class UnitTest : public TestBase
{
public:
	UnitTest()
	 : TestBase("UnitTest", "Test description")
	{
		partest::TestFlags flags = partest::TEST_FLAGS_INHERIT;
		
		addTest(partest::TestInfo("ValidateState", "Simple smoke test"),
			flags,
			PARTEST_CTX(this) { return this->validateState(ctx, 0, 0); }
		);
	}
	
	bool isStateValid(const SomeState stateObj) { ... }
	
	void validateState(TestContext &ctx, unsigned x, unsigned y)
	{
		ASSERT_EQUAL(x, y);
		
		SomeState state;
		
		ctx.subtest("DoesUpdateState", partest::TEST_FLAGS_INHERIT, PARTEST_CTX(this, &state)
		{
			ASSERT_TRUE(this->isStateValid(state));
		});
	}
}
```

### Test registration and invocation
```cpp
#include <partest/bootstrap.h>
#include "UnitTest.h"

int main(int argc, char **argv)
{
	// Initialize the test suite with the current configuration
	partest::initializeSuite(argc, argv);
	
	// Create an instance of the test class and pass it to the runner
	partest::addTestClass(partest::make_unique<UnitTest>());
	partest::runAllTests();
	
	// Display aggregated logs from the test suite in the console
	partest::displayAllTests();
	
	// Aggregate the count of all assertions that did not pass, at any level
	unsigned assertions = partest::getAssertionFailureCount();
	
	// Aggregate the count of all test functions that did not pass.
	// This ignores subtests; a failed subtest counts against its parent test's success
	unsigned results = partest::getTopLevelFailures();
	
	std::cout << "Total assertion failures: " << assertions << std::endl;
	std::cout << "Total top-level failures: " << results << std::endl;
	
	return results;
}
```

More comprehensive examples of test use cases can be found in `/tests`, which contains the current suite of tests that ParTest runs on itself.

## Test configuration
Any test or subtest can be configured with a name, a description, and execution flags. Test suites, test functions, and subtests can be individually configured with their own values.

## Command Line Arguments
| Argument | Usage |
| ----------- | ----------- |
| -f, --filter <testNames> | Run only the specified test suites, with names separated by spaces. Names must match the strings declared in your test suites. Default behavior is to run all configured suites. |
| -o, --output <path> | Specify an output path for the JUnit report. Default location is `./testResults.xml` |
| -c, --concurrent | If set, test suites will run in parallel, based on system capability. If unset, test suites run sequentially. |

### Test Flags
TestFlags currently contains the following flags:
| Constant | Usage |
| ----------- | ----------- |
| skip | When enabled, the test or subtest will not be run |
| stopOnFail | When enabled, the test or subtest will stop on first failure |
| expectFailure | When enabled, the test or subtest is considered to have passed if one or more of its assertions failed. A test with this flag set will report *unexpected success* if no assertions fail. |

Note: `expectedFailure` is always set to `disabled`, *even when flags are initialized with `TEST_FLAGS_INHERIT`*. Nesting `expectFailure` is intended to be set explicitly. Enabling it at different levels within the same test hierarchy is not recommended. Correct test resolution in these cases can be difficult to reason about.

The following flags also exist, but they are currently unused:
- stopSubtestOnFail
- verbose

Each flag can be set to `enabled`, `disabled`, or `inherit`. Inherited flags will resolve to their parents' values.
If no flags are specified for a test, they are all set to `disabled`.

### Flag Constants
Several constants exist to initialize the flags for a test:
| Constant | Usage |
| ----------- | ----------- |
| TEST_FLAGS_DISABLED | Disables all flags. This is the default behavior if no preset is specified. |
| TEST_FLAGS_INHERIT | Sets all flags to `inherit` EXCEPT for `expectFailure`. `expectFailure` is ALWAYS disabled unless explicitly enabled. |
| TEST_FLAGS_SKIP | Disables all flags except `skip`. Use this to skip a test entirely. |
| TEST_FLAGS_MASKED | Used internally for flag resolution. |

### Flag Chaining
Some flags can be chained to modify functionality on one line:
- `FlagState::withStopOnFail(FlagState enabled = FlagState::Enabled)` - Returns a copy of the state with stopOnFail explicitly configured.
- `FlagState::withExpectFaliure(FlagState enabled = FlagState::Enabled)` - Returns a copy of the state with expectFailure explicitly configured.

Expected usage:
```cpp
// A subtest that will stop on first failure
ctx.subtest("testName", TEST_FLAGS_INHERIT.withStopOnFail() ...

// A subtest configured to recognize failures as expected
ctx.subtest("testName", TEST_FLAGS_INHERIT.withExpectFaliure() ...

// A subtest configured to do both
ctx.subtest("testName", TEST_FLAGS_INHERIT.withStopOnFail().withExpectFaliure() ...
```