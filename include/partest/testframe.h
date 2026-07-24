#ifndef PARTEST_TESTFRAME_H
#define PARTEST_TESTFRAME_H

#include <chrono>
#include <vector>
#include <deque>
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

	class TestFrameReaderInterface
	{
	public:
		TestFrameReaderInterface() = default;
		virtual ~TestFrameReaderInterface() = default;

		virtual void readTree(const TestFrame &root) = 0;
	};

	class TestFrameView
	{
		const TestFrame *m_testFrame;

	public:
		TestFrameView(const TestFrame &testFrame);

		static const TestFrameView &getNullTestFrameView();

		unsigned id() const noexcept;
		unsigned parentId() const noexcept;
		
		unsigned assertionCount() const noexcept;
		unsigned subtestCount() const noexcept;
		
		unsigned assertionFailureCount() const noexcept;
		unsigned subtestFailureCount(unsigned depth = 1) const noexcept;

		const TestInfo &info() const noexcept;
		PARTEST_STRING_PARAM name() const noexcept;
		PARTEST_STRING_PARAM description() const noexcept;
		const TestFlags &flags() const noexcept;
		const TestState &state() const noexcept;
		
		TestStatus status() const noexcept;
		TestResult result() const noexcept;
		std::chrono::steady_clock::time_point startTime() const noexcept;
		std::chrono::steady_clock::time_point endTime() const noexcept;
	};

	class TestFrame
	{
		unsigned int m_id;
		std::chrono::steady_clock::time_point m_startTime;
		std::chrono::steady_clock::time_point m_endTime;
		/**
		* Get a globally incrementing counter. Used internally to assign IDs to newly created test frames.
		* 
		* @return the next value for frameCount
		*/
		static unsigned int nextId() noexcept {
			static std::atomic<unsigned int> frameCount(NO_TEST_ID + 1);
			return frameCount.fetch_add(1, std::memory_order_relaxed);
		}

		TestFrame()
			: m_eventEmitter(nullptr), flags(), metadata(), state(), m_id(NO_TEST_ID), m_testFrameView(*this)
		{ 
			metadata.name = "Undefined Test Frame";
			metadata.description = "Empty frame representing test suite root";
		}

		EventEmitterInterface *m_eventEmitter;
		TestFrameView m_testFrameView;

		std::vector<TestFrame *> m_subtests; // Vector of sub-tests
		std::deque<LogEntry> m_logs; // Logs associated with this test frame
		std::deque<AssertionResult> m_assertions; // Results of assertions triggered by this test frame

		TestFrame *m_parent = nullptr; // Pointer to the parent test frame
		
		std::function<void()> m_testSetup = nullptr; // Test function associated with this frame
		std::function<void()> m_testFunction = nullptr; // Test function associated with this frame
		std::function<void()> m_testTeardown = nullptr; // Test function associated with this frame

	public:
		using TestFrameIter = std::vector<TestFrame *>::iterator;
		using TestFrameConstIter = std::vector<TestFrame *>::const_iterator;
		using LogEntryConstIter = std::deque<LogEntry>::const_iterator;
		using AssertionConstIter = std::deque<AssertionResult>::const_iterator;

		TestFrame(EventEmitterInterface *eventEmitter) : m_eventEmitter(eventEmitter), flags(), metadata(), state(), m_id(nextId()), m_testFrameView(*this) { }
		TestFrame(EventEmitterInterface *eventEmitter, const TestFlags &flags, const TestInfo &metadata, const TestState &result,
				const std::function<void()> &testFunction = nullptr,
				const std::function<void()> &testSetup = nullptr,
				const std::function<void()> &testTeardown = nullptr)
			: m_eventEmitter(eventEmitter), flags(flags), metadata(metadata), state(result),
				m_testFunction(testFunction), m_testSetup(testSetup), m_testTeardown(testTeardown),
				m_id(nextId()), m_testFrameView(*this) { }
	
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

		std::chrono::steady_clock::time_point startTime() const noexcept { return m_startTime; }
		std::chrono::steady_clock::time_point endTime() const noexcept { return m_endTime; }

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

		void recordLog(LogLevel level, PARTEST_STRING_PARAM type, PARTEST_STRING_PARAM message)
		{
			m_logs.push_back(LogEntry(level, type, message));
			m_eventEmitter->emitLog(m_testFrameView, m_logs.back());
		}

		const std::deque<LogEntry> &getLogs() const noexcept { return m_logs; }

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

		void processAssertion(const AssertionResult &result)
		{
			updateResult(result.passed ? TestResult::Passed : TestResult::Failed);
			m_assertions.push_back(result);
			m_eventEmitter->emitAssertion(m_testFrameView, AssertionResultView(m_assertions.back()));
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

		size_t logEntryCount() const noexcept { return m_logs.size(); }
		LogEntryConstIter logEntryBegin() const noexcept { return m_logs.cbegin(); }
		LogEntryConstIter logEntryEnd() const noexcept { return m_logs.cend(); }

		size_t assertionCount() const noexcept { return m_assertions.size(); }
		AssertionConstIter assertionsBegin() const noexcept { return m_assertions.cbegin(); }
		AssertionConstIter assertionsEnd() const noexcept { return m_assertions.cend(); }

		bool initializeTest()
		{
			m_eventEmitter->emitBeginTest(TestFrameView(*this));
			// If effective flags indicate the test should be skipped, do nothing and return immediately
			if(getEffectiveFlags().skip == FlagState::Enabled)
			{
				updateStatus(TestStatus::Skipped);
				return false;
			}
			else
			{
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
			if(m_testFunction == nullptr)
				throw std::runtime_error("Attempted to run a test function that is not set.");

			struct ClockGuard
			{
				TestFrame *frame;
				~ClockGuard()
				{
					frame->m_endTime = std::chrono::steady_clock::now();
				}
			} guard{this};

			updateStatus(TestStatus::Running);
			m_startTime = std::chrono::steady_clock::now();
			m_testFunction();
			// ClockGuard sets endTime automatically on return
		}

		TestFrame *finalizeTest()
		{
			// If effective flags indicate the test should be skipped, do nothing and return immediately
			if(getEffectiveFlags().skip == FlagState::Disabled)
			{
				if(getResult() == TestResult::NoResult)
				{
					recordLog(LogLevel::Warning, LOG_TYPE_TEST, "Warning: '" + metadata.name + "' completed without any assertions. Defaulting to PASSED.");
					updateResult(TestResult::Passed);
				}

				if(getStatus() != TestStatus::Aborted)
					updateStatus(TestStatus::Completed);

				if(m_parent != nullptr)
				{
					m_parent->updateResult(getResult());
				}

				if(m_testTeardown != nullptr)
				{
					m_testTeardown();
				}
			}
			m_eventEmitter->emitEndTest(TestFrameView(*this));

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

		unsigned getAssertionCount() const
		{
			unsigned total = m_assertions.size();
			for(TestFrame *subtest : m_subtests)
			{
				total += subtest->getAssertionCount();
			}

			return total;
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
	
	/**
	* TestFrameView function definitions
	*/
	inline TestFrameView::TestFrameView(const TestFrame &testFrame) : m_testFrame(&testFrame) {}

	inline const TestFrameView &TestFrameView::getNullTestFrameView() { return TestFrame::getNullTestFrameInstance().testFrameView(); }

	inline unsigned TestFrameView::id() const noexcept { return m_testFrame->id(); }
	inline unsigned TestFrameView::parentId() const noexcept { return m_testFrame->parentId(); }

	inline unsigned TestFrameView::assertionCount() const noexcept { return m_testFrame->assertionCount(); }
	inline unsigned TestFrameView::subtestCount() const noexcept { return m_testFrame->subtestCount(); }

	inline unsigned TestFrameView::assertionFailureCount() const noexcept { return m_testFrame->getAssertionFailureCount(); }
	inline unsigned TestFrameView::subtestFailureCount(unsigned depth = 1) const noexcept { return m_testFrame->getTestFailureCount(depth); }

	inline const TestInfo &TestFrameView::info() const noexcept { return m_testFrame->metadata; }
	inline PARTEST_STRING_PARAM TestFrameView::name() const noexcept { return m_testFrame->metadata.name; }
	inline PARTEST_STRING_PARAM TestFrameView::description() const noexcept { return m_testFrame->metadata.description; }
	inline const TestFlags &TestFrameView::flags() const noexcept { return m_testFrame->flags; }
	inline const TestState &TestFrameView::state() const noexcept { return m_testFrame->state; }

	inline TestStatus TestFrameView::status() const noexcept { return m_testFrame->state.getStatus(); }
	inline TestResult TestFrameView::result() const noexcept { return m_testFrame->state.getResult(); }

	inline std::chrono::steady_clock::time_point TestFrameView::startTime() const noexcept { return m_testFrame->startTime(); }
	inline std::chrono::steady_clock::time_point TestFrameView::endTime() const noexcept { return m_testFrame->startTime(); }
}

#endif // PARTESTTESTFRAME_H
