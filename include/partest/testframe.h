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
#include <partest/assert.h>
#include <partest/eventemitterinterface.h>
#include <partest/frameworkcontext.h>

namespace partest
{
	PARTEST_INLINE_VAR_17 constexpr unsigned NO_TEST_ID = 0;

	class TestFrame;
	class Event;

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
		TestOutcome getEffectiveResult() const;
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
		std::unique_ptr<Event> m_endTestEvent; // Pre-created end test event to avoid allocation during teardown

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
		mutable std::mutex m_stateMutex; // Mutex for synchronizing access to test state

		
		/**
		* Get a globally incrementing counter. Used internally to assign IDs to newly created test frames.
		* 
		* @return the next value for frameCount
		*/
		static unsigned int nextId() noexcept {
			static std::atomic<unsigned int> frameCount(NO_TEST_ID + 1);
			return frameCount.fetch_add(1, std::memory_order_relaxed);
		}

		/**
		* Add a log entry to the current test frame.
		*
		* @param entry The log entry to be added
		* @return true if the log entry was added successfully, false if the test has already finished running
		* @throws std::bad_alloc if memory allocation fails while adding the log entry
		*/
		bool pushLogEntry(const LogEntry &entry)
		{
			std::unique_lock<std::mutex> logLock(m_logsMutex, std::defer_lock);
			std::unique_lock<std::mutex> stateLock(m_stateMutex, std::defer_lock);
			std::lock(logLock, stateLock);

			if(state.hasFinishedRunning())
				return false;

			m_logs.push_back(entry);

			return true;
		}

		/**
		* Emit a log entry to the event emitter.
		*
		* @param entry The log entry to be emitted
		* @throws std::bad_alloc if memory allocation fails while emitting the log entry
		*/
		void emitLog(const LogEntry &entry)
		{
			m_eventEmitter->emitLog(&m_testFrameView, entry, std::chrono::system_clock::now());
		}

		void handleBadAlloc(BadAllocSource source, bool userCode, bool rethrow, bool isNewFailure)
		{
			abortAndCancelSubtests(userCode ? FailureMode::UserOutOfMemory : FailureMode::FrameworkOutOfMemory, source);

			if(isNewFailure)
				FrameworkContext::badAllocCount().fetch_add(1, std::memory_order_relaxed);

			if(rethrow)
				throw;
		}

		/**
		* Handle unknown exceptions by aborting and canceling subtests, and logging the exception.
		*
		* @throws FrameworkAllocationFailure if memory allocation fails while handling the exception
		*/
		void handleUnknownExceptions()
		{
			TestStatus status = getStatus();
			abortAndCancelSubtests(FailureMode::Exception);
			std::string message;
			try
			{
				if(status == TestStatus::SettingUp)
					message = "Unhandled exception while initializing test '" + metadata.name + "': " + stringFromCurrentException();
				else if(status == TestStatus::Running)
					message = "Unhandled exception in test '" + metadata.name + "': " + stringFromCurrentException();
				else if(status == TestStatus::TearingDown)
					message = "Unhandled exception while finalizing test '" + metadata.name + "': " + stringFromCurrentException();
				else
					message = "Unhandled exception in test '" + metadata.name + "' with test status '" + maybeStringify(status) + "': " + stringFromCurrentException();
				LogEntry log = LogEntry(LogLevel::Error, LOG_TYPE_EXCEPTION, message);
				if(pushLogEntry(log))
					emitLog(log);
			}
			catch(std::bad_alloc &)
			{
				handleBadAlloc(BadAllocSource::TestExecution, false, false, true);
			}
		}

		/**
		* Add an assertion result to the current test frame.
		*
		* @param result The assertion result to be added
		* @return true if the assertion result was added successfully, false if the test has already finished running
		* @throws std::bad_alloc if memory allocation fails while adding the assertion result
		*/
		bool pushAssertion(const AssertionResult &result)
		{
			std::unique_lock<std::mutex> assertionLock(m_assertionsMutex, std::defer_lock);
			std::unique_lock<std::mutex> stateLock(m_stateMutex, std::defer_lock);
			std::lock(assertionLock, stateLock);

			if(state.hasFinishedRunning())
				return false;

			m_assertions.push_back(result);
			state.updateResultFromAssertion(result.passed());
			return true;
		}

		/**
		* Emit an assertion result to the event emitter.
		*
		* @param result The assertion result to be emitted
		* @throws std::bad_alloc if memory allocation fails while emitting the assertion result
		*/
		void emitAssertion(const AssertionResult &result)
		{
			m_eventEmitter->emitAssertion(&m_testFrameView, result, std::chrono::system_clock::now());
		}
		
		/**
		* Process an assertion result by pushing it and emitting it.
		*
		* @param result The assertion result to be processed
		* @return true if the assertion result was processed successfully, false if the test has already finished running
		* @throws std::bad_alloc if memory allocation fails while processing the assertion result
		*/
		bool processAssertion(const AssertionResult &result)
		{
			try
			{
				if(pushAssertion(result))
				{
					emitAssertion(result);
					return true;
				}
			}
			catch(std::bad_alloc &)
			{
				handleBadAlloc(BadAllocSource::AssertionHandling, false, true, true);
			}
			return false;
		}

		void preallocEndTestEvent()
		{
			if(!m_endTestEvent)
				m_endTestEvent = m_eventEmitter->preallocEndTestEvent(&m_testFrameView);
		}

		/**
		* Add a subtest to the current test frame.
		* 
		* @param subtest Pointer to the subtest to be added
		* @return Pointer to the added subtest
		*/
		TestFrame *addSubtest(std::unique_ptr<TestFrame> subtest)
		{
			assert(m_eventEmitter != nullptr && "Event emitter must be set before adding subtests.");

			TestFrame *subtestPtr = subtest.get();
			{
				std::unique_lock<std::mutex> subtestsLock(m_subtestsMutex, std::defer_lock);
				std::unique_lock<std::mutex> stateLock(m_stateMutex, std::defer_lock);
				std::lock(subtestsLock, stateLock);
				if(state.isDeconstructing() || state.hasFinishedRunning())
					throw TestIntegrityFailure("Cannot add subtest to a test frame that is deconstructing or has finished running.");
				m_subtests.push_back(subtestPtr);
			}
			subtest.release();

			subtestPtr->m_parent = this;

			if(!subtestPtr->m_eventEmitter)
				subtestPtr->m_eventEmitter = m_eventEmitter;
			subtestPtr->preallocEndTestEvent();

			return subtestPtr;
		}

		/**
		* Default constructor for TestFrame.
		*/
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
		using TestFrameIter = std::vector<TestFrame *>::iterator;				// Iterator type for iterating over subtests
		using TestFrameConstIter = std::vector<TestFrame *>::const_iterator;	// Const iterator type for iterating over subtests
		using LogEntryConstIter = std::deque<LogEntry>::const_iterator;			// Const iterator type for iterating over log entries
		using AssertionConstIter = std::deque<AssertionResult>::const_iterator; // Const iterator type for iterating over assertion results

		/**
		* Constructor for TestFrame.
		*
		* @param eventEmitter Pointer to the event emitter
		*/
		TestFrame(EventEmitterInterface *eventEmitter) : m_eventEmitter(eventEmitter), flags(), metadata(), state(), m_id(nextId()), m_testFrameView(*this) { }

		/**
		* Constructor for TestFrame.
		*
		* @param flags The test flags for this test frame
		* @param metadata Metadata for this test frame
		* @param testFunction The function this test frame will invoke when run. Defaults to nullptr.
		* @param testSetup The function this test frame will invoke for pre-run setup. Defaults to nullptr.
		* @param testTeardown The function this test frame will invoke for post-run teardown. Defaults to nullptr.
		* @note testFunction *must* be set before the test frame is run. Attempting to run a testFrame with a null testFunction will result in a TestIntegrityFailure exception being thrown.
		*/
		TestFrame(const TestFlags &flags,
				const TestInfo &metadata,
				const std::function<void(TestContext&)> &testFunction = nullptr,
				const std::function<void(TestContext&)> &testSetup = nullptr,
				const std::function<void(TestContext&)> &testTeardown = nullptr)
			: m_eventEmitter(nullptr), flags(flags), metadata(metadata), state(flags.expectFailure == FlagState::Enabled),
				m_testFunction(testFunction), m_testSetup(testSetup), m_testTeardown(testTeardown),
			m_id(nextId()), m_testFrameView(*this) { }

		/**
		* Constructor for TestFrame.
		*
		* @param eventEmitter Pointer to the event emitter
		* @param flags The test flags for this test frame
		* @param metadata Metadata for this test frame
		* @param testFunction The function this test frame will invoke when run. Defaults to nullptr.
		* @param testSetup The function this test frame will invoke for pre-run setup. Defaults to nullptr.
		* @param testTeardown The function this test frame will invoke for post-run teardown. Defaults to nullptr.
		* @note testFunction *must* be set before the test frame is run. Attempting to run a testFrame with a null testFunction will result in a TestIntegrityFailure exception being thrown.
		*/
		TestFrame(EventEmitterInterface *eventEmitter,
				const TestFlags &flags,
				const TestInfo &metadata,
				const std::function<void(TestContext&)> &testFunction = nullptr,
				const std::function<void(TestContext&)> &testSetup = nullptr,
				const std::function<void(TestContext&)> &testTeardown = nullptr)
			: m_eventEmitter(eventEmitter), flags(flags), metadata(metadata), state(flags.expectFailure == FlagState::Enabled),
				m_testFunction(testFunction), m_testSetup(testSetup), m_testTeardown(testTeardown),
				m_id(nextId()), m_testFrameView(*this)
		{
			m_endTestEvent = m_eventEmitter->preallocEndTestEvent(&m_testFrameView);
		}

		/**
		* Create and add a subtest to the current test frame.
		*
		* @param flags The test flags for the subtest
		* @param metadata Metadata for the subtest
		* @param testFunction The function the subtest will invoke when run. Defaults to nullptr.
		* @param testSetup The function the subtest will invoke for pre-run setup. Defaults to nullptr.
		* @param testTeardown The function the subtest will invoke for post-run teardown. Defaults to nullptr.
		* @return A pointer to the created subtest
		* @throws FrameworkAllocationFailure if memory allocation fails while creating the subtest
		* @throws TestIntegrityFailure if invoked while the current test frame is deconstructing or has finished running
		* @note testFunction *must* be set before the subtest is run. Attempting to run a subtest with a null testFunction will result in a TestIntegrityFailure exception being thrown.
		*/
		TestFrame *addSubtest(
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
			catch(std::bad_alloc &)
			{
				handleBadAlloc(BadAllocSource::TestCreation, false, true, true);
			}
			return nullptr; // This line will never be reached, but is here to satisfy the compiler.
		}

		/**
		* Create and add a subtest to the current test frame.
		*
		* @param eventEmitter Pointer to a specified event emitter
		* @param flags The test flags for the subtest
		* @param metadata Metadata for the subtest
		* @param testFunction The function the subtest will invoke when run. Defaults to nullptr.
		* @param testSetup The function the subtest will invoke for pre-run setup. Defaults to nullptr.
		* @param testTeardown The function the subtest will invoke for post-run teardown. Defaults to nullptr.
		* @return A pointer to the created subtest
		* @throws FrameworkAllocationFailure if memory allocation fails while creating the subtest
		* @throws TestIntegrityFailure if invoked while the current test frame is deconstructing or has finished running
		* @note testFunction *must* be set before the subtest is run. Attempting to run a subtest with a null testFunction will result in a TestIntegrityFailure exception being thrown.
		*/
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
				return addSubtest(partest::make_unique<TestFrame>(eventEmitter, flags, metadata, testFunction, testSetup, testTeardown));
			}
			catch(std::bad_alloc &)
			{
				handleBadAlloc(BadAllocSource::TestCreation, false, true, true);
			}
			return nullptr; // This line will never be reached, but is here to satisfy the compiler.
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

		/**
		* Get a reference to the null TestFrame instance. The null TestFrame is a static instance of TestFrame that represents an empty or uninitialized test frame. It can be used as a placeholder or default value when a valid TestFrame is not available.
		*
		* @return A reference to a static null TestFrame instance
		*/
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
			catch(std::bad_alloc &)
			{
				handleBadAlloc(BadAllocSource::TestInfoUpdating, false, true, true);
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

		void clearAssertions()
		{
			std::lock_guard<std::mutex> assertionLock(m_assertionsMutex);
			m_assertions.clear();
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
			std::lock_guard<std::mutex> stateLock(m_stateMutex);
			state = TestState::defaultState(flags.expectFailure == FlagState::Enabled);
		}

		// TODO: Should this be public? This is probably something only the destructor shoul call.
		// It's a reset function, and it's intended to be used to restart the entire frame.
		// But how should it even be accessed? If a rogue thread references anything inside of it, it could end up with a dangling pointer.
		// Aside from this call, the public interface guarantees that a test tree is not deleted after creation, so this violates that guarantee.
		void clearAll() 
		{
			clearAssertions();
			clearLogs(); 
			clearSubtests(); 
			resetState();
		}

		// Add locks around accessors and mutators to status and result.
		TestStatus getStatus() const
		{
			std::lock_guard<std::mutex> stateLock(m_stateMutex);
			return state.getStatus();
		}
		TestOutcome getEffectiveResult() const
		{
			std::lock_guard<std::mutex> stateLock(m_stateMutex);
			return state.getEffectiveResult();
		}

		bool isRunning() const
		{
			std::lock_guard<std::mutex> stateLock(m_stateMutex);
			return state.isRunning();
		}

		bool isDeconstructing() const
		{
			std::lock_guard<std::mutex> stateLock(m_stateMutex);
			return state.isDeconstructing();
		}

		bool hasFinishedRunning() const
		{
			std::lock_guard<std::mutex> stateLock(m_stateMutex);
			return state.hasFinishedRunning();
		}

		bool hasFailures() const
		{
			std::lock_guard<std::mutex> stateLock(m_stateMutex);
			return state.hasFailures();
		}

		bool wasSkipped() const
		{
			std::lock_guard<std::mutex> stateLock(m_stateMutex);
			return state.wasSkipped();
		}

		bool getExpectFailure() const
		{
			std::lock_guard<std::mutex> stateLock(m_stateMutex);
			return state.getExpectFailure();
		}

		bool isAborting() const
		{
			std::lock_guard<std::mutex> stateLock(m_stateMutex);
			return state.isAborting();
		}

		bool hasBeenAborted() const
		{
			std::lock_guard<std::mutex> stateLock(m_stateMutex);
			return state.hasBeenAborted();
		}

		FailureMode getFailureMode() const
		{
			std::lock_guard<std::mutex> stateLock(m_stateMutex);
			return state.getFailureMode();
		}

		void updateResult(const TestResult &result)
		{
			std::lock_guard<std::mutex> stateLock(m_stateMutex);
			state.updateResult(result);
		}

		void updateResultFromAssertion(bool passed)
		{
			std::lock_guard<std::mutex> stateLock(m_stateMutex);
			state.updateResultFromAssertion(passed);
		}

		void updateFromSubtestState(const TestState &subtestState)
		{
			std::lock_guard<std::mutex> stateLock(m_stateMutex);
			state.updateFromSubtestState(subtestState);
		}

		bool updateStatus(TestStatus status)
		{
			std::lock_guard<std::mutex> stateLock(m_stateMutex);
			return state.updateStatus(status);
		}

		void updateState(TestResult result, TestStatus status, FailureMode failureMode = FailureMode::None)
		{
			std::lock_guard<std::mutex> stateLock(m_stateMutex);
			state.updateResult(result);
			state.updateStatus(status);
			state.updateFailureMode(failureMode);
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

		/**
		* Set up the test frame prior to test execution, notify the event emitter, and update status. If set, invokes the test setup function.
		*
		* @param ctx The test context to be passed to the test setup function
		* @return true if the test should be run, false if it was skipped
		* @throws FrameworkAllocationFailure if memory allocation fails during setup
		*/
		bool initializeTest(TestContext& ctx)
		{
			try
			{
				// TODO: Is this right? The timestamp is different from the actual test's run time, which makes sense for *profiling, but it doesn't include setup time.
				// Should I remove the timestamps from around run() and use a different mechanism for profiling time?
				m_eventEmitter->emitBeginTest(&m_testFrameView, std::chrono::system_clock::now());
				// If effective flags indicate the test should be skipped, do nothing and return immediately
				if(getEffectiveFlags().skip == FlagState::Enabled)
				{
					updateStatus(TestStatus::Skipped);
					return false;
				}
			}
			catch(std::bad_alloc &)
			{
				handleBadAlloc(BadAllocSource::TestPreInitialization, false, false, true);
				return false;
			}

			// If the test was aborted before it was initialized, we should not run the setup function.
			// This is unrelated to the catch above; another thread could have aborted the test before
			// it was initialized regardless of bad_alloc here.
			if(!updateStatus(TestStatus::SettingUp))
			{
				return false;
			}

			try
			{
				if(m_testSetup != nullptr)
				{
					m_testSetup(ctx);
				}
				return true;
			}
			catch(const TestIntegrityFailure &)
			{
				// Don't do anything. These exceptions are only raised when the test is already in an invalid state.
			}
			catch(const FrameworkAllocationFailure &)
			{
				// These are exceptions originating from deep in the framework, and from points where the failure can't be recorded in the test state.
				handleBadAlloc(BadAllocSource::TestInitialization, false, false, false);
			}
			catch(std::bad_alloc  &)
			{
				// Check whether the allocation failure is already marked. If it is, then it was a framework failure,
				// likely caused by a log or assertion (probably incorrectly) invoked from the setup function.
				// In this case the test has already been aborted and the failure mode set, so we don't need to do anything further.
				//
				// Otherwise, it is a user code error. We'll mark it aborted and allow the teardown to clean up.
				if(getFailureMode() != FailureMode::FrameworkOutOfMemory)
				{
					handleBadAlloc(BadAllocSource::TestInitialization, true, false, true);
				}
			}
			catch(...)
			{
				handleUnknownExceptions();
			}
			return false;
		}

		/**
		* Run the test function associated with this test frame and progress the test state accordingly. Updates status and end time when finished.
		*
		* @param ctx The test context to be passed to the test function
		* @throws FrameworkAllocationFailure if memory allocation fails during test execution
		*/
		void runTestFunction(TestContext& ctx)
		{
			// Framework invariant: A test should not be run if it was skipped. This is enforced by the test framework, and should never be violated.
			assert(!wasSkipped() && "Invalid test state. Test should not be run if it was skipped.");

			// This precedes the run guard because the guard itself would temporarily override status.
			// Correct status transitions from SettingUp to Aborting on the failed path, skipping TearingDown entirely.
			if(m_testFunction == nullptr)
			{
				abortAndCancelSubtests(FailureMode::NoTestFunction);
				return;
			}

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
			
			try
			{
				if(!updateStatus(TestStatus::Running))
				{
					m_startTime = std::chrono::steady_clock::now();
					return;
				}
				// Setting start time after locking the status mutex to minimize overhead of the test execution time measurement.
				m_startTime = std::chrono::steady_clock::now();

				// TODO: What happens if a user catches (...) and swallows an exception?
				// If StopOnFail is enabled and the result is "failed", then I can check that here. It means an assertion should have bubbled up and did not.
				// I should consider logging a test integrity error if that happens. It's an indication that the user misused a catch block.
				// Maybe I should provide a PARTEST_RETHROW macro that simply rethrows my exception types, which can be placed between the user code and his catch block.
				m_testFunction(ctx);
			}
			// A test returned early due to an assertion failure with stopOnFail enabled
			// Nothing special to do here, but this is necessary to prevent the exception from propagating further.
			
			// Assertion failures indicate that the test has already been marked as Failed, so no additional action is needed here 
			catch(const partest::AssertionFailure &)
			{ }
			// FrameworkAllocationFailure is a fatal error that should not be recoverable. It should be handled by the framework and not by the test code.
			// Rethrow to propagate the error to the framework.
			catch(std::bad_alloc &)
			{
				// Check whether the allocation failure is already marked. If it is, then it was a framework failure,
				// likely from a log, assertion, or subtest creation that failed during test execution.
				// In this case the test has already been aborted and the failure mode set, so we don't need to do anything further.
				// Otherwise, it is a user code error. We'll mark it aborted and allow the teardown to clean up.
				if(getFailureMode() != FailureMode::FrameworkOutOfMemory)
				{
					handleBadAlloc(BadAllocSource::TestExecution, true, false, true);
				}
			}
			catch(const TestIntegrityFailure &)
			{
				// Test integrity failures indicate that the test has already been marked as Failed, so no additional action is needed here
				// TODO: Re-evaluate this. Is this correct? Currently these errors indicate that a test has been misused, and they are reported directly to the global logger, so there's nothing to be done here. They're primarily controls to stop a rogue thread post-mortem, and if they reach this point that goal has been accomplished.
				// Since every test kills its subtests on teardown, even if this test's parent does something strange, further out of sync access at that level will just log another failure, which is probably fine?
			}
			catch(const FrameworkAllocationFailure &)
			{
				handleBadAlloc(BadAllocSource::TestExecution, false, false, false);
			}
			// Unexpected exceptions will generally indicate errors within the user's test code and must be reported
			catch(...)
			{
				handleUnknownExceptions();
			}

			// RunGuard updates the status and end time automatically via RAII when this function exits, even if an exception is thrown.
		}

		/**
		* Clean up the test frame after test execution, notify the event emitter, and update status. If set, invokes the test teardown function.
		*
		* @param ctx The test context to be passed to the test teardown function
		* @throws FrameworkAllocationFailure if memory allocation fails during test finalization
		* @throws AssertionFailure if the test has failed and stopOnFail is enabled, except on a root test frame
		*/
		void finalizeTest(TestContext& ctx)
		{
			assert(m_endTestEvent != nullptr && "End test event must be preallocated before finalizing the test.");

			struct EndGuard
			{
				TestFrame *frame;
				// True if another exception is already in flight.
				bool exceptionInFlight;
				~EndGuard() noexcept(false)
				{
					try
					{
						// TODO: Ensure that emitEvent actually can't allocate memory.
						frame->m_eventEmitter->emitEndTest(std::move(frame->m_endTestEvent), std::chrono::system_clock::now());
						// TODO: Maybe find a way to avoid memory allocation in maybeRaiseOnReturn.
						// Currently constructs a string in place, which could throw std::bad_alloc.
						frame->maybeRaiseOnReturn();
					}
					catch(std::bad_alloc &)
					{
						// This should be impossible to reach now, unless something went wrong during the emitEndTest or maybeRaiseOnAssertion calls.
						// Regardless, I think this still counts as an allocation failure in finalize, so it needs to be handled the same as elsewhere in this function.
						frame->handleBadAlloc(BadAllocSource::TestFinalization, false, false, true);
					}
					// Under normal conditions this should propagate, but other possible exceptions result in aborted/failed tests.
					// Since maybeRaiseOnReturn only forwards AssertionFailure, it would be semantically the same as any other exception caught at this point.
					catch(AssertionFailure &)
					{
						if(!exceptionInFlight)
							throw;
					}
				}
			} guard{this, true};

			// If effective flags indicate the test should be skipped, do nothing and return immediately
			if(getEffectiveFlags().skip == FlagState::Enabled)
			{
				updateStatus(TestStatus::Skipped);
				guard.exceptionInFlight = false;
				return;
			}
			// Test was cancelled before setup ran. Don't do anything else, but let the guard run to emit the end test event and raise any exceptions.
			else if(hasBeenAborted())
			{
				guard.exceptionInFlight = false;
				return;
			}

			// Flag state/run status invariant. Test should not be marked as skipped if the skip flag is not set.
			assert(!wasSkipped() && "Invalid test state. Test should not have been skipped without skip flag set.");
			// Run status invariant. A test should only be finalized if it is in the process of tearing down or has been aborted.
			assert(isDeconstructing() && "Test frame is not in a valid state to finalize. Test should be tearing down or aborted.");

			try
			{
				bool subtestsFinished = true;
				std::unique_lock<std::mutex> subtestsLock(m_subtestsMutex);
				for(TestFrame *subtest : m_subtests)
				{
					if(subtest->hasFinishedRunning() || subtest->wasSkipped())
					{
						updateFromSubtestState(subtest->state);
					}
					// Log still running subtests if the current test is not aborting. No subtests should be in progress under normal circumstances.
					// TODO: Should the current test be flagged as aborting if a subtest is still running? Probably.
					else if(!isAborting())
					{
						updateResultFromAssertion(false);
						subtestsFinished = false;
						break;
					}
				}
				subtestsLock.unlock();

				if(!subtestsFinished)
				{
					abortAndCancelSubtests(FailureMode::DanglingThread);
					LogEntry log = LogEntry(LogLevel::Error, LOG_TYPE_TEST, "Test \"" + fullTestName() + "\" has not completed all subtests.");
					if(pushLogEntry(log))
						emitLog(log);
				}

				// Don't even bother trying to log anything if the test is aborting due to an out-of-memory condition.
				// The log allocation will likely fail and throw again, plus the memory error is more important than an empty test.
				// It would also be incorrect to mark the test as passing on an abort.
				if(getEffectiveResult() == TestOutcome::NoResult && getFailureMode() != FailureMode::UserOutOfMemory)
				{
					LogEntry log = LogEntry(LogLevel::Warning, LOG_TYPE_TEST, "Test \"" + fullTestName() + "\" completed without any assertions. Defaulting to PASSED.");
					if(pushLogEntry(log))
						emitLog(log);

					// Shunt a passing value to the state
					updateResultFromAssertion(true);
				}
			}
			catch(std::bad_alloc &)
			{
				// Even here, we still need to attempt to allow teardown to run.
				handleBadAlloc(BadAllocSource::TestFinalization, false, false, true);
			}

			try
			{
				if(m_testTeardown != nullptr)
				{
					m_testTeardown(ctx);
				}
			}
			// Catching this here doesn't mean the test failed, only that something was called out of sequence during teardown.
			catch(const TestIntegrityFailure &)
			{ }
			// Unique error type propagated by TestContext when a failure emerged out-of-sequence with the test frame
			catch(const FrameworkAllocationFailure &)
			{
				// These are exceptions originating from deep in the framework, and from points where the failure can't be recorded in the test state.
				// This *should* only be possible from a detached thread.
				handleBadAlloc(BadAllocSource::DesyncedTestState, false, false, false);
			}
			catch(std::bad_alloc &)
			{
				// Check whether the allocation failure is already marked. If it is, then it was a framework failure,
				// likely from a log or assertion invoked (probably incorrectly) from the teardown function.
				// In this case the test has already been aborted and the failure mode set, so we don't need to do anything further.

				// Otherwise, it is a user code error. We'll mark it aborted.
				// Teardown cannot be completed, but we'll ensure subtests are marked as well, if necessary.
				if(getFailureMode() != FailureMode::FrameworkOutOfMemory)
				{
					handleBadAlloc(BadAllocSource::TestFinalization, true, false, true);
				}
			}
			catch(...)
			{
				handleUnknownExceptions();
			}

			updateStatus(TestStatus::Completed);
			guard.exceptionInFlight = false;
		}

		void abortAndCancelSubtests(FailureMode reason, BadAllocSource source = BadAllocSource::Unknown)
		{
			std::unique_lock<std::mutex> stateLock(m_stateMutex);

			// We're already inside the lock, so we have to call the component functions directly.
			TestStatus status = state.getStatus();

			// We only need to abort if the test is still running. If it has finished, then there's nothing to do.
			if(state.hasFinishedRunning() || getEffectiveFlags().skip == FlagState::Enabled)
				return;

			// A second abort request with the same reason is redundant, so we can ignore it.
			if(state.getFailureMode() == reason)
				return;


			// Check if the test is already aborting. If it is, we don't want to override the existing failure mode unless it's KilledByParent.
			// The reason for  this exception is that if a new failure happens after the parent has already killed the test,
			// the new failure reason is more specific. For every other case, we don't want to override the existing failure mode.
			if(state.isAborting())
			{
				// Do nothing unless the existing failure mode is specifically KilledByParent.
				// If it is, then we want to update the failure mode to the new reason, since the new reason is more specific than KilledByParent.
				if(state.getFailureMode() != FailureMode::KilledByParent)
					return;

				state.updateFailureMode(reason, source);
				return;
			}

			state.updateResult(TestResult::Failed);
			
			// Skip teardown if setup never happened.
			if(status >= TestStatus::SettingUp)
				state.updateStatus(TestStatus::TearingDown);
			else
				state.updateStatus(TestStatus::Completed);

			state.updateFailureMode(reason, source);
			stateLock.unlock();

			std::lock_guard<std::mutex> subtestsLock(m_subtestsMutex);
			for(TestFrame *subtest : m_subtests)
			{
				// Don't attribute bad alloc to the subtests. That would result in confusing reports.
				subtest->abortAndCancelSubtests(FailureMode::KilledByParent);
			}
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
		* @throws AssertionFailure if the current test has failed and stopOnFail is enabled, but NOT on the root test frame. Since this is called from finalize, raising an exception on the root would be meaningless.
		*/
		void maybeRaiseOnReturn()
		{
			if(m_parent != nullptr && getEffectiveFlags().stopOnFail == FlagState::Enabled && getTestFailureCount())
			{
				throw AssertionFailure("", 0, "Stopped on failure in " + metadata.name);
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
		* @return true if the assertion was processed successfully, false if the test has already finished running and the assertion could not be recorded.
		* @throws AssertionFailure if the assertion result did not pass and stopOnFail is enabled.
		* @throws TestIntegrityFailure if the test has already finished running and the assertion could not be recorded.
		*/
		bool commitAssertion(const AssertionResult &result)
		{
			// Pass the assertion result on to the test frame
			if(!processAssertion(result))
				throw TestIntegrityFailure("Cannot record assertion for a test frame that has finished running.");

			// On failure, allow an exception to be raised if the current test frame is configured to do so.
			if(!result.passed())
				maybeRaiseOnAssertion(result.file.c_str(), result.line, result.getCondition());

			return true;
		}

		/**
		* Record a log entry for the current test frame.
		* 
		* @param level The severity level of the log entry
		* @param type The type or category of the log entry
		* @param message The log message
		* @return true if the log entry was recorded successfully
		* @throws TestIntegrityFailure if the test frame has finished running and the log entry cannot be recorded
		* @throws FrameworkAllocationFailure if memory allocation fails while recording the log entry
		*/
		bool recordLog(LogLevel level, PARTEST_STRING_PARAM type, PARTEST_STRING_PARAM message)
		{
			try
			{
				LogEntry log = LogEntry(level, type, message);
				if(pushLogEntry(log))
				{
					emitLog(log);
					return true;
				}
				else
				{
					throw TestIntegrityFailure("Cannot record log entry for a test frame that has finished running.");
				}
			}
			catch(std::bad_alloc &)
			{
				handleBadAlloc(BadAllocSource::LogRecording, false, true, true);
			}
			return false;
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

		/**
		* Count the total number of assertions in this frame's subtest tree
		* 
		* @param onlyCountFailures If true, only count assertions that failed
		* @returns The total number of assertions
		*/
		size_t getAssertionCount(bool onlyCountFailures = false) const
		{
			size_t total = 0;
			if(!onlyCountFailures)
			{
				m_assertionsMutex.lock();
				//guaranteed noexcept and no early return
				total = m_assertions.size();
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
			std::lock_guard<std::mutex> stateLock(m_stateMutex);
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
	inline TestOutcome TestFrameView::getEffectiveResult() const { return m_testFrame->getEffectiveResult(); }
	inline bool TestFrameView::getExpectFailure() const { return m_testFrame->getExpectFailure(); }

	inline std::chrono::steady_clock::time_point TestFrameView::startTime() const noexcept { return m_testFrame->startTime(); }
	inline std::chrono::steady_clock::time_point TestFrameView::endTime() const noexcept { return m_testFrame->endTime(); }

	inline Timestamp TestFrameView::timestamp() const noexcept { return m_testFrame->timestamp(); }
}

#endif // PARTESTTESTFRAME_H
