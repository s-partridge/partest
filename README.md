# Partest

A header-only C++ testing framework designed for parameterized testing with subtest support.

## Overview

Partest provides a lightweight testing framework designed to be usable and easy to understand. The framework is header-only for easy integration and supports parameterized tests, subtests, and a comprehensive set of predefined assertions.

## Core Features

- **Parameterized Testing**: Run the same test logic with different input parameters
- **Subtests**: Organize related test cases within a single test function
- **Header-Only**: 
- **Cross-Standard**: Supports building on C++11/14/17/20, optimized with newer language features when available
- **Cross-platform**: Actively tested on Windows and Linux, with validation for MSVC, GCC, and Clang
- **Extensible**: Assertion system designed for potential future extension

## Current Status

This is an early implementation with manual setup requirements. The framework is functional but still under active development to improve usability and add automation features.

Planned scope:
 - Better event logging
 - JUnit post-test reporting
 - Extended set of Assertion types
 - Live test progress in console
 - Concurrent test execution
 - Command line arguments to control test execution, verbosity, and concurrency
 
 - Parameterized testing: this is available for free via lambda closures, but could be supported explicitly with better constraints
 
Implemented features:

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

Test functions can take the form of any invocable object with signature `void()`, but are required to operate within the scope of the running Test class.

In practice, this means:
- `subtest` can only be invoked from within a member function of a Test class.
- Assertions can only be raised from within the scope of a running test.
- Test functions themselves are simplest to use when they are implemented as members of a Test class.

### Test creation

```cpp
#include <partest/testbase.h>

class UnitTest : public TestBase
{
public:
	UnitTest()
	 : TestBase("Test name", "Test description")
	{
		registerTest(
	}
	
	void validateState(unsigned x, unsigned y)
	{
		ASSERT_EQUAL(x, y);
	}
}
```