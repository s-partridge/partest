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
		//Replace test suite ref with a function pointer for runTest, to avoid circular dependency. This will be a function pointer to TestBase::runTest
		void (*m_runTestFunc)(TestFrame *test);

	public:
		TestContext(TestFrame *currentFrame, void (*runTestFunc)(TestFrame *test))
			: m_currentFrame(currentFrame), m_runTestFunc(runTestFunc) {
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
	protected:
		using TestContext = partest::TestContext;
		using TestFrame = partest::TestFrame;
		using TestInfo = partest::TestInfo;

	private:
		std::unique_ptr<TestFrame> m_testTree; // Dynamically growing tree of test frames
		EventEmitter m_eventEmitter; // Component that transmits events to a dispatcher
		std::atomic<bool> m_started; // Flag indicating whether the test suite has started

		static void runTest(TestFrame *test)
		{
			// There is no point where this should be null in production code.
			// If it is, it indicates a serious issue with the test framework itself.
			assert(test != nullptr && "Test was run with a null TestFrame pointer.");

			TestContext ctx(test, runTest);

			if(test->initializeTest(ctx))
			{
				test->runTestFunction(ctx);
			}

			test->finalizeTest(ctx);
			// TODO: Re-evaluate. Is this correct or necessary? I'm not sure it's the right behavior, and it might be confusing.
			// It's certainly wrong if I ever implement recurrent tests, which might be a reasonable addition.
			//if(test->hasParent())
			//	test->setTestFunction(nullptr); // Clear the function to avoid dangling references. This is only necessary for subtests because they are intended to be run immediately and then discarded.
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
			m_testTree->addSubtest(&m_eventEmitter, flags, metadata, testFunc, setupFunc, teardownFunc);
		}

		template<
			PARTEST_INVOCABLE_WITH(Func, TestContext&),
			PARTEST_INVOCABLE_WITH_OPT(SetupFunc, TestContext&),
			PARTEST_INVOCABLE_WITH_OPT(TeardownFunc, TestContext&)
		>
		void addTest(PARTEST_STRING_PARAM name, const TestFlags &flags, Func &&testFunc, SetupFunc &&setupFunc = nullptr, TeardownFunc &&teardownFunc = nullptr)
		{
			assert(!m_testTree->isRunning() && "Cannot add top-level tests while the test suite is running. Ensure tests are registered prior to calling run()");
			m_testTree->addSubtest(&m_eventEmitter, flags, TestInfo(name), testFunc, setupFunc, teardownFunc);
		}

		template<
			PARTEST_INVOCABLE_WITH(Func, TestContext&),
			PARTEST_INVOCABLE_WITH_OPT(SetupFunc, TestContext&),
			PARTEST_INVOCABLE_WITH_OPT(TeardownFunc, TestContext&)
		>
		void addTest(PARTEST_STRING_PARAM name, PARTEST_STRING_PARAM description, const TestFlags &flags, Func &&testFunc, SetupFunc &&setupFunc = nullptr, TeardownFunc &&teardownFunc = nullptr)
		{
			assert(!m_testTree->isRunning() && "Cannot add top-level tests while the test suite is running. Ensure tests are registered prior to calling run()");
			m_testTree->addSubtest(&m_eventEmitter, flags, TestInfo(name, description), testFunc, setupFunc, teardownFunc);
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
			m_started.store(false);
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
			// Only allow a run when the test suite has not already started. Once started, throw TestIntegrityError to indicate that the test suite is in an invalid state.
			if(m_started.exchange(true))
			{
				m_testTree->recordLog(LogLevel::Error, LOG_TYPE_TEST, "Attempted to run test while it was already running. A test suite can only be run once per instance.");
				return;
			}
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

		size_t getAssertionCount(const char *testName, bool onlyCountFailures = false) const
		{
			return getAssertionCount(std::string(testName), onlyCountFailures);
		}

		size_t getAssertionCount(PARTEST_STRING_PARAM testName, bool onlyCountFailures = false) const
		{
			const TestFrame *subtest = m_testTree->getSubtest(testName);

			if(!subtest)
				return 0;

			return subtest->getAssertionCount(onlyCountFailures);
		}

		size_t getAssertionCount(bool onlyCountFailures = false) const
		{
			return m_testTree->getAssertionCount(onlyCountFailures);
		}

		size_t getTestFailureCount(const char *testName, unsigned depth = 0) const
		{
			return getTestFailureCount(std::string(testName), depth);
		}

		size_t getTestFailureCount(PARTEST_STRING_PARAM testName, unsigned depth = 0) const
		{
			const TestFrame *subtest = m_testTree->getSubtest(testName);
			
			if(!subtest)
				return 0;

			return subtest->getTestFailureCount(depth);
		}

		size_t getTestFailureCount(unsigned depth = 1) const
		{
			return m_testTree->getTestFailureCount(depth);
		}

		size_t getTestSkippedCount(unsigned depth = 1) const
		{
			return m_testTree->getTestSkippedCount(depth);
		}
	};

	template<PARTEST_INVOCABLE_WITH_DEF(Func, TestContext&)>
	void TestContext::subtest(const TestInfo &testInfo, const TestFlags& flags, Func &&testFunc)
	{
		assert(m_currentFrame != nullptr && "Parent test frame is null. Subtests must be added to a valid parent test frame.");
		m_runTestFunc(m_currentFrame->addSubtest(flags, testInfo, testFunc));
	}

	inline void TestContext::commitAssertion(const AssertionResult &result)
	{
		m_currentFrame->commitAssertion(result);
	}

	inline void TestContext::recordLog(LogLevel level, PARTEST_STRING_PARAM type, PARTEST_STRING_PARAM message)
	{
		m_currentFrame->recordLog(level, type, message);
	}

	inline void TestContext::setTestFile(PARTEST_STRING_PARAM fileName)
	{
		m_currentFrame->setTestFile(fileName);
	}

	inline void TestContext::setTestLine(unsigned line)
	{
		m_currentFrame->setTestLine(line);
	}
} // namespace partest

#endif // PARTEST_H
