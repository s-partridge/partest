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
			PostTestTeardown,		// Failure occurred after the test frame had finished running, but before the runner itself shut down
			PostFrameworkTeardown	// Failure occurred after the entire framework had finished running, and the runner itself was shutting down
		};

		TestFrame *m_currentFrame;
		//Replace test suite ref with a function pointer for runTest, to avoid circular dependency. This will be a function pointer to TestBase::runTest
		void (*m_runTestFunc)(TestFrame *test);

		void recordSubtestFailure(PARTEST_STRING_PARAM testFrame, PARTEST_STRING_PARAM subtestFrame, FailureMode failureMode)
		{
			bool badAllocOccurred = false;
			try
			{
				switch(failureMode)
				{
				case FailureMode::PostTestTeardown:
					FrameworkContext::writeGlobalLog(LogLevel::Error, partest::LOG_TYPE_EXCEPTION, "Attempted to add subtest '"
						+ PARTEST_STRING_PARAM_TO_STRING(subtestFrame) + "' after test '"
						+ PARTEST_STRING_PARAM_TO_STRING(testFrame) + "' concluded. This indicates that a detached thread awoke post-teardown.");
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
				case FailureMode::PostTestTeardown:
					FrameworkContext::writeGlobalLog(LogLevel::Error, partest::LOG_TYPE_EXCEPTION, "Attempted to process an assertion for test '"
						+ PARTEST_STRING_PARAM_TO_STRING(testFrame) + "' after the test concluded. This indicates that a detached thread awoke post-teardown.\nAssertion of type "
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
				case FailureMode::PostTestTeardown:
					FrameworkContext::writeGlobalLog(LogLevel::Error, partest::LOG_TYPE_EXCEPTION, "Attempted to add log entry to test '"
						+ PARTEST_STRING_PARAM_TO_STRING(testFrame) + "' after it concluded. This indicates that a detached thread awoke post-teardown.\n" + PARTEST_STRING_PARAM_TO_STRING(message));
					break;
				case FailureMode::PostFrameworkTeardown:
					FrameworkContext::writeGlobalLog(LogLevel::Error, partest::LOG_TYPE_EXCEPTION, "Attempted to add log entry to test after the runner concluded. This indicates that a detached thread awoke post-teardown.\nLog message: " + PARTEST_STRING_PARAM_TO_STRING(message));
					break;
				}
			}
			catch(std::bad_alloc &)
			{
				badAllocOccurred = true;
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
				case FailureMode::PostTestTeardown:
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
			}

			// Use simple nonstandard exception to bubble up past user code that may catch std::bad_alloc.
			// This is a last-resort measure to ensure that the framework can report the failure, even if the user code has a catch-all for std::bad_alloc.
			if(badAllocOccurred)
				throw FrameworkAllocationFailure();
		}

	public:
		TestContext(TestFrame *currentFrame, void (*runTestFunc)(TestFrame *test))
			: m_currentFrame(currentFrame), m_runTestFunc(runTestFunc) { }

		template<PARTEST_INVOCABLE_WITH(Func, TestContext&)>
		void subtest(PARTEST_STRING_PARAM name, Func &&testFunc)
		{ subtest(TestInfo(name), TestFlags::defaultInherit(), testFunc); }

		template<PARTEST_INVOCABLE_WITH(Func, TestContext&)>
		void subtest(PARTEST_STRING_PARAM name, PARTEST_STRING_PARAM description, Func &&testFunc)
		{ subtest(TestInfo(name, description), TestFlags::defaultInherit(), testFunc); }

		template<PARTEST_INVOCABLE_WITH(Func, TestContext&)>
		void subtest(PARTEST_STRING_PARAM name, const TestFlags& flags, Func &&testFunc)
		{ subtest(TestInfo(name), flags, testFunc); }

		template<PARTEST_INVOCABLE_WITH(Func, TestContext&)>
		void subtest(PARTEST_STRING_PARAM name, PARTEST_STRING_PARAM description, const TestFlags& flags, Func &&testFunc)
		{ subtest(TestInfo(name, description), flags, testFunc); }

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

			try
			{
				newSubtest = m_currentFrame->addSubtest(flags, testInfo, testFunc);
			}
			catch(TestIntegrityFailure &)
			{
				// If the subtest cannot be added, log the error with the runner and rethrow the exception to indicate that this call was made after the test had finished running.
				// This should only happen if the subtest was added from a different thread after the test has completed.
				// TODO: Remember that if this is run from a raw thread and the user doesn't catch it, it will terminate the program. This will only function from framework-mananged thread wrappers. See partest::thread once it exists.
				recordSubtestFailure(m_currentFrame->metadata.name, testInfo.name, FailureMode::PostTestTeardown);
				throw;
			}
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
				m_currentFrame->commitAssertion(result);
			}
			catch(TestIntegrityFailure &)
			{
				// If the assertion cannot be recorded, log the error with the runner and rethrow the exception to indicate that this call was made after the test had finished running.
				// This should only happen if the assertion was made from a different thread after the test has completed.
				
				// TODO: Remember that if this is run from a raw thread and the user doesn't catch it, it will terminate the program.
				// This will only function from framework-mananged thread wrappers. See partest::thread once it exists.
				recordAssertionFailure(m_currentFrame->metadata.name, result, FailureMode::PostTestTeardown);
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
				m_currentFrame->recordLog(level, type, message);
			}
			catch(TestIntegrityFailure &)
			{
				// If the log entry cannot be recorded, log the error with the runner and rethrow the exception to indicate that this call was made after the test had finished running.
				// This should only happen if the log entry was made from a different thread after the test has completed.
				// TODO: Remember that if this is run from a raw thread and the user doesn't catch it, it will terminate the program.
				// This will only function from framework-mananged thread wrappers. See partest::thread once it exists.
				recordLogFailure(m_currentFrame->metadata.name, level, type, message, FailureMode::PostTestTeardown);
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
