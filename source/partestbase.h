// Author: Samuel Partridge
// 
// Partest is a lightweight C++ testing framework designed for simplicity and ease of use.
// It allows developers to define and run tests with minimal boilerplate code, making it ideal for quick validation of code functionality.
// Header-only implementation for easy integration into existing projects.
#ifndef PARTESTBASE_H
#define PARTESTBASE_H

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cassert>
#include <functional>

#include "partestcommon.h"
#include "partesttypes.h"
#include "partesttestframe.h"

/**
* Basic assertion macro for use within tests. Must be called within a TestFrame context.
*/
#define ASSERT_TRUE(condition) handleAssertTrue((condition), #condition, __FILE__, __LINE__)

/**
* Basic assertion macros for equality checks. Must be called within a TestFrame context.
*/
#define ASSERT_EQUAL(expected, actual) handleAssertEqual((expected), (actual), #expected, #actual, __FILE__, __LINE__)
#define ASSERT_NOT_EQUAL(expected, actual) handleAssertNotEqual((expected), (actual), #expected, #actual, __FILE__, __LINE__)

namespace partest
{
	/**
	* Base class for all partest tests.
	*/
	class PartestBase
	{
	private:
		std::unique_ptr<TestFrame> m_testTree; // Dynamically growing tree of test frames

		TestFrame *m_currentFrame; // Pointer to the current test frame

		void runTest(TestFrame *test) noexcept
		{
			// There is no point where this should be null in production code.
			// If it is, it indicates a serious issue with the test framework itself.
			assert(test != nullptr && "Test was run with a null TestFrame pointer.");

			m_currentFrame = test;
			if(m_currentFrame->initializeTest())
			{
				try
				{
					test->runTestFunction();
				}
				// A test returned early due to an assertion failure with stopOnFail enabled.
				// Nothing special to do here, but this is necessary to prevent the exception from propagating further.

				// Assertion failures indicate that the test has already been marked as FAILED, so no additional action is needed here.
				#pragma warning(suppress:4101) 
				catch(const partest::AssertionFailure &e) {	}

				catch(const std::exception &e)
				{
					// An unexpected exception occurred during test execution.
					// Mark the test as aborted and log a generic message.
					test->state.updateStatus(ABORTED);
					test->state.updateResult(FAILED);

					std::cerr << "Error: Exception in test '" << m_currentFrame->metadata.name << "': " << e.what() << std::endl;
				}
				catch(...)
				{
					// An unknown exception occurred during test execution.
					// Mark the test as aborted and log a generic message.
					test->state.updateStatus(ABORTED);
					test->state.updateResult(FAILED);
					std::cerr << "Error: Unknown exception in test '" << m_currentFrame->metadata.name << "'." << std::endl;
				}
			}
			m_currentFrame = test->finalizeTest();
		}

		/**
		* Public function to run all registered tests.
		* Run all registered tests, calling setup and teardown functions before and after.
		*/
		void runBaseTests()
		{
			// Iterate through all registered tests
			for(std::vector<TestFrame*>::iterator test = m_currentFrame->subtestsBegin(); test != m_currentFrame->subtestsEnd(); ++test)
			{
				runTest(*test);

				// If the test failed and stopOnFail is enabled, stop executing further tests
				if(m_currentFrame->state.getResult() == FAILED && m_currentFrame->getEffectiveFlags().stopOnFail == ENABLED)
				{
					std::cout << "Stopping further tests due to failure in test '" << m_currentFrame->metadata.name << "' with stopOnFail enabled." << std::endl;
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
		void addTest(const TestInfo metadata, const TestFlags &flags, const std::function<void()> &testFunc, const std::function<void()> &setupFunc = nullptr, const std::function<void()> &teardownFunc = nullptr)
		{
			TestFrame *newFrame = m_testTree->addSubtest(std::make_unique<TestFrame>(flags, metadata, TestState::defaultState(), testFunc, setupFunc, teardownFunc));
		}

		template<PARTEST_ENABLE_IF_INVOCABLE(Func)>
		void subtest(Func &&testFunc) { subtest(TestInfo::defaultInfo(), TestFlags::defaultInherit(), testFunc); }

		template<PARTEST_ENABLE_IF_INVOCABLE(Func)>
		void subtest(const TestFlags& flags, Func &&testFunc) { subtest(TestInfo::defaultInfo(), flags, testFunc); }

		template<PARTEST_ENABLE_IF_INVOCABLE(Func)>
		void subtest(const TestInfo &testInfo, Func &&testFunc) { subtest(testInfo, TestFlags::defaultInherit(), testFunc); }

		template<PARTEST_ENABLE_IF_INVOCABLE(Func)>
		void subtest(const TestInfo &testInfo, const TestFlags& flags, Func &&testFunc)
		{
			TestFrame *subtest = m_currentFrame->addSubtest(std::make_unique<TestFrame>(flags, testInfo, TestState::defaultState(), testFunc));
			runTest(subtest);
			subtest->setTestFunction(nullptr); // Clear the function to avoid dangling references. This is only necessary for subtests because they are intended to be run immediately and then discarded.

			maybeRaiseForCurrentTest(__FILE__, __LINE__, "Stopped on subtest failure");
		}

		// Current test frame accessors
		/**
		* Get the current test frame.
		* @return A constant reference to the current TestFrame.
		*/
		const TestFrame &getCurrentFrame() const { return *m_currentFrame; }

		/**
		* Get the effective flags of the current test frame.
		* @return The effective TestFlags of the current test frame.
		*/
		TestFlags getCurrentFlags() const { return m_currentFrame->getEffectiveFlags(); }
		void updateTestResult(TestResult result, PARTEST_STRING_PARAM log)
		{
			m_currentFrame->state.updateResult(result);
			m_currentFrame->log(log);
		}

		/**
		* Check if the current test should raise an assertion failure based on its status and flags. Used in ASSERT macros.
		* 
		* @param file The file where the assertion is being checked. Typically provided by the __FILE__ macro.
		* @param line The line number where the assertion is being checked. Typically provided by the __LINE__ macro.
		* @param condition The condition being asserted, as a string. Typically provided by the condition expression itself.
		* @throws AssertionFailure if the current test has failed and stopOnFail is enabled.
		*/
		void maybeRaiseForCurrentTest(const char *file, int line, PARTEST_STRING_PARAM condition)
		{
			std::cout << condition << std::endl;

			if(m_currentFrame->getEffectiveFlags().stopOnFail == ENABLED && (m_currentFrame->state.getResult() == FAILED || m_currentFrame->state.getResult() == MIXED))
			{
				std::cout << "Stopped test early due to failure in " << m_currentFrame->metadata.name << std::endl;
				m_currentFrame->log("Stopped test early due to failure.");
				throw AssertionFailure(file, line, "Assertion failed: " + condition + " in test '" + m_currentFrame->metadata.name + "'.");
			}
		}

		void logAndMaybeRaiseForCurrentFrame(const char *file, int line, PARTEST_STRING_PARAM message)
		{
			std::cout << message << std::endl;
			if(m_currentFrame->getEffectiveFlags().stopOnFail == ENABLED && (m_currentFrame->state.getResult() == FAILED || m_currentFrame->state.getResult() == MIXED))
			{
				std::cout << "Stopped test early due to failure in " << m_currentFrame->metadata.name << std::endl;
				m_currentFrame->log("Stopped test early due to failure.");
				throw AssertionFailure(file, line, message);
			}
		}

		/**
		* Recursively print the test tree starting from the given frame, with indentation based on depth.
		* @param frame The TestFrame to start printing from.
		* @param depth The current depth in the tree, used for indentation. Default is 0.
		*/
		const void printTestTree(const TestFrame &frame, unsigned depth = 0) const
		{
			std::string depthPrefix = std::string(depth, '\t');
			std::cout << depthPrefix << "Test '" << frame.metadata.name << ": " << frame.state << std::endl;

			depth++;
			// Recursively print subtests
			for(auto frameIter = frame.subtestsBegin(); frameIter != frame.subtestsEnd(); ++frameIter)
			{
				printTestTree(**frameIter, depth);
			}
		}

		/**
		* Setup function to be overridden by derived classes
		*/
		virtual void setup() {}

		/**
		* Teardown function to be overridden by derived classes
		*/
		virtual void teardown() {}

		////////////////
		// Assertions //
		////////////////
		template<typename T>
		void handleAssertTrue(const T &condition, const char *conditionStr, const char *file, int line)
		{
			std::ostringstream message;
			if(condition)
			{
				message << "ASSERT_TRUE(" << conditionStr << ") passed.";
				updateTestResult(PASSED, message.str());
				return;
			}

			message << "Assertion failed at " << file << ":" << line
				<< ": ASSERT_TRUE(" << conditionStr << ") was false.\n";

			updateTestResult(FAILED, message.str());
			maybeRaiseForCurrentTest(file, line, message.str());
		}

		template<typename T, typename U>
		void handleAssertEqual(const T &expected, const U &actual, const char *expectedStr, const char *actualStr, const char *file, int line)
		{
			std::ostringstream message;

			if(expected == actual)
			{
				message << "ASSERT_EQUAL(" << expectedStr << ", " << actualStr << ") passed.";
				updateTestResult(PASSED, message.str());
				return;
			}

			message << "Assertion failed at " << file << ":" << line
				<< ": ASSERT_EQUAL(" << expectedStr << ", " << actualStr << ")\n"
				<< "  Expected: " << expected << "\n"
				<< "  Actual:   " << actual << "\n";

			updateTestResult(FAILED, message.str());
			maybeRaiseForCurrentTest(file, line, message.str());
		}

		void handleAssertEqual(const char* expected, const char* actual, const char* expectedStr, const char* actualStr, const char* file, int line)
		{
			std::ostringstream message;

			if (expected != nullptr && actual != nullptr && strcmp(expected, actual) == 0
			    || expected == nullptr && actual == nullptr)
			{
				message << "ASSERT_EQUAL(" << expectedStr << ", " << actualStr << ") passed.";
				updateTestResult(PASSED, message.str());
				return;
			}

			message << "Assertion failed at " << file << ":" << line
				<< ": ASSERT_EQUAL(" << expectedStr << ", " << actualStr << ")\n"
				<< "  Expected: \"" << expected << "\"\n"
				<< "  Actual:   \"" << actual << "\"\n";
			updateTestResult(FAILED, message.str());
			maybeRaiseForCurrentTest(file, line, message.str());
		}

		template<typename T, typename U>
		void handleAssertNotEqual(const T &expected, const U &actual, const char *expectedStr, const char *actualStr, const char *file, int line)
		{
			std::ostringstream message;

			if(expected != actual)
			{
				message << "ASSERT_NOT_EQUAL(" << expectedStr << ", " << actualStr << ") passed.";
				updateTestResult(PASSED, message.str());
				return;
			}

			message << "Assertion failed at " << file << ":" << line
				<< ": ASSERT_NOT_EQUAL(" << expectedStr << ", " << actualStr << ")\n"
				<< "\"" << actualStr << "\" should not have been " << expected << "\n";

			updateTestResult(FAILED, message.str());
			maybeRaiseForCurrentTest(file, line, message.str());
		}


		void handleAssertNotEqual(const char* expected, const char* actual, const char* expectedStr, const char* actualStr, const char* file, int line)
		{
			std::ostringstream message;

			if(expected != actual)
			{
				message << "ASSERT_NOT_EQUAL(" << expectedStr << ", " << actualStr << ") passed.";
				updateTestResult(PASSED, message.str());
				return;
			}

			message << "Assertion failed at " << file << ":" << line
				<< ": ASSERT_NOT_EQUAL(" << expectedStr << ", " << actualStr << ")\n"
				<< "\"" << actualStr << "\" should not have been " << expected << "\n";

			updateTestResult(FAILED, message.str());
			maybeRaiseForCurrentTest(file, line, message.str());
		}


	public:
		PartestBase(PARTEST_STRING_PARAM name, PARTEST_STRING_PARAM description, const TestFlags &flags = TestFlags::defaultDisabled())
		{
			// Initialize the root test frame. This frame is not associated with any specific test but serves as the root of the test tree.
			// Its primary purpose is to contain information such as the overall test suite name and description in the same collection as the individual tests.
			m_testTree = std::make_unique<TestFrame>(flags, TestInfo(name, description), TestState::defaultState());
			// Set the setup and teardown functions for the root test frame
			m_testTree->setSetupFunction([this]() { this->setup(); });
			m_testTree->setTestFunction([this]() { this->runBaseTests(); });
			m_testTree->setTeardownFunction([this]() { this->teardown(); });

			m_currentFrame = nullptr; // No current frame until tests are run
		}
		virtual ~PartestBase() = default;

		PartestBase(const PartestBase &) = delete; // Disable copy constructor
		PartestBase &operator=(const PartestBase &) = delete; // Disable copy assignment
		PartestBase(PartestBase &&) = delete; // Disable move constructor
		PartestBase &operator=(PartestBase &&) = delete; // Disable move assignment

		void setName(PARTEST_STRING_PARAM name) { m_testTree->metadata.name = name; }
		const std::string &getName() const { return m_testTree->metadata.name; }

		void setDescription(PARTEST_STRING_PARAM description) { m_testTree->metadata.description = description; }
		const std::string &getDescription() const { return m_testTree->metadata.description; }
		
		void setFlags(const TestFlags &flags) { m_testTree->flags.setFlags(flags); }
		const TestFlags &getFlags() const { return m_testTree->flags; }

		void run()
		{
			runTest(m_testTree.get());
		}

		/**
		* Print the entire test tree, including all tests and their statuses.
		*/
		void printTestTree() const
		{
			printTestTree(*m_testTree);
		}
	};
} // namespace partest

#endif // PARTEST_H