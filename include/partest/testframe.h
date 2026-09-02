#ifndef PARTEST_TESTFRAME_H
#define PARTEST_TESTFRAME_H

#include <chrono>
#include <vector>
#include <deque>
#include <mutex>
#include <memory>
#include <functional>

#include <partest/common.h>
#include <partest/types.h>
#include <partest/log.h>
#include <partest/eventemitterinterface.h>
#include <partest/assert.h>

namespace partest
{
	PARTEST_INLINE_VAR_17 constexpr unsigned NO_TEST_ID = 0;

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
		
		size_t assertionCount() const noexcept;
		size_t subtestCount() const noexcept;
		size_t testSkippedCount() const;
		
		size_t assertionFailureCount() const;
		size_t subtestFailureCount(unsigned depth = 1) const;

		const TestInfo &info() const noexcept;
		PARTEST_STRING_PARAM name() const noexcept;
		PARTEST_STRING_PARAM description() const noexcept;
		TestFlags flags() const noexcept;
		TestState state() const;
		
		std::string fullTestName() const;
		std::string testNameToDepth(size_t depth) const;

		TestStatus getStatus() const;
		TestResult getEffectiveResult() const;
		bool getExpectFailure() const;

		std::chrono::steady_clock::duration duration() const noexcept { return endTime() - startTime(); }

		std::chrono::steady_clock::time_point startTime() const noexcept;
		std::chrono::steady_clock::time_point endTime() const noexcept;

		Timestamp timestamp() const noexcept;
	};

	class TestContext;

	class TestFrame
	{
		unsigned int m_id;
		std::chrono::steady_clock::time_point m_startTime;
		std::chrono::steady_clock::time_point m_endTime;
		Timestamp m_timeStarted;

		EventEmitterInterface *m_eventEmitter;
		TestFrameView m_testFrameView;
		TestState state;

		std::vector<TestFrame *> m_subtests; // Vector of sub-tests
		std::deque<LogEntry> m_logs; // Logs associated with this test frame
		std::deque<AssertionResult> m_assertions; // Results of assertions triggered by this test frame

		TestFrame *m_parent = nullptr; // Pointer to the parent test frame
		
		std::function<void(TestContext&)> m_testSetup = nullptr; // Test function associated with this frame
		std::function<void(TestContext&)> m_testFunction = nullptr; // Test function associated with this frame
		std::function<void(TestContext&)> m_testTeardown = nullptr; // Test function associated with this frame

		// Concurrency primitives
		mutable std::mutex m_subtestsMutex; // Mutex for synchronizing access to subtests
		mutable std::mutex m_logsMutex; // Mutex for synchronizing access to logs
		mutable std::mutex m_assertionsMutex; // Mutex for synchronizing access to assertions
		mutable std::mutex m_statusMutex; // Mutex for synchronizing access to test status and state
		mutable std::mutex m_resultMutex; // Mutex for synchronizing access to test result
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
			m_startTime = std::chrono::steady_clock::time_point();
			m_endTime = std::chrono::steady_clock::time_point();
			m_timeStarted = std::chrono::system_clock::time_point();

			metadata.name = "Undefined Test Frame";
			metadata.description = "Empty frame representing test suite root";
		}

	public:
		using TestFrameIter = std::vector<TestFrame *>::iterator;
		using TestFrameConstIter = std::vector<TestFrame *>::const_iterator;
		using LogEntryConstIter = std::deque<LogEntry>::const_iterator;
		using AssertionConstIter = std::deque<AssertionResult>::const_iterator;

		TestFrame(EventEmitterInterface *eventEmitter) : m_eventEmitter(eventEmitter), flags(), metadata(), state(), m_id(nextId()), m_testFrameView(*this) { }
		TestFrame(EventEmitterInterface *eventEmitter, const TestFlags &flags, const TestInfo &metadata,
				const std::function<void(TestContext&)> &testFunction = nullptr,
				const std::function<void(TestContext&)> &testSetup = nullptr,
				const std::function<void(TestContext&)> &testTeardown = nullptr)
			: m_eventEmitter(eventEmitter), flags(flags), metadata(metadata), state(flags.expectFailure == FlagState::Enabled),
				m_testFunction(testFunction), m_testSetup(testSetup), m_testTeardown(testTeardown),
				m_id(nextId()), m_testFrameView(*this) { }
	
		// Nothing should be moving or copying TestFrame instances. They exist as part of a tree structure managed by TestBase.
		TestFrame(const TestFrame &) = delete; // Disable copy constructor
		TestFrame &operator=(const TestFrame &) = delete; // Disable copy assignment
		TestFrame(TestFrame &&) = delete; // Disable move constructor
		TestFrame &operator=(TestFrame &&) = delete; // Disable move assignment

		~TestFrame()
		{
			clearSubtests();
		}

		static const TestFrame &getNullTestFrameInstance()
		{
			static TestFrame nullInstance;
			return nullInstance;
		}

		unsigned int id() const noexcept { return m_id; }
		unsigned int parentId() const noexcept { return (m_parent != nullptr ? m_parent->m_id : NO_TEST_ID); }

		std::string fullTestName() const
		{
			if(m_parent != nullptr)
				return m_parent->fullTestName() + '.' + metadata.name;
			return metadata.name;
		}

		std::string testNameToDepth(size_t depth) const
		{
			if(m_parent != nullptr && depth > 0)
				return m_parent->testNameToDepth(depth - 1) + '.' + metadata.name;
			return metadata.name;
		}

		std::chrono::steady_clock::time_point startTime() const noexcept { return m_startTime; }
		std::chrono::steady_clock::time_point endTime() const noexcept { return m_endTime; }
		Timestamp timestamp() const noexcept { return m_timeStarted; }

		PARTEST_STRING_PARAM testFile() const noexcept { return metadata.file; }
		void setTestFile(PARTEST_STRING_PARAM fileName) { metadata.file = fileName; }

		unsigned testLine() const noexcept { return metadata.line; }
		void setTestLine(unsigned line) { metadata.line = line; }
		const TestFrameView &testFrameView() const noexcept { return m_testFrameView; }

		TestInfo metadata; // Test metadata, including name and description
		TestFlags flags; // Effective flags for this test frame

		void setSetupFunction(const std::function<void(TestContext&)> &setupFunction) { m_testSetup = setupFunction; }
		void setTestFunction(const std::function<void(TestContext&)> &testFunction) { m_testFunction = testFunction; }
		void setTeardownFunction(const std::function<void(TestContext&)> &teardownFunction) { m_testTeardown = teardownFunction; }
		
		bool hasSetupFunction() const noexcept { return m_testSetup != nullptr; }
		bool hasTestFunction() const noexcept { return m_testFunction != nullptr; }
		bool hasTeardownFunction() const noexcept { return m_testTeardown != nullptr; }

		void processAssertion(const AssertionResult &result)
		{
			std::unique_lock<std::mutex> assertionLock(m_assertionsMutex, std::defer_lock);
			std::unique_lock<std::mutex> resultLock(m_resultMutex, std::defer_lock);
			
			std::lock(assertionLock, resultLock);
			
			state.updateResultFromAssertion(result.passed());
			m_assertions.push_back(result);
			
			resultLock.unlock();
			assertionLock.unlock();

			m_eventEmitter->emitAssertion(m_testFrameView, result, std::chrono::system_clock::now());
		}

		void abortTest(PARTEST_STRING_PARAM message)
		{
			// Mark the test as aborted and log a generic message.
			updateStatus(TestStatus::Aborting);
			// Ensure that the test result was set. A generic exception indicates test failure.
			updateResult(TestResult::Failed);
			recordLog(LogLevel::Error, LOG_TYPE_EXCEPTION, message);
		}

		void recordLog(LogLevel level, PARTEST_STRING_PARAM type, PARTEST_STRING_PARAM message)
		{
			std::unique_lock<std::mutex> logLock(m_logsMutex);
			m_logs.push_back(LogEntry(level, type, message));
			LogEntry &logEntry = m_logs.back();
			logLock.unlock();

			m_eventEmitter->emitLog(m_testFrameView, logEntry, std::chrono::system_clock::now());
		}

		void clearLogs()
		{
			std::lock_guard<std::mutex> logLock(m_logsMutex);
			m_logs.clear();
		}

		void clearSubtests() 
		{ 
			std::lock_guard<std::mutex> subtestsLock(m_subtestsMutex);
			for(TestFrame *subtest : m_subtests)
				delete subtest;
			m_subtests.clear();
		}

		void resetState() 
		{ 
			std::unique_lock<std::mutex> statusLock(m_statusMutex, std::defer_lock);
			std::unique_lock<std::mutex> resultLock(m_resultMutex, std::defer_lock);
			std::lock(statusLock, resultLock);

			state = TestState::defaultState(); 
		}

		void clearAll() 
		{ 
			clearLogs(); 
			clearSubtests(); 
			resetState();
		}

		// Add locks around accessors and mutators to status and result.
		TestStatus getStatus() const
		{
			std::lock_guard<std::mutex> statusLock(m_statusMutex);
			return state.getStatus();
		}
		TestResult getEffectiveResult() const
		{
			std::lock_guard<std::mutex> resultLock(m_resultMutex);
			return state.getEffectiveResult();
		}

		bool isRunning() const
		{
			std::lock_guard<std::mutex> statusLock(m_statusMutex);
			return state.isRunning();
		}

		bool hasFinishedRunning() const
		{
			std::lock_guard<std::mutex> statusLock(m_statusMutex);
			return state.hasFinishedRunning();
		}

		bool hasFailures() const
		{
			std::lock_guard<std::mutex> resultLock(m_resultMutex);
			return state.hasFailures();
		}

		bool wasSkipped() const
		{
			std::lock_guard<std::mutex> statusLock(m_statusMutex);
			return state.wasSkipped();
		}

		bool getExpectFailure() const
		{
			std::lock_guard<std::mutex> statusLock(m_statusMutex);
			return state.getExpectFailure();
		}

		void updateResult(const TestResult &result)
		{
			std::lock_guard<std::mutex> resultLock(m_resultMutex);
			state.updateResult(result);
		}

		void updateResultFromSubtest(const TestState &subtestState)
		{
			std::lock_guard<std::mutex> resultLock(m_resultMutex);
			state.updateResultFromSubtest(subtestState);
		}

		void updateStatus(TestStatus status)
		{
			std::lock_guard<std::mutex> statusLock(m_statusMutex);
			state.updateStatus(status);
		}

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

		bool hasSubtests() const
		{
			std::lock_guard<std::mutex> subtestsLock(m_subtestsMutex);
			return !m_subtests.empty();
		}

		const TestFrame *getSubtest(PARTEST_STRING_PARAM subtestName) const
		{
			std::lock_guard<std::mutex> subtestsLock(m_subtestsMutex);
			for(TestFrame *subtest: m_subtests)
			{
				if(subtest && subtest->metadata.name == subtestName)
					return subtest;
			}

			return nullptr;
		}

		/**
		* Add a subtest to the current test frame.
		* 
		* @param subtest Pointer to the subtest to be added
		*/
		TestFrame *addSubtest(std::unique_ptr<TestFrame> subtest)
		{
			TestFrame *subtestPtr = subtest.get();
			{
				std::lock_guard<std::mutex> subtestsLock(m_subtestsMutex);
				m_subtests.push_back(subtestPtr);
			}
			subtestPtr->m_parent = this;
			subtest.release();

			return subtestPtr;
		}

		/**
		* Iterator access for subtests, logs, and assertions. These iterators are not thread-safe.
		* As separate, atomic operations, iterators cannot be guaranteed to be valid unless the test frame is not being modified,
		* so instead, access to TestFrame itself is strictly controlled through TestContext and TestBase.
		*/
		size_t subtestCount() const noexcept { return m_subtests.size(); }
		TestFrameIter subtestsBegin() noexcept { return m_subtests.begin(); }
		TestFrameIter subtestsEnd() noexcept { return m_subtests.end(); }
		TestFrameConstIter subtestsBegin() const noexcept{ return m_subtests.cbegin(); }
		TestFrameConstIter subtestsEnd() const noexcept { return m_subtests.cend(); }

		size_t logCount() const noexcept { return m_logs.size(); }
		LogEntryConstIter logsBegin() const noexcept { return m_logs.cbegin(); }
		LogEntryConstIter logsEnd() const noexcept { return m_logs.cend(); }

		size_t assertionCount() const noexcept { return m_assertions.size(); }
		AssertionConstIter assertionsBegin() const noexcept { return m_assertions.cbegin(); }
		AssertionConstIter assertionsEnd() const noexcept { return m_assertions.cend(); }

		bool initializeTest(TestContext& ctx)
		{
			assert(getStatus() == TestStatus::Awaiting && "Test frame is already initialized or has already run.");

			m_timeStarted = std::chrono::system_clock::now();
			m_eventEmitter->emitBeginTest(TestFrameView(*this), m_timeStarted);
			// If effective flags indicate the test should be skipped, do nothing and return immediately
			if(getEffectiveFlags().skip == FlagState::Enabled)
			{
				updateStatus(TestStatus::Skipped);
				m_eventEmitter->emitEndTest(TestFrameView(*this), std::chrono::system_clock::now());
				return false;
			}
			else
			{
				updateStatus(TestStatus::SettingUp);
				if(m_testSetup != nullptr)
				{
					m_testSetup(ctx);
				}

				return true;
			}
		}

		/**
		* Run the test function associated with this test frame, if one is set.
		*
		* @throws std::runtime_error if no test function is set.
		*/
		void runTestFunction(TestContext& ctx)
		{
			// Framework invariant: A test should not be run if it was skipped. This is enforced by the test framework, and should never be violated.
			assert(!wasSkipped() && "Invalid test state. Test should not be run if it was skipped.");

			// This precedes the run guard because the guard itself would temporarily override status.
			// Correct status transitions from SettingUp to Aborting on the failed path, skipping TearingDown entirely.
			if(m_testFunction == nullptr)
				throw std::runtime_error("Attempted to run a test function that is not set.");

			struct RunGuard
			{
				TestFrame *frame;
				~RunGuard()
				{
					frame->m_endTime = std::chrono::steady_clock::now();
					frame->updateStatus(TestStatus::TearingDown);
				}
			} guard{this};

			updateStatus(TestStatus::Running);
			m_startTime = std::chrono::steady_clock::now();
			m_testFunction(ctx);
			// RunGuard updates the status and end time automatically via RAII when this function exits, even if an exception is thrown.
		}

		TestFrame *finalizeTest(TestContext& ctx)
		{
			// If effective flags indicate the test should be skipped, do nothing and return immediately
			if(getEffectiveFlags().skip != FlagState::Enabled)
			{
				// Flag state/run status invariant. Test should not be marked as skipped if the skip flag is not set.
				assert(!wasSkipped() && "Invalid test state. Test should not have been skipped without skip flag set.");
				// Run status invariant. A test should only be finalized if it is in the process of tearing down or has been aborted.
				assert(getStatus() == TestStatus::TearingDown || getStatus() == TestStatus::Aborting && "Test frame is not in a valid state to finalize. Test should be tearing down or aborted.");

				// TODO: Refactor this to iterate over subtests instead of the child calling on the parent.
				// This moves the call to the logical end of the test tree, where the parent can evaluate all subtests and update its own state accordingly.
				// It also provides a place to potentially ensure that concurrent subtests have completed. At this point, if they have not, something is wrong.

				for(TestFrame *subtest : m_subtests)
				{
					if(subtest->hasFinishedRunning() || subtest->wasSkipped())
					{
						updateResultFromSubtest(subtest->state);
					}
					else
					{
						recordLog(LogLevel::Warning, LOG_TYPE_TEST, "Subtest \"" + subtest->fullTestName() + "\" has not completed. This may indicate a problem with the test framework or a test that did not complete properly.");
					}
				}

				if(getEffectiveResult() == TestResult::NoResult)
				{
					recordLog(LogLevel::Warning, LOG_TYPE_TEST, "\"" + fullTestName() + "\" completed without any assertions. Defaulting to PASSED.");
					// Shunt a passing value to the state
					state.updateResultFromAssertion(true);
				}

				if(m_testTeardown != nullptr)
				{
					m_testTeardown(ctx);
				}

				if(getStatus() != TestStatus::Aborting)
				{
					updateStatus(TestStatus::Completed);
				}
				else
				{
					updateStatus(TestStatus::Aborted);
				}
			}
			m_eventEmitter->emitEndTest(TestFrameView(*this), std::chrono::system_clock::now());

			return m_parent;
		}

		/**
		* Count the total number of subtests that failed at a specific tree depth.
		* 
		* @param depth Subtest depth to drill down to, from the current test frame
		* @returns Number of tests below this one that failed at specified depth
		*/
		size_t getTestFailureCount(unsigned depth = 1) const
		{
			std::lock_guard<std::mutex> subtestLock(m_subtestsMutex);
			// Only evaluate this frame's result if we're at evaluation depth, or if no subtests exist.
			if(depth == 0 || m_subtests.empty())
				return hasFailures() ? 1 : 0;

			size_t failureCount = 0;

			for(TestFrame *subtest : m_subtests)
			{
				failureCount += subtest->getTestFailureCount(depth - 1);
			}

			return failureCount;
		}

		/**
		* Count the total number of subtests that were skipped at a specific tree depth.
		* 
		* @param depth Subtest depth to drill down to, from the current test frame
		* @returns Number of tests below this one that failed at specified depth
		*/
		size_t getTestSkippedCount(unsigned depth = 1) const
		{
			std::lock_guard<std::mutex> subtestLock(m_subtestsMutex);
			// Only evaluate this frame's result if we're at evaluation depth, or if no subtests exist.
			if(depth == 0 || m_subtests.empty())
				return wasSkipped() ? 1 : 0;
			
			size_t skippedCount = 0;

			for(TestFrame *subtest : m_subtests)
			{
				skippedCount += subtest->getTestSkippedCount(depth - 1);
			}

			return skippedCount;
		}

		size_t getAssertionCount(bool onlyCountFailures = false) const
		{
			size_t total = 0;
			if(!onlyCountFailures)
			{
				m_assertionsMutex.lock();
				//guaranteed noexcept and no early return
				total = (unsigned)m_assertions.size();
				m_assertionsMutex.unlock();
			}
			else
			{
				m_assertionsMutex.lock();
				//guaranteed noexcept and no early return
				for(const AssertionResult &result : m_assertions)
				{
					if(!result.passed())
						++total;
				}
				m_assertionsMutex.unlock();
			}

			std::lock_guard<std::mutex> subtestLock(m_subtestsMutex);
			for(TestFrame *subtest : m_subtests)
			{
				total += subtest->getAssertionCount(onlyCountFailures);
			}
			return total;
		}

		/**
		* Count the total number of assertions that failed in this frame's subtest tree
		* 
		* @returns Sum total of all assertions that failed for this frame and all of its descendants
		*/
		size_t getAssertionFailureCount() const
		{
			size_t failureCount = 0;

			std::unique_lock<std::mutex> assertionLock(m_assertionsMutex);
			for(const AssertionResult &result : m_assertions)
			{
				if(!result.passed())
					++failureCount;
			}
			assertionLock.unlock();

			std::lock_guard<std::mutex> subtestLock(m_subtestsMutex);
			for(TestFrame *subtest : m_subtests)
			{
				failureCount += subtest->getAssertionFailureCount();
			}

			return failureCount;
		}

		/**
		* Get a snapshot of this frame's current state
		* 
		* @returns A copy of the current state for this test frame
		*/
		TestState getCurrentState() const
		{
			std::unique_lock<std::mutex> statusLock(m_statusMutex, std::defer_lock);
			std::unique_lock<std::mutex> resultLock(m_resultMutex, std::defer_lock);
			std::lock(statusLock, resultLock);
			return state;
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

	inline size_t TestFrameView::assertionCount() const noexcept { return m_testFrame->assertionCount(); }
	inline size_t TestFrameView::subtestCount() const noexcept { return m_testFrame->subtestCount(); }

	inline size_t TestFrameView::assertionFailureCount() const { return m_testFrame->getAssertionFailureCount(); }
	inline size_t TestFrameView::subtestFailureCount(unsigned depth) const { return m_testFrame->getTestFailureCount(depth); }
	inline size_t TestFrameView::testSkippedCount() const { return m_testFrame->getTestSkippedCount(1); }

	inline const TestInfo &TestFrameView::info() const noexcept { return m_testFrame->metadata; }
	inline PARTEST_STRING_PARAM TestFrameView::name() const noexcept { return m_testFrame->metadata.name; }
	inline PARTEST_STRING_PARAM TestFrameView::description() const noexcept { return m_testFrame->metadata.description; }
	inline TestFlags TestFrameView::flags() const noexcept { return m_testFrame->getEffectiveFlags(); }
	inline TestState TestFrameView::state() const { return m_testFrame->getCurrentState(); }

	inline std::string TestFrameView::fullTestName() const { return m_testFrame->fullTestName(); }
	inline std::string TestFrameView::testNameToDepth(size_t depth) const { return m_testFrame->testNameToDepth(depth); }

	inline TestStatus TestFrameView::getStatus() const { return m_testFrame->getStatus(); }
	inline TestResult TestFrameView::getEffectiveResult() const { return m_testFrame->getEffectiveResult(); }
	inline bool TestFrameView::getExpectFailure() const { return m_testFrame->getExpectFailure(); }

	inline std::chrono::steady_clock::time_point TestFrameView::startTime() const noexcept { return m_testFrame->startTime(); }
	inline std::chrono::steady_clock::time_point TestFrameView::endTime() const noexcept { return m_testFrame->endTime(); }

	inline Timestamp TestFrameView::timestamp() const noexcept { return m_testFrame->timestamp(); }
}

#endif // PARTESTTESTFRAME_H
