#ifndef PARTEST_TEST_CONTEXT_H
#define PARTEST_TEST_CONTEXT_H

#include <partest/frameworkcontext.h>
#include <partest/testframe.h>

namespace partest
{
	class TestContext
	{
		enum class FailureMode
		{
			PostTestRunPhase,			// Failure occurred after the test frame had finished running, but before the runner itself shut down
			PostFrameworkTeardown	// Failure occurred after the entire framework had finished running, and the runner itself was shutting down
		};

		TestFrame *m_currentFrame;
		//Replace test suite ref with a function pointer for runTest, to avoid circular dependency. This will be a function pointer to TestBase::runTest
		void (*m_runTestFunc)(TestFrame *test);

		// If true, this context is operating during test teardown. Otherwise it is assumed to be during setup or execution.
		// This only matters in threaded execution contexts, where a thread may call back into a test frame at unexpected times.
		bool m_inTeardown;

		void recordSubtestFailure(PARTEST_STRING_PARAM testFrame, PARTEST_STRING_PARAM subtestFrame, FailureMode failureMode)
		{
			bool badAllocOccurred = false;
			try
			{
				switch(failureMode)
				{
				case FailureMode::PostTestRunPhase:
					FrameworkContext::writeGlobalLog(LogLevel::Error, partest::LOG_TYPE_EXCEPTION, "Attempted to add subtest '"
						+ PARTEST_STRING_PARAM_TO_STRING(subtestFrame) + "' after test '"
						+ PARTEST_STRING_PARAM_TO_STRING(testFrame) + "' completed its run phase.");
					break;
				case FailureMode::PostFrameworkTeardown:
					FrameworkContext::writeGlobalLog(LogLevel::Error, partest::LOG_TYPE_EXCEPTION, "Attempted to add subtest '"
						+ PARTEST_STRING_PARAM_TO_STRING(subtestFrame) + "' after the test runner concluded. This indicates that a detached thread awoke post-teardown.");
					break;
				}
			}
			catch(std::bad_alloc &)
			{
				badAllocOccurred = true;
				FrameworkContext::badAllocCount().fetch_add(1, std::memory_order_relaxed);
			}

			// Use simple nonstandard exception to bubble up past user code that may catch std::bad_alloc.
			// The separation from the throw block is to ensure that in concurrent contexts, where multiple threads may be trying to log failures, we don't take up an extra emergency exception slot in the stack. By using a flag here, the original bad_alloc is freed first. We still run the risk of termination if too many threads hit this at once, but this pattern allows us to use a custom type without taking up the extra slot.
			// This is a last-resort measure to ensure that the framework can report the failure, even if the user code has a catch-all for std::bad_alloc.
			if(badAllocOccurred)
				throw FrameworkAllocationFailure();
		}

		void recordAssertionFailure(PARTEST_STRING_PARAM testFrame, const AssertionResult &result, FailureMode failureMode)
		{
			bool badAllocOccurred = false;
			try
			{
				switch(failureMode)
				{
				case FailureMode::PostTestRunPhase:
					FrameworkContext::writeGlobalLog(LogLevel::Error, partest::LOG_TYPE_EXCEPTION, "Attempted to process an assertion for test '"
						+ PARTEST_STRING_PARAM_TO_STRING(testFrame) + "' after it completed its run phase.\nAssertion of type "
						+ result.assertType() + " originated from: " + maybeStringify(result.file) + ':' + maybeStringify(result.line));
					break;
				case FailureMode::PostFrameworkTeardown:
					FrameworkContext::writeGlobalLog(LogLevel::Error, partest::LOG_TYPE_EXCEPTION, "Attempted to process an assertion after the runner concluded. This indicates that a detached thread awoke post-teardown.\nAssertion of type "
						+ result.assertType() + " originated from: " + maybeStringify(result.file) + ':' + maybeStringify(result.line));
					break;
				}
			}
			catch(std::bad_alloc &)
			{
				badAllocOccurred = true;
				FrameworkContext::badAllocCount().fetch_add(1, std::memory_order_relaxed);
			}

			// Use simple nonstandard exception to bubble up past user code that may catch std::bad_alloc.
			// This is a last-resort measure to ensure that the framework can report the failure, even if the user code has a catch-all for std::bad_alloc.
			if(badAllocOccurred)
				throw FrameworkAllocationFailure();
		}

		void recordLogFailure(PARTEST_STRING_PARAM testFrame, LogLevel level, PARTEST_STRING_PARAM type, PARTEST_STRING_PARAM message, FailureMode failureMode)
		{
			bool badAllocOccurred = false;
			try
			{
				switch(failureMode)
				{
				case FailureMode::PostTestRunPhase:
					FrameworkContext::writeGlobalLog(LogLevel::Error, partest::LOG_TYPE_EXCEPTION, "Attempted to add log entry to test '"
						+ PARTEST_STRING_PARAM_TO_STRING(testFrame) + "' after it completed its run phase.\n" + PARTEST_STRING_PARAM_TO_STRING(message));
					break;
				case FailureMode::PostFrameworkTeardown:
					FrameworkContext::writeGlobalLog(LogLevel::Error, partest::LOG_TYPE_EXCEPTION, "Attempted to add log entry to test after the runner concluded. This indicates that a detached thread awoke post-teardown.\nLog message: " + PARTEST_STRING_PARAM_TO_STRING(message));
					break;
				}
			}
			catch(std::bad_alloc &)
			{
				badAllocOccurred = true;
				FrameworkContext::badAllocCount().fetch_add(1, std::memory_order_relaxed);
			}

			// Use simple nonstandard exception to bubble up past user code that may catch std::bad_alloc.
			// This is a last-resort measure to ensure that the framework can report the failure, even if the user code has a catch-all for std::bad_alloc.
			if(badAllocOccurred)
				throw FrameworkAllocationFailure();
		}

		void recordMetadataFailure(PARTEST_STRING_PARAM testFrame, PARTEST_STRING_PARAM metadataKey, PARTEST_STRING_PARAM metadataValue, FailureMode failureMode)
		{
			bool badAllocOccurred = false;
			try
			{
				switch(failureMode)
				{
				case FailureMode::PostTestRunPhase:
					FrameworkContext::writeGlobalLog(LogLevel::Error, partest::LOG_TYPE_EXCEPTION, "Attempted to set '"
						+ PARTEST_STRING_PARAM_TO_STRING(metadataKey) + "' to '"
						+ PARTEST_STRING_PARAM_TO_STRING(metadataValue) + "' on test '"
						+ PARTEST_STRING_PARAM_TO_STRING(testFrame) + "' after it concluded. This indicates that a detached thread awoke post-teardown.");
					break;
				case FailureMode::PostFrameworkTeardown:
					FrameworkContext::writeGlobalLog(LogLevel::Error, partest::LOG_TYPE_EXCEPTION, "Attempted to set '"
						+ PARTEST_STRING_PARAM_TO_STRING(metadataKey) + "' to '"
						+ PARTEST_STRING_PARAM_TO_STRING(metadataValue) + "' after the test runner concluded. This indicates that a detached thread awoke post-teardown.");
					break;
				}
			}
			catch(std::bad_alloc &)
			{
				badAllocOccurred = true;
				FrameworkContext::badAllocCount().fetch_add(1, std::memory_order_relaxed);
			}

			// Use simple nonstandard exception to bubble up past user code that may catch std::bad_alloc.
			// This is a last-resort measure to ensure that the framework can report the failure, even if the user code has a catch-all for std::bad_alloc.
			if(badAllocOccurred)
				throw FrameworkAllocationFailure();
		}

		/**
		* bad_alloc guarded factory function for TestInfo.
		* This ensures that if a bad_alloc occurs during the construction of a TestInfo object, it is handled gracefully and reported to the framework.
		*/
		TestInfo makeTestInfo(PARTEST_STRING_PARAM name, PARTEST_STRING_PARAM description = "", PARTEST_STRING_PARAM file = "", unsigned int line = 0)
		{
			FrameworkContext::LifetimeGuard guard;
			try
			{
				return TestInfo(name, description, file, line);
			}
			catch(const std::bad_alloc &)
			{
				if(guard.isAlive())
					m_currentFrame->handleBadAlloc(BadAllocSource::TestCreation, false, true);
				else
					FrameworkContext::badAllocCount().fetch_add(1, std::memory_order_relaxed);
			}
			throw TestAllocationFailure();
		}

	public:
		TestContext(TestFrame *currentFrame, void (*runTestFunc)(TestFrame *test), bool inTeardown) noexcept
			: m_currentFrame(currentFrame), m_runTestFunc(runTestFunc), m_inTeardown(inTeardown) { }

		template<PARTEST_INVOCABLE_WITH(Func, TestContext&)>
		void subtest(PARTEST_STRING_PARAM name, Func &&testFunc)
		{ subtest(makeTestInfo(name), TestFlags::defaultInherit(), testFunc); }

		template<PARTEST_INVOCABLE_WITH(Func, TestContext&)>
		void subtest(PARTEST_STRING_PARAM name, PARTEST_STRING_PARAM description, Func &&testFunc)
		{ subtest(makeTestInfo(name, description), TestFlags::defaultInherit(), testFunc); }
		template<PARTEST_INVOCABLE_WITH(Func, TestContext&)>
		void subtest(PARTEST_STRING_PARAM name, const TestFlags& flags, Func &&testFunc)
		{ subtest(makeTestInfo(name), flags, testFunc); }

		template<PARTEST_INVOCABLE_WITH(Func, TestContext&)>
		void subtest(PARTEST_STRING_PARAM name, PARTEST_STRING_PARAM description, const TestFlags& flags, Func &&testFunc)
		{ subtest(makeTestInfo(name, description), flags, testFunc); }
		template<PARTEST_INVOCABLE_WITH(Func, TestContext&)>
		void subtest(Func &&testFunc)
		{ subtest(TestInfo::defaultInfo(), TestFlags::defaultInherit(), testFunc); }

		template<PARTEST_INVOCABLE_WITH(Func, TestContext&)>
		void subtest(const TestFlags& flags, Func &&testFunc)
		{ subtest(TestInfo::defaultInfo(), flags, testFunc); }

		template<PARTEST_INVOCABLE_WITH(Func, TestContext&)>
		void subtest(const TestInfo &testInfo, Func &&testFunc)
		{ subtest(testInfo, TestFlags::defaultInherit(), testFunc); }

		template<PARTEST_INVOCABLE_WITH(Func, TestContext&)>
		void subtest(const TestInfo &testInfo, const TestFlags& flags, Func &&testFunc)
		{
			assert(m_currentFrame != nullptr && "Parent test frame is null. Subtests must be added to a valid parent test frame.");
			TestFrame *newSubtest = nullptr;

			FrameworkContext::LifetimeGuard guard;
			if(!guard.isAlive())
			{
				// If the framework context is no longer alive, log the error with the runner and throw an exception to indicate that this call was made after the test had finished running.
				// This should only happen if the subtest was added from a different thread after the test has completed.
				recordSubtestFailure("", testInfo.name, FailureMode::PostFrameworkTeardown);
				throw TestIntegrityFailure("Attempted to add subtest '" + testInfo.name + "' after the test runner concluded.");
			}

			bool testAllocFailed = false;
			try
			{
				// During teardown, only a context invoked through the teardown function is valid.
				// If the context is *not* in teardown, this indicates it was invoked via a thread spawned during the test.
				// Any use of a non-teardown context is a trigger to kill the thread.
				if(!m_inTeardown && m_currentFrame->hasStartedTeardown())
					throw TestIntegrityFailure("Caught test worker thread running during teardown.");

				newSubtest = m_currentFrame->addSubtest(flags, testInfo, testFunc);
			}
			catch(TestIntegrityFailure &)
			{
				// If the subtest cannot be added, log the error with the runner and rethrow the exception to indicate that this call was made after the test had finished running.
				// This should only happen if the subtest was added from a different thread after the test has completed.
				// TODO: Remember that if this is run from a raw thread and the user doesn't catch it, it will terminate the program. This will only function from framework-mananged thread wrappers. See partest::thread once it exists.
				recordSubtestFailure(m_currentFrame->metadata.name, testInfo.name, FailureMode::PostTestRunPhase);
				throw;
			}
			// Because testFunc is a template type, it may require conversion to a std::function, which may allocate memory.
			// If this allocation fails, we need to catch it and handle it gracefully.
			catch(std::bad_alloc &)
			{
				m_currentFrame->handleBadAlloc(BadAllocSource::TestCreation, false, true);
				testAllocFailed = true;
			}
			
			if(testAllocFailed)
				throw TestAllocationFailure();

			m_runTestFunc(newSubtest);
		}

		void commitAssertion(const AssertionResult &result)
		{
			FrameworkContext::LifetimeGuard guard;
			if(!guard.isAlive())
			{
				// If the framework context is no longer alive, log the error with the runner and throw an exception to indicate that this call was made after the test had finished running.
				// This should only happen if the subtest was added from a different thread after the test has completed.		
				recordAssertionFailure("", result, FailureMode::PostFrameworkTeardown);
				throw TestIntegrityFailure("Attempted to commit assertion after the test runner concluded.");
			}

			try
			{
				// During teardown, only a context invoked through the teardown function is valid.
				// If the context is *not* in teardown, this indicates it was invoked via a thread spawned during the test.
				// Any use of a non-teardown context is a trigger to kill the thread.
				if(!m_inTeardown && m_currentFrame->hasStartedTeardown())
					throw TestIntegrityFailure("Caught test worker thread running during teardown.");

				m_currentFrame->commitAssertion(result);
			}
			catch(TestIntegrityFailure &)
			{
				// If the assertion cannot be recorded, log the error with the runner and rethrow the exception to indicate that this call was made after the test had finished running.
				// This should only happen if the assertion was made from a different thread after the test has completed.
				
				// TODO: Remember that if this is run from a raw thread and the user doesn't catch it, it will terminate the program.
				// This will only function from framework-mananged thread wrappers. See partest::thread once it exists.
				recordAssertionFailure(m_currentFrame->metadata.name, result, FailureMode::PostTestRunPhase);
				throw;
			}
		}

		void recordLog(LogLevel level, PARTEST_STRING_PARAM type, PARTEST_STRING_PARAM message)
		{
			FrameworkContext::LifetimeGuard guard;
			if(!guard.isAlive())
			{
				// If the framework context is no longer alive, log the error with the runner and throw an exception to indicate that this call was made after the test had finished running.
				// This should only happen if the subtest was added from a different thread after the test has completed.
				// Can't set the test name here because the frame may have been destroyed already, so just pass an empty string.
				recordLogFailure("", level, type, message, FailureMode::PostFrameworkTeardown);
				throw TestIntegrityFailure("Attempted to record log entry after the test runner concluded.");
			}

			try
			{
				// During teardown, only a context invoked through the teardown function is valid.
				// If the context is *not* in teardown, this indicates it was invoked via a thread spawned during the test.
				// Any use of a non-teardown context is a trigger to kill the thread.
				if(!m_inTeardown && m_currentFrame->hasStartedTeardown())
					throw TestIntegrityFailure("Caught test worker thread running during teardown.");

				m_currentFrame->recordLog(level, type, message);
			}
			catch(TestIntegrityFailure &)
			{
				// If the log entry cannot be recorded, log the error with the runner and rethrow the exception to indicate that this call was made after the test had finished running.
				// This should only happen if the log entry was made from a different thread after the test has completed.
				// TODO: Remember that if this is run from a raw thread and the user doesn't catch it, it will terminate the program.
				// This will only function from framework-mananged thread wrappers. See partest::thread once it exists.
				recordLogFailure(m_currentFrame->metadata.name, level, type, message, FailureMode::PostTestRunPhase);
				throw;
			}
		}

		void setTestFile(PARTEST_STRING_PARAM fileName)
		{
			FrameworkContext::LifetimeGuard guard;
			if(!guard.isAlive())
			{
				// If the framework context is no longer alive, log the error with the runner and throw an exception to indicate that this call was made after the test had finished running.
				// This should only happen if the subtest was added from a different thread after the test has completed.
				recordMetadataFailure("", "file", fileName, FailureMode::PostFrameworkTeardown);
				throw TestIntegrityFailure("Attempted to set test file after the test runner concluded.");
			}
			m_currentFrame->setTestFile(fileName);
		}

		void setTestLine(unsigned line)
		{
			FrameworkContext::LifetimeGuard guard;
			if(!guard.isAlive())
			{
				// If the framework context is no longer alive, log the error with the runner and throw an exception to indicate that this call was made after the test had finished running.
				// This should only happen if the subtest was added from a different thread after the test has completed.
				// Can't pass the test name here because the frame may have been destroyed already, so just pass an empty string.
				recordMetadataFailure("", "line", std::to_string(line), FailureMode::PostFrameworkTeardown);
				throw TestIntegrityFailure("Attempted to set test line after the test runner concluded.");
			}
			m_currentFrame->setTestLine(line);
		}
	};
}
#endif
