// Author: Samuel Partridge
// 
// Partest is a lightweight C++ testing framework designed for simplicity and ease of use.
// It allows developers to define and run tests with minimal boilerplate code, making it ideal for quick validation of code functionality.
// Header-only implementation for easy integration into existing projects.
#ifndef PARTEST_H
#define PARTEST_H

#include <iostream>
#include <string>
#include <vector>
#include <functional>

#include "partesttypes.h"

/**
* Macro to define a subtest block with specific flags. Must be used within a TestFrame context.
*
* @param testInfo Metadata for the subtest, including name and description. Use TestInfo::defaultInfo() for default values.
* @param flags Flags specific to this subtest, which can override global or current flags. Use TestFlags::defaultInherit() to inherit all flags.
*/
#define SUBTEST(testInfo, flags) for(bool _subtest_once = initializeSubtest(flags, testInfo); _subtest_once; _subtest_once = finalizeSubtest())

/**
* Basic assertion macro for use within tests. Must be used within a TestFrame context.
*/
#define ASSERT_TRUE(condition) updateTestStatus((condition) ? partest::PASSED : partest::FAILED)

namespace partest
{
	class PartestBase; // Forward declaration

	class TestFrame
	{		
		std::vector<TestFrame *> m_subtests; // Vector of sub-tests
		TestFrame *m_parent = nullptr; // Pointer to the parent test frame

	public:
		TestFrame() : flags(), metadata(), result() {}
		TestFrame(const TestFlags &flags, const TestInfo &metadata, const TestResult &result)
			: flags(flags), metadata(metadata), result(result) { }
	
		// Nothing should be moving or copying TestFrame instances. They exist as part of a tree structure managed by PartestBase.
		TestFrame(const TestFrame &) = delete; // Disable copy constructor
		TestFrame &operator=(const TestFrame &) = delete; // Disable copy assignment
		TestFrame(TestFrame &&) = delete; // Disable move constructor
		TestFrame &operator=(TestFrame &&) = delete; // Disable move assignment

		~TestFrame()
		{
			for(TestFrame *subtest : m_subtests)
				if(subtest != nullptr)
				{
					delete subtest;
				}
		}

		TestInfo metadata; // Test parameters including flags
		TestFlags flags; // Effective flags for this test frame
		TestResult result; // Result of the test

		/**
		* Add a subtest to the current test frame.
		* 
		* @param subtest Pointer to the subtest to be added
		*/
		TestFrame *addSubtest(std::unique_ptr<TestFrame> &subtest)
		{
			m_subtests.push_back(subtest.release());
			m_subtests.back()->m_parent = this;

			return m_subtests.back();
		}

		/**
		* Get the parent test frame.
		*/
		TestFrame *getParent() const { return m_parent; }

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
				return flags.getEffectiveFlags(parentFlags);
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
		// Vector of test functions and their parameters
		std::vector<std::pair<TestFrame *, std::function<void()>>> m_tests;
		std::unique_ptr<TestFrame> m_testTree; // Dynamically growing tree of test frames
		std::vector<TestResult> m_results; // Vector of test results

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
			std::unique_ptr<TestFrame> newFrame = std::make_unique<TestFrame>(flags, metadata, TestResult::defaultResult());

			// Add the new test frame as a subtest of the root test frame
			// Adding it to the root ensures that all tests are organized under a single parent, simplifying management and execution
			TestFrame *baseTestFrame = m_testTree->addSubtest(std::move(newFrame));

			m_tests.emplace_back(baseTestFrame, testFunc);
		}

		void addResult(const TestStatus &status, const std::string &testName)
		{
			std::string message = "Test '" + testName;
			switch(status)
			{
			case PASSED:
				message += "' passed successfully.";
				break;
			case MIXED:
				message +=  "' had mixed results.";
				break;
			case FAILED:
				message += "' failed.";
				break;
			case SKIPPED:
				message += "' was skipped.";
				break;
			default:
				message += "' is awaiting execution.";
				break;
			}
			m_results.emplace_back(status, message);
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
		void updateTestStatus(TestStatus result) { m_currentFrame->result.updateStatus(result); }

	public:
		PartestBase(const std::string &name, const std::string &description, const TestFlags &flags = TestFlags::defaultDisabled())
		{
			// Initialize the root test frame. This frame is not associated with any specific test but serves as the root of the test tree.
			// Its primary purpose is to contain information such as the overall test suite name and description in the same collection as the individual tests.
			m_testTree = std::make_unique<TestFrame>(flags, TestInfo(name, description), TestResult::defaultResult());
		
			// Set the current frame to the root test frame initially
			m_currentFrame = m_testTree.get();
		}
		virtual ~PartestBase() = default;

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

		bool initializeSubtest(const TestFlags& newFlags = TestFlags::defaultInherit(), const TestInfo &newMetadata = TestInfo::defaultInfo())
		{
			std::unique_ptr<TestFrame>newSubtest = std::make_unique<TestFrame>(newFlags, newMetadata, TestResult::defaultResult());
			m_currentFrame = m_currentFrame->addSubtest(std::move(newSubtest));

			// Always return true to ensure the for loop in the SUBTEST macro executes exactly once
			return true;
		}

		bool finalizeSubtest()
		{
			if(m_currentFrame->getParent() != nullptr)
			{
				TestStatus subtestStatus = m_currentFrame->result.status;
				m_currentFrame = m_currentFrame->getParent();
				m_currentFrame->result.updateStatus(subtestStatus);
			}
			else
			{
				std::cerr << "Warning: Attempted to finalize subtest but already at root test frame." << std::endl;
			}

			// Always return false to ensure the for loop in the SUBTEST macro executes exactly once
			return false;
		}

		void runTests()
		{
			// Call setup function
			setup();
			
			// Iterate through all registered tests
			for(const std::pair<TestFrame *, std::function<void()>> &test : m_tests)
			{
				// Set the current frame to the test being executed
				m_currentFrame = test.first;
				// Execute the test function and aggregate the result
				test.second();
				addResult(m_currentFrame->result.status, m_currentFrame->metadata.name);
			}

			// Call teardown function
			teardown();

			// Reset current frame to root after tests
			m_currentFrame = m_testTree.get();
		}

		/**
		* Get the results of all executed tests.
		* 
		* @return A constant reference to a vector of TestResult objects representing the results of all executed tests.
		*/
		const std::vector<TestResult> &getResults() const { return m_results; }
	};
} // namespace partest

#endif // PARTEST_H