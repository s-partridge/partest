# Partest Framework Development Roadmap

This feature list includes various things that may or may not end up in the final implementation.
Consider whatever is here to be the absolute maximum scope for now, with potential changes in the future.

---

## Section 1: Core Assertion System Overhaul
**Priority: Critical**

### 1.1 Assert Mechanism Improvements
- [x] Separate test state (NOT_STARTED, RUNNING, COMPLETED, ABORTED) from test outcome (PASSED, FAILED, MIXED, SKIPPED)
- [ ] Create assertion result storage structure
  - Store file, line, condition string, actual values
  - Track individual assertion outcomes
- [x] Implement assertion result vector per test frame
- [ ] Update `TestResult` to contain collection of assertion details

### 1.2 Type-Safe Comparison Assertions
- [x] Implement specialized comparison for C-strings (char*, const char*)
- [ ] Add string comparison (std::string)
- [ ] Handle floating-point comparisons with epsilon tolerance
- [ ] Create comparison dispatcher using template metaprogramming
- [x] Support custom comparison operators for user types

### 1.3 Expanded Assertion Macros
- [x] `ASSERT_FALSE(condition)`
- [ ] `ASSERT_NULL(pointer)` / `ASSERT_NOT_NULL(pointer)`
- [x] `ASSERT_GREATER(a, b)` / `ASSERT_LESS(a, b)`
- [x] `ASSERT_GREATER_EQUAL(a, b)` / `ASSERT_LESS_EQUAL(a, b)`
- [ ] `ASSERT_NEAR(expected, actual, epsilon)` for floating-point
- [ ] `ASSERT_THROWS(expression, exception_type)`
- [ ] `ASSERT_NO_THROW(expression)`
- [ ] `ASSERT_STR_CONTAINS(haystack, needle)`

### 1.4 Detailed Failure Messages
- [x] Capture and format actual vs expected values
- [x] Implement value stringification system
  - Handle primitive types
  - Handle pointers
  - Handle containers (optional, via template specialization)
- [x] Format assertion failure messages with context
- [x] Include expression text in failure output

---

## Section 2: Logging and Reporting System
**Priority: High**

### 2.1 Structured Logging
- [ ] Implement `verbose` flag functionality
  - Log test entry/exit
  - Log each assertion execution
  - Show passing assertions (not just failures)
- [x] Add log levels (ERROR, WARNING, INFO, DEBUG)
- [x] Create logger interface for output customization
- [ ] Support log filtering by level

### 2.2 Test Results Reporting
- [ ] Enhance `printTestTree()` to show assertion details
- [ ] Create summary statistics (total tests, passed, failed, skipped)
- [ ] Add color-coded console output (if terminal supports it)
- [ ] Implement hierarchical result display with indentation
- [ ] Show failure details inline with test tree

### 2.3 Output Formatters
- [ ] Console formatter (human-readable, current default)
- [ ] JSON formatter (for CI/tooling integration)
- [ ] XML formatter (JUnit-compatible for Jenkins, etc.)
- [ ] TAP (Test Anything Protocol) formatter
- [ ] Allow user to select formatter at runtime

---

## Section 3: Performance Testing
**Priority: Medium**

### 3.1 Timing Infrastructure (Foundation)
- [x] Add timing to each test frame
  - Record begin/end test timestamps with events
- [ ] Store duration in test frame/result
- [ ] Add optional macros to manually start/stop timing
- [ ] Support different time units (ms, μs, ns)
- [ ] Display execution time in test reports

### 3.2 Benchmarking Utilities
- [ ] Create utilities to run tests multiple times
- [ ] Collect timing data across iterations
- [ ] Support warmup iterations (exclude from measurements)
- [ ] Create `BENCHMARK()` macro or helper for repeated execution

### 3.3 Statistical Analysis
- [ ] Calculate mean execution time
- [ ] Calculate median execution time
- [ ] Calculate standard deviation
- [ ] Report statistical summary for benchmarks

### 3.4 Comparative Performance Testing
- [ ] Utilities to benchmark multiple implementations
- [ ] Comparison assertions:
  - `ASSERT_PERFORMANCE_SIMILAR(timeA, timeB, tolerance)` - Within X%
  - `ASSERT_FASTER_BY_FACTOR(timeA, timeB, factor)` - At least X times faster
  - `ASSERT_FASTER_THAN(timeA, timeB)` - Simply faster
- [ ] Generate comparison reports
- [ ] Support performance regression detection

---

## Section 4: Advanced Test Features
**Priority: Medium**

### 4.1 Per-Test Setup/Teardown
- [x] Add optional setup/teardown function pointers to TestFrame
- [x] Extend `addTest()` signature with optional setup/teardown parameters
- [ ] Extend `subtest()` signature with optional setup/teardown parameters
- [x] Execute per-test setup before test function
- [x] Execute per-test teardown after test function (even on failure)
- [ ] Document distinction between class-level and test-level lifecycle

### 4.3 Test Filtering and Selection
- [ ] Run tests matching name pattern (wildcards/regex)
- [ ] Filter by test status (run only failed tests)
- [ ] Tag-based test selection
- [ ] Exclude specific tests

### 4.4 Parallel Execution
- [ ] Design flag hierarchy for parallelism control
  - Runner-level: enable parallel test class execution
  - Class-level: enable parallel test execution within a class
  - Both default to disabled (serial execution)
- [ ] Implement thread pool for test execution
- [ ] Add synchronization for result collection
- [ ] Buffer test output to prevent interleaving
- [ ] Add thread-safety warnings to documentation
- [ ] Implement max concurrency control (limit thread count)
- [ ] Ensure proper exception handling across threads
- [ ] Test on multiple platforms for threading issues

### 4.5 Test Dependencies and Ordering
- [ ] Optional explicit test ordering
- [ ] Test dependency declaration (run A before B)
- [ ] Randomized test execution (catch order-dependent bugs)

---

## Section 5: Reliability and Safety
**Priority: Medium-High**

### 5.1 Exception Handling
- [ ] Display exception details in results
- [ ] Support testing for specific exceptions
- [ ] Add exception safety verification utilities

---

## Section 6: Build System and Distribution
**Priority: Medium**

### 6.1 Build System Integration
- [ ] Create CMake configuration
- [ ] Support header-only mode (current implementation)

### 6.2 Cross-Platform Testing
- [ ] Verify Windows (MSVC) support
- [ ] Verify Linux (GCC/Clang) support
- [ ] Verify macOS (Clang) support
- [ ] Test on multiple compiler versions
- [ ] Verify C++14, C++17, C++20 compatibility

### 6.3 Local Build Automation
- [ ] Create build configurations for C++14, 17, 20 in Visual Studio
- [ ] Set up Batch Build or PowerShell script for local multi-standard testing
- [ ] Configure post-build events to run tests automatically

### 6.4 CI/CD Pipeline (GitHub Actions)
- [ ] Set up GitHub Actions workflow for cloud-based automated builds
- [ ] Configure multi-platform builds (Windows/MSVC, Linux/GCC, macOS/Clang)
- [ ] Test across C++ standards on each platform

---

## Section 7: Documentation and Examples
**Priority: High (before public release)**

### 7.1 API Documentation
- [ ] Document all public functions and macros
- [ ] Document flag system and inheritance
- [ ] Document test structure and lifecycle
- [ ] Create Doxygen or similar documentation

### 7.2 User Guide
- [ ] Getting started tutorial
- [ ] Best practices guide
- [ ] Advanced features guide

### 7.3 Examples
- [ ] Basic assertion examples
- [ ] Subtest examples
- [ ] Performance testing examples
- [ ] Integration examples (real-world use cases)
- [ ] Create comprehensive example test suite

---

## Section 8: Polish and Release Preparation
**Priority: High (before public release)**

### 8.1 API Stability
- [ ] Finalize public API (no more breaking changes)
- [ ] Deprecation policy for future changes

### 8.2 Quality Assurance
- [~] Self-test suite (test the testing framework)
- [ ] Edge case testing
- [ ] Error message quality review
- [ ] Performance benchmarking

### 8.3 Release Artifacts
- [ ] Create release packages
- [ ] Write release notes
- [ ] Tag stable releases

---

## Future Considerations (Post-1.0)

### Potential Enhancements
- Timeout support for hanging tests (requires parallel execution infrastructure)
  - Add timeout flag to test configuration
  - Implement test timeout detection
  - Abort hanging tests gracefully
  - Report timeout failures clearly
- Threaded assertion macros for thread-safety testing
  - Per-thread result storage (lock-free design)
  - ASSERT_*_THREADSAFE variants that accept a thread-specific storage reference
  - Each thread writes to its own pre-allocated vector slot (no mutexes)
  - Consolidate results after thread join
  - Enables testing thread-safety without framework locks hiding race conditions
- Enforce Test Registration
  - Create internal mechanism to mark test classes as registered when addTest is called
  - Optionally halt program if not all tests have been registered when runAllTests is called
  - Add statistics for number of times each test is registered and run
  - Can be handled by managing a static set of created and registered tests in PartestBase
  - Each test name gets added to the created set in its constructor
  - Each test name gets added to the registered set when addTest is called, incrementing counter
  - Can include a globl flag to halt/warn/do nothing when a test is not registered
  - runAllTests can validate test registration and respond based on the flag

---

## Notes

### Philosophy
- Keep the framework lightweight and dependency-free
- Header-only by default for easy integration
- Zero-overhead abstractions where possible
- Intuitive API that feels natural in C++
- Maintain backward compatibility within major versions

### Non-Goals
- This is not a mocking framework
- This is not a UI testing framework
- Keep external dependencies minimal or zero