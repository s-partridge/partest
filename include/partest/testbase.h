// File: testbase.h
// Author: Samuel Partridge
//  
// Partest is a lightweight C++ testing framework designed for simplicity and ease of use.
// It allows developers to define and run tests with minimal boilerplate code, making it ideal for quick validation of code functionality.
// Header-only implementation for easy integration into existing projects.

#ifndef PARTEST_BASE_H
#define PARTEST_BASE_H

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cassert>
#include <cstring>
#include <functional>

#include <partest/common.h>
#include <partest/types.h>
#include <partest/log.h>
#include <partest/fileops.h>
#include <partest/testframe.h>
#include <partest/exceptions.h>
#include <partest/eventemitter.h>

#define PARTEST_CTX(...) [__VA_ARGS__](partest::TestContext &ctx)

// Add optional source logging for C++20 and later, using std::source_location to capture file and line information automatically.
#if PARTEST_CPP_VERSION >= 20
#include <source_location>
#define PARTEST_SOURCE_LOCATION_OPT , const std::source_location &location = std::source_location::current()
#else
#define PARTEST_SOURCE_LOCATION_OPT
#endif

// For test suite classes, automatically set the file name and line number where the test suite is defined.
// This is useful for reporting and debugging.
#define PARTEST_SET_SUITE_FILE this->setFileName(__FILE__);
#define PARTEST_SET_SUITE_LINE this->setConstructorLine(__LINE__);
#define PARTEST_SET_SUITE_INFO PARTEST_SET_SUITE_FILE PARTEST_SET_SUITE_LINE

// For test functions, automatically set the file name and line number where the test function is defined.
#define PARTEST_SET_TEST_FILE ctx.setTestFile(__FILE__);
#define PARTEST_SET_TEST_LINE ctx.setTestLine(__LINE__);
#define PARTEST_SET_TEST_INFO PARTEST_SET_TEST_FILE PARTEST_SET_TEST_LINE

namespace partest
{

	class TestBase;

	class TestContext
	{
		TestFrame *m_currentFrame;
		TestBase *m_testSuite;

	public:
		TestContext(TestBase *testSuite, TestFrame *currentFrame)
			: m_testSuite(testSuite), m_currentFrame(currentFrame) {
		}

		template<PARTEST_INVOCABLE_WITH(Func, TestContext&)>
		void subtest(PARTEST_STRING_PARAM name, Func &&testFunc)
		{ subtest(TestInfo(name), TestFlags::defaultInherit(), testFunc); }

		template<PARTEST_INVOCABLE_WITH(Func, TestContext&)>
		void subtest(PARTEST_STRING_PARAM name, PARTEST_STRING_PARAM description, Func &&testFunc)
		{ subtest(TestInfo(name, description), TestFlags::defaultInherit(), testFunc); }

		template<PARTEST_INVOCABLE_WITH(Func, TestContext&)>
		void subtest(PARTEST_STRING_PARAM name, const TestFlags& flags, Func &&testFunc)
		{ subtest(TestInfo(name), flags, testFunc); }

		template<PARTEST_INVOCABLE_WITH(Func, TestContext&)>
		void subtest(PARTEST_STRING_PARAM name, PARTEST_STRING_PARAM description, const TestFlags& flags, Func &&testFunc)
		{ subtest(TestInfo(name, description), flags, testFunc); }

		template<PARTEST_INVOCABLE_WITH(Func, TestContext&)>
		void subtest(Func &&testFunc)
		{ subtest(TestInfo::defaultInfo(), TestFlags::defaultInherit(), testFunc); }

		template<PARTEST_INVOCABLE_WITH(Func, TestContext&)>
		void subtest(const TestFlags& flags, Func &&testFunc)
		{ subtest(TestInfo::defaultInfo(), flags, testFunc); }

		template<PARTEST_INVOCABLE_WITH(Func, TestContext&)>
		void subtest(const TestInfo &testInfo, Func &&testFunc)
		{ subtest(testInfo, TestFlags::defaultInherit(), testFunc); }

		template<PARTEST_INVOCABLE_WITH(Func, TestContext&)>
		void subtest(const TestInfo &testInfo, const TestFlags& flags, Func &&testFunc);

		void commitAssertion(const AssertionResult &result);
		void recordLog(LogLevel level, PARTEST_STRING_PARAM type, PARTEST_STRING_PARAM message);
		void setTestFile(PARTEST_STRING_PARAM fileName);
		void setTestLine(unsigned line);
	};
	/**
	* Base class for all partest tests.
	*/
	class TestBase
	{
		friend class TestContext;

	protected:
		using TestContext = partest::TestContext;
		using TestFrame = partest::TestFrame;
		using TestInfo = partest::TestInfo;

	private:
		std::unique_ptr<TestFrame> m_testTree; // Dynamically growing tree of test frames
		EventEmitter m_eventEmitter; // Component that transmits events to a dispatcher

		void runTest(TestFrame *test)
		{
			// There is no point where this should be null in production code.
			// If it is, it indicates a serious issue with the test framework itself.
			assert(test != nullptr && "Test was run with a null TestFrame pointer.");

			TestContext ctx(this, test);

			if(test->initializeTest(ctx))
			{
				std::stringstream resultStream;
				try
				{
					test->runTestFunction(ctx);
				}
				
				// A test returned early due to an assertion failure with stopOnFail enabled
				// Nothing special to do here, but this is necessary to prevent the exception from propagating further.

				// Assertion failures indicate that the test has already been marked as Failed, so no additional action is needed here 
				catch(const partest::AssertionFailure &)
				{ }
				// Unexpected exceptions will generally indicate errors within the user's test code and must be reported
				catch(...)
				{
					std::string message = "Error: Unhandled exception in test '" + test->metadata.name + "': " + stringFromCurrentException();
					test->abortTest(message);
				}

				test->finalizeTest(ctx);
			}
		}

		/**
		* Function to run all registered tests.
		* Run all registered tests, calling setup and teardown functions before and after.
		*/
		void runBaseTests(TestContext& ctx)
		{
			// Iterate through all registered tests
			// The root of the test tree is an exception to thread safety concerns. It is encapsulated entirely here.
			// Root test tree immutability is an invariant of the framework, enforced by addTest. No new test nodes can be adde while this function is running.
			for(std::vector<TestFrame*>::iterator test = m_testTree->subtestsBegin(); test != m_testTree->subtestsEnd(); ++test)
			{
				runTest(*test);

				// If the test failed and stopOnFail is enabled, stop executing further tests
				if(m_testTree->getEffectiveFlags().stopOnFail == FlagState::Enabled && m_testTree->hasFailures())
				{
					break;
				}
			}
		}

		/**
		* Check if the current test should raise an assertion failure based on its status and flags. Used in ASSERT macros.
		* 
		* @param file The file where the assertion is being checked. Typically provided by the __FILE__ macro.
		* @param line The line number where the assertion is being checked. Typically provided by the __LINE__ macro.
		* @param condition The condition being asserted, as a string. Typically provided by the condition expression itself.
		* @throws AssertionFailure if the current test has failed and stopOnFail is enabled.
		*/
		void maybeRaiseOnAssertion(const char *file, int line, PARTEST_STRING_PARAM condition, TestFrame *test)
		{
			if(test->getEffectiveFlags().stopOnFail == FlagState::Enabled && (test->hasFailures()))
			{
				throw AssertionFailure(file, line, condition);
			}
		}

		void maybeRaiseOnSubtestReturned(const char *file, int line, PARTEST_STRING_PARAM condition, TestFrame *test)
		{
			if(test->getEffectiveFlags().stopOnFail == FlagState::Enabled && test->getTestFailureCount(1))
			{
				throw AssertionFailure(file, line, condition);
			}
		}

		/**
		* Check if the current test should raise an assertion failure based on its status and flags. Used in ASSERT macros.
		* 
		* @param result Object containing the evaluated result of an assertion
		* @throws AssertionFailure if the current test has failed and stopOnFail is enabled.
		*/
		void maybeRaiseOnAssertion(const AssertionResult &result, TestFrame *test) { maybeRaiseOnAssertion(result.file.c_str(), result.line, result.getCondition(), test); }

		/////////////////////
		//Subtest overloads//
		/////////////////////
		template<PARTEST_INVOCABLE_WITH(Func, TestContext&)>
		void subtest(const TestInfo &testInfo, const TestFlags& flags, Func &&testFunc, TestFrame *parent)
		{
			assert(parent != nullptr && "Parent test frame is null. Subtests must be added to a valid parent test frame.");

			TestFrame *subtest = parent->addSubtest(partest::make_unique<TestFrame>(&m_eventEmitter, flags, testInfo, testFunc));
			runTest(subtest);
			subtest->setTestFunction(nullptr); // Clear the function to avoid dangling references. This is only necessary for subtests because they are intended to be run immediately and then discarded.

			maybeRaiseOnSubtestReturned("", 0, "Stopped on failure in " + parent->metadata.name, subtest);
		}

		/**
		* Process an evaluated assertion. Log it and raise an exception if necessary.
		* 
		* @param result Output of an evaluated assertion. AssertionResults should be produced by assertion handlers.
		* @throws AssertionFailure if the assertion result did not pass and stopOnFail is enabled.
		*/
		void commitAssertion(const AssertionResult &result, TestFrame *test)
		{
			// Pass the assertion result on to the test frame
			test->processAssertion(result);

			// On failure, allow an exception to be raised if the current test frame is configured to do so.
			if(!result.passed())
				maybeRaiseOnAssertion(result.file.c_str(), result.line, result.getCondition(), test);
		}

		/**
		* Log a message.
		* @param level The log level.
		* @param type The log type.
		* @param message The log message.
		*/
		void recordLog(LogLevel level, PARTEST_STRING_PARAM type, PARTEST_STRING_PARAM message, TestFrame *test)
		{
			test->recordLog(level, type, message);
		}

	protected:
		/** 
		* Adds a test function to the list of tests to be executed. Expected to be called in the constructor of derived classes.
		* 
		* Example usage:
		*   addTest(TestInfo("TestName", "Description of the test"), TestFlags::defaultInherit(), [this]() { return this->testFunction(); });
		*   addTest(partest::TestInfo::defaultInfo(), partest::TestFlags::defaultInherit(), [this]() { return this->anotherTestFunction(); });
		* 
		* @param metadata Metadata for the test, including name and description. Use TestInfo::defaultInfo() for default values.
		* @param flags Flags specific to this test, which can override global or current flags. Use TestFlags::defaultInherit() to inherit all flags.
		* @param testFunc The test function to be executed, which should return a TestStatus. Typically a lambda that calls a member function with specific parameters.
		* @param setupFunc Optional setup function to be called before the test function. Default is nullptr.
		* @param teardownFunc Optional teardown function to be called after the test function. Default is nullptr.
		*/
		template<
			PARTEST_INVOCABLE_WITH(Func, TestContext&),
			PARTEST_INVOCABLE_WITH_OPT(SetupFunc, TestContext&),
			PARTEST_INVOCABLE_WITH_OPT(TeardownFunc, TestContext&)
		>
		void addTest(const TestInfo &metadata, const TestFlags &flags, Func &&testFunc, SetupFunc &&setupFunc = nullptr, TeardownFunc &&teardownFunc = nullptr)
		{
			assert(!m_testTree->isRunning() && "Cannot add top-level tests while the test suite is running. Ensure tests are registered prior to calling run()");
			m_testTree->addSubtest(partest::make_unique<TestFrame>(&m_eventEmitter, flags, metadata, testFunc, setupFunc, teardownFunc));
		}

		template<
			PARTEST_INVOCABLE_WITH(Func, TestContext&),
			PARTEST_INVOCABLE_WITH_OPT(SetupFunc, TestContext&),
			PARTEST_INVOCABLE_WITH_OPT(TeardownFunc, TestContext&)
		>
		void addTest(PARTEST_STRING_PARAM name, const TestFlags &flags, Func &&testFunc, SetupFunc &&setupFunc = nullptr, TeardownFunc &&teardownFunc = nullptr)
		{
			assert(!m_testTree->isRunning() && "Cannot add top-level tests while the test suite is running. Ensure tests are registered prior to calling run()");
			m_testTree->addSubtest(partest::make_unique<TestFrame>(&m_eventEmitter, flags, TestInfo(name), testFunc, setupFunc, teardownFunc));
		}

		template<
			PARTEST_INVOCABLE_WITH(Func, TestContext&),
			PARTEST_INVOCABLE_WITH_OPT(SetupFunc, TestContext&),
			PARTEST_INVOCABLE_WITH_OPT(TeardownFunc, TestContext&)
		>
		void addTest(PARTEST_STRING_PARAM name, PARTEST_STRING_PARAM description, const TestFlags &flags, Func &&testFunc, SetupFunc &&setupFunc = nullptr, TeardownFunc &&teardownFunc = nullptr)
		{
			assert(!m_testTree->isRunning() && "Cannot add top-level tests while the test suite is running. Ensure tests are registered prior to calling run()");
			m_testTree->addSubtest(partest::make_unique<TestFrame>(&m_eventEmitter, flags, TestInfo(name, description), testFunc, setupFunc, teardownFunc));
		}

		void setFileName(PARTEST_STRING_PARAM fileName) { m_testTree->setTestFile(fileName); }
		void setConstructorLine(unsigned line) { m_testTree->setTestLine(line); }
		/**
		* Setup function to be overridden by derived classes
		*/
		virtual void setup(TestContext& ctx) {}

		/**
		* Teardown function to be overridden by derived classes
		*/
		virtual void teardown(TestContext& ctx) {}

	public:
		TestBase(PARTEST_STRING_PARAM name, PARTEST_STRING_PARAM description,
			const TestFlags &flags = TEST_FLAGS_DISABLED PARTEST_SOURCE_LOCATION_OPT)
		{
			// Initialize the root test frame. This frame is not associated with any specific test but serves as the root of the test tree.
			// Its primary purpose is to contain information such as the overall test suite name and description in the same collection as the individual tests.
			m_testTree = partest::make_unique<TestFrame>(&m_eventEmitter, flags, TestInfo(name, description));
			// Set the setup and teardown functions for the root test frame
			m_testTree->setSetupFunction([this](TestContext& context) { this->setup(context); });
			m_testTree->setTestFunction([this](TestContext& context) { this->runBaseTests(context); });
			m_testTree->setTeardownFunction([this](TestContext& context) { this->teardown(context); });

		#if PARTEST_CPP_VERSION >= 20
			// from PARTEST_SOURCE_LOCATION_OPT, only available automatically in C++20 and later.
			// For earlier versions, users must set the file and line manually in the derived class constructor.
			m_testTree->setTestFile(getFilename(location.file_name()));
			m_testTree->setTestLine(location.line());
		#endif
		}
		virtual ~TestBase() = default;

		TestBase(const TestBase &) = delete; // Disable copy constructor
		TestBase &operator=(const TestBase &) = delete; // Disable copy assignment
		TestBase(TestBase &&) = delete; // Disable move constructor
		TestBase &operator=(TestBase &&) = delete; // Disable move assignment

		void configureEventEmitter(const EmitterConfig &emitterConfig) { m_eventEmitter.setConfiguration(emitterConfig); }

		void setName(PARTEST_STRING_PARAM name) { m_testTree->metadata.name = name; }
		const std::string &getName() const noexcept { return m_testTree->metadata.name; }

		void setDescription(PARTEST_STRING_PARAM description) { m_testTree->metadata.description = description; }
		const std::string &getDescription() const noexcept { return m_testTree->metadata.description; }
		
		void setFlags(const TestFlags &flags) noexcept { m_testTree->flags.setFlags(flags); }
		const TestFlags &getFlags() const noexcept { return m_testTree->flags; }

		bool containsTest(PARTEST_STRING_PARAM testName) const
		{
			return m_testTree->getSubtest(testName) != nullptr;
		}

		void run()
		{
			runTest(m_testTree.get());
		}

		bool readTestTree(TestFrameReaderInterface *reader)
		{
			// Ensure test suite is not being modified while reading the tree. This is a safety check to prevent concurrent modifications during test execution.
			if(!m_testTree->isRunning())
			{
				reader->readTree(*m_testTree);
				return true;
			}
			return false;
		}

		size_t getTestCount() const
		{
			return m_testTree->subtestCount();
		}

		size_t getAssertionCount(bool onlyCountFailures = false) const
		{
			return m_testTree->getAssertionCount(onlyCountFailures);
		}

		size_t getAssertionCount(PARTEST_STRING_PARAM testName, bool onlyCountFailures = false) const
		{
			const TestFrame *subtest = m_testTree->getSubtest(testName);

			if(!subtest)
				return 0;

			return subtest->getAssertionCount(onlyCountFailures);
		}

		size_t getTestFailureCount(unsigned depth = 1) const
		{
			return m_testTree->getTestFailureCount(depth);
		}

		size_t getTestSkippedCount(unsigned depth = 1) const
		{
			return m_testTree->getTestSkippedCount(depth);
		}

		size_t getTestFailureCount(PARTEST_STRING_PARAM testName, unsigned depth = 0) const
		{
			const TestFrame *subtest = m_testTree->getSubtest(testName);
			
			if(!subtest)
				return 0;

			return subtest->getTestFailureCount(depth);
		}
	};

	template<PARTEST_INVOCABLE_WITH_DEF(Func, TestContext&)>
	void TestContext::subtest(const TestInfo &testInfo, const TestFlags& flags, Func &&testFunc)
	{ m_testSuite->subtest(testInfo, flags,testFunc, m_currentFrame); }

	inline void TestContext::commitAssertion(const AssertionResult &result)
	{
		m_testSuite->commitAssertion(result, m_currentFrame);
	}

	inline void TestContext::recordLog(LogLevel level, PARTEST_STRING_PARAM type, PARTEST_STRING_PARAM message)
	{
		m_testSuite->recordLog(level, type, message, m_currentFrame);
	}

	inline void TestContext::setTestFile(PARTEST_STRING_PARAM fileName) { m_currentFrame->setTestFile(fileName); }
	inline void TestContext::setTestLine(unsigned line) { m_currentFrame->setTestLine(line); }
} // namespace partest

#endif // PARTEST_H