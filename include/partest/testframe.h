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
		// TODO: Consider vectors of pointers instead. Deque has high default overhead on GCC,
		// and pointers would consolidate ownership here without introducing any concerns about reference invalidation.

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

		void recordLogUnchecked(LogLevel level, PARTEST_STRING_PARAM type, PARTEST_STRING_PARAM message)
		{
			std::unique_lock<std::mutex> logLock(m_logsMutex);
			m_logs.push_back(LogEntry(level, type, message));
			LogEntry &logEntry = m_logs.back();
			logLock.unlock();

			m_eventEmitter->emitLog(m_testFrameView, logEntry, std::chrono::system_clock::now());
		}

		void recordExceptionLogAndThrow(BadAllocSource processingSource, const char *processingSourceStr)
		{
			try
			{
				recordLogUnchecked(LogLevel::Error, LOG_TYPE_EXCEPTION, "Memory allocation failed while processing " + std::string(processingSourceStr) + ".");
			}
			// If recordLogUnchecked throws a bad_alloc, we eat it and report the original bad_alloc here instead, since it was first.
			catch(...)
			{
			}

			TestStatus currentStatus = getStatus();
			abortTestImmediately();
			throw FrameworkAllocationFailure(processingSource, currentStatus, this);
		}

		/**
		* Add a subtest to the current test frame.
		* 
		* @param subtest Pointer to the subtest to be added
		*/
		TestFrame *addSubtest(std::unique_ptr<TestFrame> subtest)
		{
			assert(m_eventEmitter != nullptr && "Event emitter must be set before adding subtests.");

			TestFrame *subtestPtr = subtest.get();
			{
				std::lock_guard<std::mutex> subtestsLock(m_subtestsMutex);
				m_subtests.push_back(subtestPtr);
			}
			subtestPtr->m_parent = this;
			subtestPtr->m_eventEmitter = m_eventEmitter;
			subtest.release();

			return subtestPtr;
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

		TestFrame(const TestFlags &flags,
				const TestInfo &metadata,
				const std::function<void(TestContext&)> &testFunction = nullptr,
				const std::function<void(TestContext&)> &testSetup = nullptr,
				const std::function<void(TestContext&)> &testTeardown = nullptr)
			: m_eventEmitter(nullptr), flags(flags), metadata(metadata), state(flags.expectFailure == FlagState::Enabled),
				m_testFunction(testFunction), m_testSetup(testSetup), m_testTeardown(testTeardown),
			m_id(nextId()), m_testFrameView(*this) { }

		TestFrame(EventEmitterInterface *eventEmitter,
				const TestFlags &flags,
				const TestInfo &metadata,
				const std::function<void(TestContext&)> &testFunction = nullptr,
				const std::function<void(TestContext&)> &testSetup = nullptr,
				const std::function<void(TestContext&)> &testTeardown = nullptr)
			: m_eventEmitter(eventEmitter), flags(flags), metadata(metadata), state(flags.expectFailure == FlagState::Enabled),
				m_testFunction(testFunction), m_testSetup(testSetup), m_testTeardown(testTeardown),
				m_id(nextId()), m_testFrameView(*this) { }

		TestFrame *addSubtest(
			const TestFlags &flags,
			const TestInfo &metadata,
			const std::function<void(TestContext&)> &testFunction = nullptr,
			const std::function<void(TestContext&)> &testSetup = nullptr,
			const std::function<void(TestContext&)> &testTeardown = nullptr)
		{
			return addSubtest(m_eventEmitter, flags, metadata, testFunction, testSetup, testTeardown);
		}

		TestFrame *addSubtest(
			EventEmitterInterface *eventEmitter,
			const TestFlags &flags,
			const TestInfo &metadata,
			const std::function<void(TestContext&)> &testFunction = nullptr,
			const std::function<void(TestContext&)> &testSetup = nullptr,
			const std::function<void(TestContext&)> &testTeardown = nullptr)
		{
			try
			{
				return addSubtest(partest::make_unique<TestFrame>(m_eventEmitter, flags, metadata, testFunction, testSetup, testTeardown));
			}
			catch(std::bad_alloc)
			{
				recordExceptionLogAndThrow(BadAllocSource::TestCreation, "subtest creation");
			}
		}

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
		void setTestFile(PARTEST_STRING_PARAM fileName)
		{
			try
			{
				metadata.file = fileName;
			}
			catch(std::bad_alloc)
			{
				recordExceptionLogAndThrow(BadAllocSource::TestInfoUpdating, "test file name");
			}
		}

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
			try
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
			catch(std::bad_alloc)
			{
				recordExceptionLogAndThrow(BadAllocSource::AssertionHandling, "assertion processing");
			}
		}

		void recordLog(LogLevel level, PARTEST_STRING_PARAM type, PARTEST_STRING_PARAM message)
		{
			try
			{
				recordLogUnchecked(level, type, message);
			}
			catch(std::bad_alloc)
			{
				recordExceptionLogAndThrow(BadAllocSource::LogRecording, "log recording");
			}
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

		bool isDeconstructing() const
		{
			std::lock_guard<std::mutex> statusLock(m_statusMutex);
			return state.isDeconstructing();
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

		void updateState(const TestResult &result, const TestStatus &status)
		{
			std::unique_lock<std::mutex> statusLock(m_statusMutex, std::defer_lock);
			std::unique_lock<std::mutex> resultLock(m_resultMutex, std::defer_lock);
			std::lock(statusLock, resultLock);
			state.updateResult(result);
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

			try
			{
				m_eventEmitter->emitBeginTest(TestFrameView(*this), m_timeStarted);
				// If effective flags indicate the test should be skipped, do nothing and return immediately
				if(getEffectiveFlags().skip == FlagState::Enabled)
				{
					updateStatus(TestStatus::Skipped);
					m_eventEmitter->emitEndTest(TestFrameView(*this), std::chrono::system_clock::now());
					return false;
				}
			}
			catch(std::bad_alloc &)
			{
				recordExceptionLogAndThrow(BadAllocSource::TestInitialization, "test initialization");
			}

			try
			{
				updateStatus(TestStatus::SettingUp);
				if(m_testSetup != nullptr)
				{
					m_testSetup(ctx);
				}

				return true;
			}
			catch(partest::FrameworkAllocationFailure &)
			{
				// Rethrow FrameworkAllocationFailure to propagate it to the framework.
				// The exception could have come from several levels deep. Mark the current test as aborted.
				abortTestImmediately();
				throw;
			}
			catch(...)
			{
				try
				{
					std::string message = "Unhandled exception while initializing test '" + metadata.name + "': " + stringFromCurrentException();
					abortTest(message);
				}
				catch(std::bad_alloc &)
				{
					recordExceptionLogAndThrow(BadAllocSource::TestInitialization, "test initialization exception handling");
				}
				return false;
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

			try
			{
				// This precedes the run guard because the guard itself would temporarily override status.
				// Correct status transitions from SettingUp to Aborting on the failed path, skipping TearingDown entirely.
				if(m_testFunction == nullptr)
					throw std::runtime_error("Attempted to run a test function that is not set.");

				struct RunGuard
				{
					TestFrame *frame;
					// updateStatus could theoretically raise std::system_error if the mutex is already locked.
					// It shouldn't, but by contract that makes the destructor not noexcept.
					~RunGuard() noexcept(false)
					{
						frame->m_endTime = std::chrono::steady_clock::now();
						frame->updateStatus(TestStatus::TearingDown);
					}
				} guard{this};

				updateStatus(TestStatus::Running);
				m_startTime = std::chrono::steady_clock::now();
				m_testFunction(ctx);
			}
			// A test returned early due to an assertion failure with stopOnFail enabled
			// Nothing special to do here, but this is necessary to prevent the exception from propagating further.
			
			// Assertion failures indicate that the test has already been marked as Failed, so no additional action is needed here 
			catch(const partest::AssertionFailure &)
			{ }
			// FrameworkAllocationFailure is a fatal error that should not be recoverable. It should be handled by the framework and not by the test code.
			// Rethrow to propagate the error to the framework.
			catch(const partest::FrameworkAllocationFailure &)
			{
				abortTestImmediately();
				throw;
			}
			// Unexpected exceptions will generally indicate errors within the user's test code and must be reported
			catch(...)
			{
				try
				{
					std::string message = "Unhandled exception in test '" + metadata.name + "': " + stringFromCurrentException();
					abortTest(message);
				}
				catch(std::bad_alloc &)
				{
					recordExceptionLogAndThrow(BadAllocSource::TestExecution, "test function exception handling");
				}
			}

			// RunGuard updates the status and end time automatically via RAII when this function exits, even if an exception is thrown.
		}

		void finalizeTest(TestContext& ctx)
		{
			try
			{
				// If effective flags indicate the test should be skipped, do nothing and return immediately
				if(getEffectiveFlags().skip != FlagState::Enabled)
				{
					// Flag state/run status invariant. Test should not be marked as skipped if the skip flag is not set.
					assert(!wasSkipped() && "Invalid test state. Test should not have been skipped without skip flag set.");
					// Run status invariant. A test should only be finalized if it is in the process of tearing down or has been aborted.
					assert(isDeconstructing() && "Test frame is not in a valid state to finalize. Test should be tearing down or aborted.");

			
					for(TestFrame *subtest : m_subtests)
					{
						if(subtest->hasFinishedRunning() || subtest->wasSkipped())
						{
							updateResultFromSubtest(subtest->state);
						}
						else
						{
							recordLogUnchecked(LogLevel::Warning, LOG_TYPE_TEST, "Subtest \"" + subtest->fullTestName() + "\" has not completed. This may indicate a problem with the test framework or a test that did not complete properly.");
						}
					}

					if(getEffectiveResult() == TestResult::NoResult)
					{
						recordLogUnchecked(LogLevel::Warning, LOG_TYPE_TEST, "\"" + fullTestName() + "\" completed without any assertions. Defaulting to PASSED.");
						// Shunt a passing value to the state
						state.updateResultFromAssertion(true);
					}

					try
					{
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
					// Rethrow FrameworkAllocationFailure to propagate it to the framework.
					catch(partest::FrameworkAllocationFailure &)
					{
						abortTestImmediately();
						throw;
					}
					catch(...)
					{
						recordLogUnchecked(LogLevel::Error, LOG_TYPE_EXCEPTION, "Unhandled exception in test teardown for \"" + fullTestName() + "\".");
						abortTestImmediately();
					}
				}
				m_eventEmitter->emitEndTest(TestFrameView(*this), std::chrono::system_clock::now());
				maybeRaiseOnReturn("", 0, "Stopped on failure in " + metadata.name);
			}
			// Rethrow FrameworkAllocationFailure to propagate it to the framework.
			catch(partest::FrameworkAllocationFailure &)
			{
				throw;
			}
			catch(std::bad_alloc &)
			{
				recordExceptionLogAndThrow(BadAllocSource::TestFinalization, "test finalization");
			}
		}

		// Mark the test to abort immediately, without allowing the test to continue tearing down.
		// This is used when a framework error is raised during test execution, and the test teardown should not be executed.
		void abortTestImmediately()
		{
			updateState(TestResult::Failed, TestStatus::Aborted);
		}

		// Mark the test to be aborted, but allow the test to continue tearing down.
		// This is used when an exception is raised during test execution, but the test teardown should still be executed.
		void abortTest(PARTEST_STRING_PARAM message)
		{
			updateState(TestResult::Failed, TestStatus::Aborting);
			recordLogUnchecked(LogLevel::Error, LOG_TYPE_EXCEPTION, message);
		}

		/**
		* Check whether the current test should raise an assertion failure based on its status and flags. Used in ASSERT macros.
		* 
		* @param file The file where the assertion is being checked. Typically provided by the __FILE__ macro.
		* @param line The line number where the assertion is being checked. Typically provided by the __LINE__ macro.
		* @param condition The condition being asserted, as a string. Typically provided by the condition expression itself.
		* @throws AssertionFailure if the current test has failed and stopOnFail is enabled.
		*/
		void maybeRaiseOnAssertion(const char *file, int line, PARTEST_STRING_PARAM condition)
		{
			if(getEffectiveFlags().stopOnFail == FlagState::Enabled && (hasFailures()))
			{
				throw AssertionFailure(file, line, condition);
			}
		}

		/**
		* Check whether the current test should raise an assertion failure based on its status and flags.
		* 
		* @param file The file where the assertion is being checked. Typically provided by the __FILE__ macro.
		* @param line The line number where the assertion is being checked. Typically provided by the __LINE__ macro.
		* @param condition The condition being asserted, as a string. Typically provided by the condition expression itself.
		* 
		* @throws AssertionFailure if the current test has failed and stopOnFail is enabled, but NOT on the root test frame. Since this is called from finalize, raising an exception on the root would be meaningless.
		*/
		void maybeRaiseOnReturn(const char *file, int line, PARTEST_STRING_PARAM condition)
		{
			if(m_parent != nullptr && getEffectiveFlags().stopOnFail == FlagState::Enabled && getTestFailureCount())
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
		void maybeRaiseOnAssertion(const AssertionResult &result, TestFrame *test) { maybeRaiseOnAssertion(result.file.c_str(), result.line, result.getCondition()); }

		/**
		* Process an evaluated assertion. Log it and raise an exception if necessary.
		* 
		* @param result Output of an evaluated assertion. AssertionResults should be produced by assertion handlers.
		* @throws AssertionFailure if the assertion result did not pass and stopOnFail is enabled.
		*/
		void commitAssertion(const AssertionResult &result)
		{
			// Pass the assertion result on to the test frame
			processAssertion(result);

			// On failure, allow an exception to be raised if the current test frame is configured to do so.
			if(!result.passed())
				maybeRaiseOnAssertion(result.file.c_str(), result.line, result.getCondition());
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
