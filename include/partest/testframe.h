#ifndef PARTEST_TESTFRAME_H
#define PARTEST_TESTFRAME_H

#include <vector>
#include <memory>
#include <functional>

#include <partest/common.h>
#include <partest/types.h>
#include <partest/log.h>
#include <partest/eventemitterinterface.h>
#include <partest/assert.h>

namespace partest
{
	constexpr unsigned NO_TEST_ID = 0;

	class TestFrame;

	class TestFrameView
	{
		const TestFrame *m_testFrame;

	public:
		TestFrameView() : m_testFrame(&TestFrame::getNullTestFrameInstance()) {}

		TestFrameView(const TestFrame &testFrame) : m_testFrame(&testFrame) {}

		static TestFrameView getNullTestFrameView() { return TestFrameView(TestFrame::getNullTestFrameInstance()); }

		unsigned id() const noexcept { return m_testFrame->id(); }
		unsigned parentId() const noexcept { return m_testFrame->parentId(); }
		const TestInfo &info() const noexcept { return m_testFrame->metadata; }
		PARTEST_STRING_PARAM name() const noexcept { return m_testFrame->metadata.name; }
		PARTEST_STRING_PARAM description() const noexcept { return m_testFrame->metadata.description; }
		const TestFlags &flags() const noexcept { return m_testFrame->flags; }
		const TestState &state() const noexcept { return m_testFrame->state; }
		
		TestStatus status() const noexcept { return m_testFrame->state.getStatus(); }
		TestResult result() const noexcept { return m_testFrame->state.getResult(); }
	};

	class TestFrame
	{
		unsigned int m_id;

		/**
		* Get a globally incrementing counter. Used internally to assign IDs to newly created test frames.
		* 
		* @return the next value for frameCount
		*/
		static unsigned int nextID() noexcept {
			static std::atomic<unsigned int> frameCount(NO_TEST_ID + 1);
			return frameCount.fetch_add(1, std::memory_order_relaxed);
		}

		TestFrame()
			: m_eventEmitter(nullptr), flags(), metadata(), state(), m_id(NO_TEST_ID), m_testFrameView(*this)
		{ 
			metadata.name = "Undefined Test Frame";
			metadata.description = "Empty frame representing test suite root";
		}

	protected:
		EventEmitterInterface *m_eventEmitter;
		TestFrameView m_testFrameView;

		std::vector<TestFrame *> m_subtests; // Vector of sub-tests
		std::vector<LogEntry> m_logs; // Logs associated with this test frame
		std::vector<AssertionResult> m_assertions; // Results of assertions triggered by this test frame

		TestFrame *m_parent = nullptr; // Pointer to the parent test frame
		
		std::function<void()> m_testSetup = nullptr; // Test function associated with this frame
		std::function<void()> m_testFunction = nullptr; // Test function associated with this frame
		std::function<void()> m_testTeardown = nullptr; // Test function associated with this frame

	public:
		using TestFrameIter = std::vector<TestFrame *>::iterator;
		using TestFrameConstIter = std::vector<TestFrame *>::const_iterator;

		TestFrame(EventEmitterInterface *eventEmitter) : m_eventEmitter(eventEmitter), flags(), metadata(), state(), m_id(nextID()), m_testFrameView(*this) { }
		TestFrame(EventEmitterInterface *eventEmitter, const TestFlags &flags, const TestInfo &metadata, const TestState &result,
				const std::function<void()> &testFunction = nullptr,
				const std::function<void()> &testSetup = nullptr,
				const std::function<void()> &testTeardown = nullptr)
			: m_eventEmitter(eventEmitter), flags(flags), metadata(metadata), state(result),
				m_testFunction(testFunction), m_testSetup(testSetup), m_testTeardown(testTeardown),
				m_id(nextID()), m_testFrameView(*this) { }
	
		// Nothing should be moving or copying TestFrame instances. They exist as part of a tree structure managed by TestBase.
		TestFrame(const TestFrame &) = delete; // Disable copy constructor
		TestFrame &operator=(const TestFrame &) = delete; // Disable copy assignment
		TestFrame(TestFrame &&) = delete; // Disable move constructor
		TestFrame &operator=(TestFrame &&) = delete; // Disable move assignment

		~TestFrame()
		{
			for(TestFrame *subtest : m_subtests)
				delete subtest;
		}

		static const TestFrame &getNullTestFrameInstance()
		{
			static TestFrame nullInstance;
			return nullInstance;
		}

		unsigned int id() const noexcept { return m_id; }
		unsigned int parentId() const noexcept { return (m_parent != nullptr ? m_parent->m_id : NO_TEST_ID); }

		const TestFrameView &testFrameView() const noexcept { return m_testFrameView; }

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
			LogEntry logEntry(LogLevel::Info, PARTEST_LOG_TYPE_SUBTEST, message);
			log(logEntry);
		}

		void log(LogLevel level, PARTEST_STRING_PARAM type, PARTEST_STRING_PARAM message)
		{
			m_logs.push_back(LogEntry(level, type, message));
		}

		void log(const LogEntry &level)
		{
			m_logs.push_back(level);
		}

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

		void clearSubtests() noexcept 
		{ 
			for(TestFrame *subtest : m_subtests)
				delete subtest;
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
		PARTEST_CONSTEXPR_14 TestStatus getStatus() const noexcept { return state.getStatus(); }
		PARTEST_CONSTEXPR_14 TestResult getResult() const noexcept { return state.getResult(); }

		PARTEST_CONSTEXPR_14 bool hasFinishedRunning() const noexcept { return state.hasFinishedRunning(); }
		PARTEST_CONSTEXPR_14 bool hasFailures() const noexcept { return state.hasFailures(); }

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
		TestFrameIter subtestsBegin() noexcept { return m_subtests.begin(); }
		TestFrameIter subtestsEnd() noexcept { return m_subtests.end(); }
		size_t subtestCount() const noexcept { return m_subtests.size(); }
		TestFrameConstIter subtestsBegin() const noexcept{ return m_subtests.cbegin(); }
		TestFrameConstIter subtestsEnd() const noexcept { return m_subtests.cend(); }

		bool initializeTest()
		{
			// If effective flags indicate the test should be skipped, do nothing and return immediately
			if(getEffectiveFlags().skip == FlagState::Enabled)
			{
				updateStatus(TestStatus::Skipped);
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
				updateStatus(TestStatus::Running);
				
				m_eventEmitter->emitBeginTest(TestFrameView(*this));
				m_testFunction();
				m_eventEmitter->emitEndTest(TestFrameView(*this));

				if(getResult() == TestResult::NoResult)
				{
					log(LogLevel::Warning, PARTEST_LOG_TYPE_TEST, "Warning: '" + metadata.name + "' completed without any assertions. Defaulting to PASSED.");
					updateResult(TestResult::Passed);
				}

				updateStatus(TestStatus::Completed);
			}
			else
			{
				throw std::runtime_error("Attempted to run a test function that is not set.");
			}
		}

		TestFrame *finalizeTest()
		{
			// If effective flags indicate the test should be skipped, do nothing and return immediately
			if(getEffectiveFlags().skip == FlagState::Disabled)
			{
				if(getStatus() != TestStatus::Aborted)
					updateStatus(TestStatus::Completed);

				if(m_parent != nullptr)
				{
					std::string logString = "Finished: " + metadata.name + "; Results: " + to_string(getResult());
					log(LogLevel::Info, PARTEST_LOG_TYPE_TEST, logString);

					//m_parent->log("Finalizing subtest " + metadata.name, this);
					m_parent->updateResult(getResult());
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
				return hasFailures() ? 1 : 0;
			
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
		* Get the effective flags for this test frame, resolving any Inherit values from parent frames.
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
