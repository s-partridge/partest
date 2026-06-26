#ifndef PARTESTTESTFRAME_H
#define PARTESTTESTFRAME_H

#include <vector>
#include <memory>
#include <functional>

#include "partestcommon.h"
#include "partesttypes.h"
#include "partestlog.h"
#include "partestassert.h"

namespace partest
{
	class TestFrame
	{		
	protected:
		std::vector<TestFrame *> m_subtests; // Vector of sub-tests
		std::vector<LogEntry> m_logs; // Logs associated with this test frame
		std::vector<AssertionResult> m_assertions; // Results of assertions triggered by this test frame

		TestFrame *m_parent = nullptr; // Pointer to the parent test frame
		
		std::function<void()> m_testSetup = nullptr; // Test function associated with this frame
		std::function<void()> m_testFunction = nullptr; // Test function associated with this frame
		std::function<void()> m_testTeardown = nullptr; // Test function associated with this frame

		/**
		* Get a globally incrementing counter. Used internally to assign IDs to newly created test frames.
		* 
		* @return the next value for frameCount
		*/
		static unsigned int getNextFrameID() noexcept {
			static std::atomic<unsigned int> frameCount(0);
			return frameCount++;
		}

		unsigned int m_frameID;

	public:
		TestFrame() noexcept : flags(), metadata(), state(), m_frameID(getNextFrameID()) { }
		TestFrame(const TestFlags &flags, const TestInfo &metadata, const TestState &result,
				const std::function<void()> &testFunction = nullptr,
				const std::function<void()> &testSetup = nullptr,
				const std::function<void()> &testTeardown = nullptr)
			: flags(flags), metadata(metadata), state(result),
				m_testFunction(testFunction), m_testSetup(testSetup), m_testTeardown(testTeardown),
				m_frameID(getNextFrameID()) {}
	
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

		unsigned int frameID() const noexcept { return m_frameID; }

		TestInfo metadata; // Test metadata, including name and description
		TestFlags flags; // Effective flags for this test frame
		TestState state; // Result of the test

		void setSetupFunction(const std::function<void()> &setupFunction) { m_testSetup = setupFunction; }
		void setTestFunction(const std::function<void()> &testFunction) { m_testFunction = testFunction; }
		void setTeardownFunction(const std::function<void()> &teardownFunction) { m_testTeardown = teardownFunction; }
		
		bool hasSetupFunction() const noexcept { return m_testSetup != nullptr; }
		bool hasTestFunction() const noexcept { return m_testFunction != nullptr; }
		bool hasTeardownFunction() const noexcept { return m_testTeardown != nullptr; }

		void logSubtestTransition(PARTEST_STRING_PARAM message, const TestFrame *frame)
		{
			LogEntry logEntry(LogLevel::INFO, PARTEST_LOG_SUBTEST, message);
			logEntry.testFrameID = frame != nullptr ? frame->frameID() : 0;
			log(logEntry);
		}

		void log(LogLevel level, PARTEST_STRING_PARAM type, PARTEST_STRING_PARAM message) { m_logs.push_back(LogEntry(level, type, message)); }
		void log(const LogEntry &level) { m_logs.push_back(level); }
		const std::vector<LogEntry> &getLogs() const noexcept { return m_logs; }

		void logAssertion(const AssertionResult &result)
		{
			m_assertions.push_back(result);
		}

		void logAssertion(bool passed,
				PARTEST_STRING_PARAM assertType, PARTEST_STRING_PARAM condition,
				PARTEST_STRING_PARAM message, PARTEST_STRING_PARAM file,
				int line)
		{
			m_assertions.push_back(AssertionResult(passed, assertType, condition, message, file, line));
		}

		void clearLogs() noexcept { m_logs.clear(); }
		void clearSubtests() 
		{ 
			for(TestFrame *subtest : m_subtests)
			{
				if(subtest != nullptr)
				{
					delete subtest;
				}
			}
			m_subtests.clear(); 
		}
		void clearAll() 
		{ 
			clearLogs(); 
			clearSubtests(); 
			state = TestState::defaultState();
		}

		PARTEST_CONSTEXPR_14 void updateStatus(TestStatus status) noexcept { state.updateStatus(status); }
		PARTEST_CONSTEXPR_14 void updateResult(TestResult result) noexcept { state.updateResult(result); }
		PARTEST_CONSTEXPR_11 TestStatus getStatus() const noexcept { return state.getStatus(); }
		PARTEST_CONSTEXPR_11 TestResult getResult() const noexcept { return state.getResult(); }

		PARTEST_CONSTEXPR_11 bool hasFinishedRunning() const noexcept { return state.hasFinishedRunning(); }
		PARTEST_CONSTEXPR_11 bool hasFailures() const noexcept { return state.hasFailures(); }

		/**
		* Check whether this test frame descends from `other`
		* 
		* @param other potential ancestor of this test frame
		* @return true if `other` is an ancestor of this, false otherwise
		*/
		bool isDescendentOf(const TestFrame *other) const noexcept
		{
			const TestFrame *current = m_parent;

			while(current != nullptr)
			{
				if(current == other)
					return true;
				current = current->m_parent;
			}

			return false;
		}

		/**
		* Check whether this test frame is an ancestor of `other`
		* 
		* @param other potential ancestor of this test frame
		* @return true if `other` is an ancestor of this, false otherwise
		*/
		bool isAncestorOf(const TestFrame *other) const noexcept
		{
			return other != nullptr && other->isDescendentOf(this);
		}

		/**
		* Get the parent test frame.
		* 
		* @return non-owning pointer to the parent TestFrame, or nullptr if this is the root frame.
		*/
		TestFrame *getParent() const noexcept { return m_parent; }
		bool hasParent() const noexcept { return m_parent != nullptr; }
		bool hasSubtests() const noexcept { return !m_subtests.empty(); }

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
		* Iterator access for subtests
		*/
		std::vector<TestFrame *>::iterator subtestsBegin() noexcept { return m_subtests.begin(); }
		std::vector<TestFrame *>::iterator subtestsEnd() noexcept { return m_subtests.end(); }
		size_t subtestCount() const noexcept { return m_subtests.size(); }
		std::vector<TestFrame *>::const_iterator subtestsBegin() const noexcept{ return m_subtests.cbegin(); }
		std::vector<TestFrame *>::const_iterator subtestsEnd() const noexcept { return m_subtests.cend(); }

		bool initializeTest()
		{
			// If effective flags indicate the test should be skipped, do nothing and return immediately
			if(getEffectiveFlags().skip == ENABLED)
			{
				
				updateStatus(SKIPPED);
				return false;
			}
			else
			{
				if(m_parent != nullptr)
				{
					m_parent->logSubtestTransition("Initializing subtest " + metadata.name, this);
				}
				if(m_testSetup != nullptr)
				{
					m_testSetup();
				}
				return true;
			}
		}

		/**
		* Run the test function associated with this test frame, if one is set.
		*
		* @throws std::runtime_error if no test function is set.
		*/
		void runTestFunction()
		{
			if(m_testFunction != nullptr)
			{
				updateStatus(RUNNING);
				m_testFunction();

				if(getResult() == NO_RESULT)
				{
					log(WARNING, PARTEST_LOG_TEST, "Warning: '" + metadata.name + "' completed without any assertions. Defaulting to PASSED.");
					updateResult(PASSED);
				}

				updateStatus(COMPLETED);
			}
			else
			{
				throw std::runtime_error("Attempted to run a test function that is not set.");
			}
		}

		TestFrame *finalizeTest()
		{
			// If effective flags indicate the test should be skipped, do nothing and return immediately
			if(getEffectiveFlags().skip == DISABLED)
			{
				if(state.getStatus() != ABORTED)
					updateStatus(COMPLETED);

				if(m_parent != nullptr)
				{
					std::string logString = "Finished: " + metadata.name + "; Results: " + to_string(state.getResult());
					log(INFO, PARTEST_LOG_TEST, logString);

					//m_parent->log("Finalizing subtest " + metadata.name, this);
					m_parent->state.updateResult(state.getResult());
				}

				if(m_testTeardown != nullptr)
				{
					m_testTeardown();
				}
			}

			return m_parent;
		}

		/**
		* Count the total number of subtests that failed at a specific tree depth.
		* 
		* @param depth Subtest depth to drill down to, from the current test frame
		* @returns Number of tests below this one that failed at specified depth
		*/
		unsigned getTestFailureCount(unsigned depth = 1) const
		{
			// Only evaluate this frame's result if we're at evaluation depth, or if no subtests exist.
			if(depth == 0 || m_subtests.empty())
				return getResult() == TestResult::FAILED ? 1 : 0;
			
			unsigned failureCount = 0;

			for(TestFrame *subtest : m_subtests)
			{
				failureCount += subtest->getTestFailureCount(depth - 1);
			}

			return failureCount;
		}

		/**
		* Count the total number of assertions that failed in this frame's subtest tree
		* 
		* @returns Sum total of all assertions that failed for this frame and all of its descendants
		*/
		unsigned getAssertionFailureCount() const
		{
			unsigned failureCount = 0;

			for(const AssertionResult &result : m_assertions)
			{
				if(!result.passed)
					++failureCount;
			}

			for(TestFrame *subtest : m_subtests)
			{
				failureCount += subtest->getAssertionFailureCount();
			}

			return failureCount;
		}

		/**
		* Get the effective flags for this test frame, resolving any INHERIT values from parent frames.
		* 
		* @return The effective TestFlags for this test frame.
		*/
		TestFlags getEffectiveFlags() const noexcept
		{
			// If the current flags are not fully resolved, inherit from parent
			if(m_parent != nullptr && !flags.isResolved())
			{
				// Inherit from parent.
				return flags.mergeWithParentFlags(m_parent->getEffectiveFlags());
			}
			else
			{
				return flags;
			}
		}
	};
}

#endif // PARTESTTESTFRAME_H
