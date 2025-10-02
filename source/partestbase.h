// Author: Samuel Partridge
// 
// Partest is a lightweight C++ testing framework designed for simplicity and ease of use.
// It allows developers to define and run tests with minimal boilerplate code, making it ideal for quick validation of code functionality.
// Header-only implementation for easy integration into existing projects.
#ifndef PARTESTBASE_H
#define PARTESTBASE_H

#include <iostream>
#include <string>
#include <vector>
#include <functional>

#include "partestcommon.h"
#include "partesttypes.h"

/**
* Basic assertion macro for use within tests. Must be called within a TestFrame context.
*/
#define ASSERT_TRUE(condition) updateTestResult((condition) ? partest::PASSED : partest::FAILED); \
	maybeRaiseForCurrentTest(__FILE__, __LINE__, #condition);

/**
* Basic assertion macros for equality checks. Must be called within a TestFrame context.
*/
#define ASSERT_EQUAL(expected, actual) ASSERT_TRUE((expected) == (actual))
#define ASSERT_NOT_EQUAL(expected, actual) ASSERT_TRUE((expected) != (actual))

namespace partest
{
	class PartestBase; // Forward declaration

	class TestFrame
	{		
		std::vector<TestFrame *> m_subtests; // Vector of sub-tests
		TestFrame *m_parent = nullptr; // Pointer to the parent test frame
		
		std::function<void()> m_testFunction = nullptr; // Test function associated with this frame

	public:
		TestFrame() : flags(), metadata(), state() {}
		TestFrame(const TestFlags &flags, const TestInfo &metadata, const TestState &result, const std::function<void()> &testFunction = nullptr)
			: flags(flags), metadata(metadata), state(result), m_testFunction(testFunction) { }
	
		// Nothing should be moving or copying TestFrame instances. They exist as part of a tree structure managed by PartestBase.
		TestFrame(const TestFrame &) = delete; // Disable copy constructor
		TestFrame &operator=(const TestFrame &) = delete; // Disable copy assignment
		TestFrame(TestFrame &&) = delete; // Disable move constructor
		TestFrame &operator=(TestFrame &&) = delete; // Disable move assignment

		~TestFrame()
		{
			for(TestFrame *subtest : m_subtests)
			{
				if(subtest != nullptr)
				{
					delete subtest;
				}
			}
		}

		TestInfo metadata; // Test parameters including flags
		TestFlags flags; // Effective flags for this test frame
		TestState state; // Result of the test

		/**
		* Add a subtest to the current test frame.
		* 
		* @param subtest Pointer to the subtest to be added
		*/
		TestFrame *addSubtest(std::unique_ptr<TestFrame> subtest)
		{
			m_subtests.push_back(subtest.release());
			m_subtests.back()->m_parent = this;

			return m_subtests.back();
		}

		/**
		* Get the parent test frame.
		* 
		* @return non-owning pointer to the parent TestFrame, or nullptr if this is the root frame.
		*/
		TestFrame *getParent() const { return m_parent; }

		/**
		* Iterator access for subtests
		*/
		std::vector<TestFrame *>::iterator subtestsBegin() { return m_subtests.begin(); }
		std::vector<TestFrame *>::iterator subtestsEnd() { return m_subtests.end(); }
		size_t subtestCount() const { return m_subtests.size(); }
		std::vector<TestFrame *>::const_iterator subtestsBegin() const { return m_subtests.cbegin(); }
		std::vector<TestFrame *>::const_iterator subtestsEnd() const { return m_subtests.cend(); }

		bool hasTestFunction() const { return m_testFunction != nullptr; }

		/**
		* Run the test function associated with this test frame, if one is set.
		*
		* @throws std::runtime_error if no test function is set.
		*/
		void runTestFunction()
		{
			if(m_testFunction != nullptr)
			{
				m_testFunction();
			}
			else
			{
				throw std::runtime_error("Attempted to run a test function that is not set.");
			}
		}

		/**
		* Get the effective flags for this test frame, resolving any INHERIT values from parent frames.
		* 
		* @return The effective TestFlags for this test frame.
		*/
		TestFlags getEffectiveFlags() const
		{
			// If the current flags are not fully resolved, inherit from parent
			if(m_parent != nullptr && !flags.isResolved())
			{
				// Inherit from parent.
				TestFlags parentFlags = m_parent->getEffectiveFlags();
				return flags.mergeWithParentFlags(parentFlags);
			}
			else
			{
				return flags;
			}
		}
	};

	/**
	* Base class for all partest tests.
	*
	*/
	class PartestBase
	{
	private:
		std::unique_ptr<TestFrame> m_testTree; // Dynamically growing tree of test frames

		TestFrame *m_currentFrame; // Pointer to the current test frame

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
		*/
		void addTest(const TestInfo metadata, const TestFlags &flags, const std::function<void()> &testFunc)
		{
			std::unique_ptr<TestFrame> newFrame = std::make_unique<TestFrame>(flags, metadata, TestState::defaultState(), testFunc);

			// Add the new test frame as a subtest of the root test frame
			// Adding it to the root ensures that all tests are organized under a single parent, simplifying management and execution
			m_testTree->addSubtest(std::move(newFrame));
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
		void updateTestResult(TestResult result) { m_currentFrame->state.updateResult(result); }

		/**
		* Finalize the current test frame and return to the specified target frame. Called internally on improper test completion, such as unexpected exceptions.
		* 
		* @param targetFrame The target TestFrame to finalize to. Typically the parent of the current frame.
		* @throws TestIntegrityFailure if unable to finalize to the target frame.
		*/
		void finalizeToFrame(const TestFrame *targetFrame)
		{
			while(m_currentFrame != targetFrame)
			{
				finalizeSubtest();

				if(m_currentFrame == nullptr || m_currentFrame == m_testTree.get())
				{
					throw TestIntegrityFailure("Error: Unrecoverable error in test framework. Could not finalize to target frame " + targetFrame->metadata.name + ".");
				}
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

	public:
		PartestBase(const std::string &name, const std::string &description, const TestFlags &flags = TestFlags::defaultDisabled())
		{
			// Initialize the root test frame. This frame is not associated with any specific test but serves as the root of the test tree.
			// Its primary purpose is to contain information such as the overall test suite name and description in the same collection as the individual tests.
			m_testTree = std::make_unique<TestFrame>(flags, TestInfo(name, description), TestState::defaultState());
		
			// Set the current frame to the root test frame initially
			m_currentFrame = m_testTree.get();
		}
		virtual ~PartestBase() = default;

		PartestBase(const PartestBase &) = delete; // Disable copy constructor
		PartestBase &operator=(const PartestBase &) = delete; // Disable copy assignment
		PartestBase(PartestBase &&) = delete; // Disable move constructor
		PartestBase &operator=(PartestBase &&) = delete; // Disable move assignment

		void setName(const std::string &name) { m_testTree->metadata.name = name; }
		std::string getName() const { return m_testTree->metadata.name; }

		void setDescription(const std::string &description) { m_testTree->metadata.description = description; }
		std::string getDescription() const { return m_testTree->metadata.description; }
		
		void setFlags(const TestFlags &flags) { m_testTree->flags.setFlags(flags); }
		const TestFlags &getFlags() const { return m_testTree->flags; }

		/**
		* Setup function to be overridden by derived classes
		*/
		virtual void setup() {}

		/**
		* Teardown function to be overridden by derived classes
		*/
		virtual void teardown() {}

		/**
		* Initialize a new subtest within the current test frame.
		* 
		* @param newFlags Flags specific to this subtest, which can override global or current flags. Use TestFlags::defaultInherit() to inherit all flags.
		* @param newMetadata Metadata for the subtest, including name and description. Use TestInfo::defaultInfo() for default values.
		*/
		bool initializeSubtest(const TestFlags& newFlags = TestFlags::defaultInherit(), const TestInfo &newMetadata = TestInfo::defaultInfo())
		{
			std::unique_ptr<TestFrame>newSubtest = std::make_unique<TestFrame>(newFlags, newMetadata, TestState::defaultState());

			// If effective flags indicate the test should be skipped, mark it as skipped immediately
			m_currentFrame = m_currentFrame->addSubtest(std::move(newSubtest));

		    if(m_currentFrame->getEffectiveFlags().skip == ENABLED)
			{
				m_currentFrame->state.updateStatus(SKIPPED);
				return false;
			}
			else
			{
				m_currentFrame->state.updateStatus(RUNNING);
			}

			return true;
		}

		/**
		* Check if the current test should raise an assertion failure based on its status and flags. Used in ASSERT macros.
		* 
		* @param file The file where the assertion is being checked. Typically provided by the __FILE__ macro.
		* @param line The line number where the assertion is being checked. Typically provided by the __LINE__ macro.
		* @param condition The condition being asserted, as a string. Typically provided by the condition expression itself.
		* @throws AssertionFailure if the current test has failed and stopOnFail is enabled.
		*/
		void maybeRaiseForCurrentTest(const char *file, int line, const std::string &condition)
		{
			if(m_currentFrame->getEffectiveFlags().stopOnFail == ENABLED && (m_currentFrame->state.getResult() == FAILED || m_currentFrame->state.getResult() == MIXED))
			{
				throw AssertionFailure(file, line, "Assertion failed: " + condition + " in test '" + m_currentFrame->metadata.name + "'.");
			}
		}

		/**
		* Finalize the current subtest and return to the parent test frame.
		* 
		* @throws TestIntegrityFailure if the current frame has no parent (i.e., is the root frame).
		*/
		void finalizeSubtest()
		{
			if(m_currentFrame->getParent() != nullptr)
			{
				TestStatus subtestStatus = m_currentFrame->state.getStatus();
				TestResult subtestResult = m_currentFrame->state.getResult();

				m_currentFrame = m_currentFrame->getParent();
				m_currentFrame->state.updateStatus(subtestStatus);

				if(subtestStatus != ABORTED)
					m_currentFrame->state.updateStatus(COMPLETED);
				
				m_currentFrame->state.updateResult(subtestResult);
			}
			else
			{
				throw TestIntegrityFailure("Error: Unrecoverable error in test framework. Attempted to finalize a subtest with no parent. Current frame: " + m_currentFrame->metadata.name + ".");
			}
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
			// Enter a new subtest context with the specified flags and metadata
			// If initialization fails (e.g., due to skip flag), exit the subtest early.
			if(initializeSubtest(flags, testInfo))
			{
				try
				{
					m_currentFrame->state.updateStatus(RUNNING);
					testFunc();
				}
				// Catch assertion failures to prevent them from propagating outside the subtest.
				// Assertion failures are only thrown by ASSERT macros, and only when stopOnFail is enabled.
				
				// Assertion failures indicate that the test has already been marked as FAILED, so no additional action is needed here.
				#pragma warning(suppress:4101) 
				catch(const partest::AssertionFailure &e) {	}
			}
			// Whether the test ran successfully or not, the subtest stack needs to be rolled up.
			finalizeSubtest();
			maybeRaiseForCurrentTest(__FILE__, __LINE__, "Stopped on subtest failure");
		}

		/**
		* Public function to run all registered tests.
		* Run all registered tests, calling setup and teardown functions before and after.
		*/
		void runTests()
		{
			// Call setup function
			setup();
			
			// Iterate through all registered tests
			for(std::vector<TestFrame*>::iterator test = m_testTree->subtestsBegin(); test != m_testTree->subtestsEnd(); ++test)
			{
				// Set the current frame to the test being executed
				m_currentFrame = *test;

				// If the test is marked to be skipped, update its status and continue to the next test
				if(m_currentFrame->getEffectiveFlags().skip == ENABLED)
				{
					m_currentFrame->state.updateStatus(SKIPPED);
					continue;
				}

				try
				{
					// Execute the test function and aggregate the result
					m_currentFrame->state.updateStatus(RUNNING);
					m_currentFrame->runTestFunction();
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
					m_currentFrame->state.updateStatus(ABORTED);
					m_currentFrame->state.updateResult(FAILED);

					std::cerr << "Error: Exception in test '" << m_currentFrame->metadata.name << "': " << e.what() << std::endl;
				}
				catch(...)
				{
					// An unknown exception occurred during test execution.
					// Mark the test as aborted and log a generic message.
					m_currentFrame->state.updateStatus(ABORTED);
					m_currentFrame->state.updateResult(FAILED);
					std::cerr << "Error: Unknown exception in test '" << m_currentFrame->metadata.name << "'." << std::endl;
				}

				// Ensure we finalize back to the current test frame in case of improper test completion.
				finalizeToFrame(*test);

				// If the test failed and stopOnFail is enabled, stop executing further tests
				if(m_currentFrame->state.getResult() == FAILED && m_currentFrame->getEffectiveFlags().stopOnFail == ENABLED)
				{
					std::cout << "Stopping further tests due to failure in test '" << m_currentFrame->metadata.name << "' with stopOnFail enabled." << std::endl;
					break;
				}
			}

			// Call teardown function
			teardown();

			// Reset current frame to root after tests
			m_currentFrame = m_testTree.get();
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