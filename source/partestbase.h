// File: partestbase.h
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

#include "partestcommon.h"
#include "partesttypes.h"
#include "partestlog.h"
#include "partestassert.h"
#include "partesttestframe.h"
#include "partestexceptions.h"

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

		void runTest(TestFrame *test)
		{
			// There is no point where this should be null in production code.
			// If it is, it indicates a serious issue with the test framework itself.
			assert(test != nullptr && "Test was run with a null TestFrame pointer.");

			m_currentFrame = test;
			if(m_currentFrame->initializeTest())
			{
				std::stringstream resultStream;
				try
				{
					m_currentFrame->runTestFunction();
				}
				// A test returned early due to an assertion failure with stopOnFail enabled.
				// Nothing special to do here, but this is necessary to prevent the exception from propagating further.

				// Assertion failures indicate that the test has already been marked as Failed, so no additional action is needed here.
				#pragma warning(suppress:4101) 
				catch(const partest::AssertionFailure &e)
				{
					resultStream << "Stopped test early due to failure in " << m_currentFrame->metadata.name << std::endl;

					if(e.file() != "" && e.line() != 0)
						resultStream << "At: " << e.file() << ":" << e.line() << std::endl;

					m_currentFrame->log(LogLevel::Info, PARTEST_LOG_TYPE_TEST, resultStream.str());
				}

				catch(const std::exception &e)
				{
					// An unexpected exception occurred during test execution.
					// Mark the test as aborted and log a generic message.
					m_currentFrame->updateStatus(TestStatus::Aborted);
					// Ensure that the test result was set. A generic exception indicates test failure.
					m_currentFrame->updateResult(TestResult::Failed);

					resultStream << "Error: Unhandled exception in test '" << m_currentFrame->metadata.name << "': " << e.what() << std::endl;
					m_currentFrame->log(LogLevel::Info, PARTEST_LOG_TYPE_DEFAULT, resultStream.str());
				}
				catch(...)
				{
					// An unknown exception occurred during test execution.
					// Mark the test as aborted and log a generic message.
					m_currentFrame->updateStatus(TestStatus::Aborted);
					// Ensure that the test result was set. A generic exception indicates test failure.
					m_currentFrame->updateResult(TestResult::Failed);

					resultStream << "Error: Unknown exception in test '" << m_currentFrame->metadata.name << "'." << std::endl;
					m_currentFrame->log(LogLevel::Error, PARTEST_LOG_TYPE_DEFAULT, resultStream.str());
				}

				m_currentFrame->finalizeTest();
			}
			
			m_currentFrame = m_currentFrame->getParent();
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
				if(m_currentFrame->getEffectiveFlags().stopOnFail == FlagState::Enabled && m_currentFrame->hasFailures())
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
			m_testTree->addSubtest(partest::make_unique<TestFrame>(flags, metadata, TestState::defaultState(), testFunc, setupFunc, teardownFunc));
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
			TestFrame *subtest = m_currentFrame->addSubtest(partest::make_unique<TestFrame>(flags, testInfo, TestState::defaultState(), testFunc));
			runTest(subtest);
			subtest->setTestFunction(nullptr); // Clear the function to avoid dangling references. This is only necessary for subtests because they are intended to be run immediately and then discarded.

			maybeRaiseOnAssertion("", 0, "Stopped on failure in " + m_currentFrame->metadata.name);
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

		/**
		* Log an assertion result.
		* @param result The assertion result to log.
		*/
		void logAssertion(const AssertionResult &result)
		{
			m_currentFrame->updateResult(result.passed ? TestResult::Passed : TestResult::Failed);
			m_currentFrame->logAssertion(result);
		}

		/**
		* Log an assertion result.
		* @param passed Whether the assertion passed or failed.
		* @param log The log message to associate with the assertion result.
		*/
		void logAssertion(bool passed, PARTEST_STRING_PARAM log)
		{
			m_currentFrame->updateResult(passed ? TestResult::Passed : TestResult::Failed);
			m_currentFrame->log(LogLevel::Info, PARTEST_LOG_TYPE_ASSERT, log);
		}

		/**
		* Log a message.
		* @param level The log level.
		* @param type The log type.
		* @param message The log message.
		*/
		void addLog(LogLevel level, PARTEST_STRING_PARAM type, PARTEST_STRING_PARAM message)
		{
			m_currentFrame->log(level, type, message);
		}

		/**
		* Check if the current test should raise an assertion failure based on its status and flags. Used in ASSERT macros.
		* 
		* @param file The file where the assertion is being checked. Typically provided by the __FILE__ macro.
		* @param line The line number where the assertion is being checked. Typically provided by the __LINE__ macro.
		* @param condition The condition being asserted, as a string. Typically provided by the condition expression itself.
		* @throws AssertionFailure if the current test has failed and stopOnFail is enabled.
		*/
		void maybeRaiseOnAssertion(const char *file, int line, PARTEST_STRING_PARAM condition)
		{
			if(m_currentFrame->getEffectiveFlags().stopOnFail == FlagState::Enabled && (m_currentFrame->hasFailures()))
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
		inline void maybeRaiseOnAssertion(const AssertionResult &result) { maybeRaiseOnAssertion(result.file.c_str(), result.line, result.condition); }

		/**
		* Recursively print the test tree starting from the given frame, with indentation based on depth.
		* @param frame The TestFrame to start printing from.
		* @param depth The current depth in the tree, used for indentation. Default is 0.
		*/
		void printTestTree(const TestFrame &frame, unsigned depth = 0) const
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

		/**
		* Process an evaluated assertion. Log it and raise an exception if necessary.
		* 
		* @param result Output of an evaluated assertion. AssertionResults should be produced by assertion handlers.
		* @throws AssertionFailure if the assertion result did not pass and stopOnFail is enabled.
		*/
		void commitAssertion(const AssertionResult &result)
		{
			
 			// Store the assertion result
 			logAssertion(result);
			// Old style log for compatibility, for now
			logAssertion(result.passed, result.message);

			// On failure, allow an exception to be raised if the current test frame is configured to do so.
			if(!result.passed)
				maybeRaiseOnAssertion(result.file.c_str(), result.line, result.condition);
		}

	public:
		PartestBase(PARTEST_STRING_PARAM name, PARTEST_STRING_PARAM description, const TestFlags &flags = TestFlags::defaultDisabled())
		{
			// Initialize the root test frame. This frame is not associated with any specific test but serves as the root of the test tree.
			// Its primary purpose is to contain information such as the overall test suite name and description in the same collection as the individual tests.
			m_testTree = partest::make_unique<TestFrame>(flags, TestInfo(name, description), TestState::defaultState());
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

		std::vector<LogEntry> aggregateLogs(TestFrame *frame, LogLevel maxLevel, unsigned depth = 1)
		{
			std::vector<LogEntry> entries;

			for(const LogEntry &entry : frame->getLogs())
			{
				if(entry.level <= maxLevel)
				{
					entries.push_back(entry);
				}
			}

			//Incomplete
		}

		void printLogs(LogLevel maxLevel = LogLevel::Info, unsigned maxDepth = 1)
		{
			printLogs(m_testTree.get(), maxLevel, maxDepth, 0);
		}

		void printLogs(TestFrame *frame, LogLevel maxLevel = LogLevel::Info, unsigned maxDepth = 1, unsigned depth = 0)
		{
			std::vector<LogEntry> entries;

			for(const LogEntry &entry : frame->getLogs())
			{
				std::cout << entry.message << std::endl;

				if(depth < maxDepth && entry.type == PARTEST_LOG_TYPE_SUBTEST && entry.testFrameID != 0)
				{
					auto subtest = frame->subtestsBegin();

					while(subtest != frame->subtestsEnd())
					{
						if((*subtest)->id() == entry.testFrameID)
						{
							printLogs(*subtest, maxLevel, maxDepth, depth + 1);
							break;
						}
						subtest++;
					}
				}
			}
		}

		void aggregateLogs(LogLevel maxLevel, unsigned depth = 1)
		{
			std::vector<LogEntry> entries;

			TestFrame *frame = m_testTree.get();

			for(const LogEntry &entry : frame->getLogs())
			{
				if(entry.level <= maxLevel)
				{
					entries.push_back(entry);
				}
			}

			//Incomplete
		}

		unsigned getTestFailureCount(unsigned depth = 1) const
		{
			const TestFrame *frame = m_testTree.get();

			return frame->getTestFailureCount(depth);
		}

		unsigned getAssertionFailureCount() const
		{
			const TestFrame *frame = m_testTree.get();

			return frame->getAssertionFailureCount();
		}
	};
} // namespace partest

#endif // PARTEST_H