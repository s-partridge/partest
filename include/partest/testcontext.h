#ifndef PARTEST_TEST_CONTEXT_H
#define PARTEST_TEST_CONTEXT_H

#include <partest/frameworkcontext.h>
#include <partest/testframe.h>

namespace partest
{
	class LifetimeGuard
	{
		bool m_alive;
	public:
		LifetimeGuard()
		{
			m_alive = FrameworkContext::requestAccessIfAlive();
		}

		~LifetimeGuard()
		{
			if(m_alive)
				FrameworkContext::releaseAccess();
		}

		bool isAlive() const noexcept
		{
			return m_alive;
		}
	};

	class TestContext
	{
		TestFrame *m_currentFrame;
		//Replace test suite ref with a function pointer for runTest, to avoid circular dependency. This will be a function pointer to TestBase::runTest
		void (*m_runTestFunc)(TestFrame *test);

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

			LifetimeGuard guard;
			if(!guard.isAlive())
			{
				// If the framework context is no longer alive, log the error with the runner and throw an exception to indicate that this call was made after the test had finished running.
				// This should only happen if the subtest was added from a different thread after the test has completed.

				return; // TODO: Add call to runner's log function here.
			}

			try
			{
				newSubtest = m_currentFrame->addSubtest(flags, testInfo, testFunc);
			}
			catch(TestIntegrityFailure &)
			{
				// If the subtest cannot be added, log the error with the runner and rethrow the exception to indicate that this call was made after the test had finished running.
				// This should only happen if the subtest was added from a different thread after the test has completed.
				// TODO: Add call to runner's log function here.
				// TODO: Remember that if this is run from a raw thread and the user doesn't catch it, it will terminate the program. This will only function from framework-mananged thread wrappers. See partest::thread once it exists.
				throw;
			}
			m_runTestFunc(newSubtest);
		}

		void commitAssertion(const AssertionResult &result)
		{
			LifetimeGuard guard;
			if(!guard.isAlive())
			{
				// If the framework context is no longer alive, log the error with the runner and throw an exception to indicate that this call was made after the test had finished running.
				// This should only happen if the subtest was added from a different thread after the test has completed.

				return; // TODO: Add call to runner's log function here.
			}

			try
			{
				m_currentFrame->commitAssertion(result);
			}
			catch(TestIntegrityFailure &)
			{
				// If the assertion cannot be recorded, log the error with the runner and rethrow the exception to indicate that this call was made after the test had finished running.
				// This should only happen if the assertion was made from a different thread after the test has completed.
				// TODO: Add call to runner's log function here.
				// TODO: Remember that if this is run from a raw thread and the user doesn't catch it, it will terminate the program. This will only function from framework-mananged thread wrappers. See partest::thread once it exists.
				throw;
			}
		}

		void recordLog(LogLevel level, PARTEST_STRING_PARAM type, PARTEST_STRING_PARAM message)
		{
			LifetimeGuard guard;
			if(!guard.isAlive())
			{
				// If the framework context is no longer alive, log the error with the runner and throw an exception to indicate that this call was made after the test had finished running.
				// This should only happen if the subtest was added from a different thread after the test has completed.

				return; // TODO: Add call to runner's log function here.
			}

			try
			{
				m_currentFrame->recordLog(level, type, message);
			}
			catch(TestIntegrityFailure &)
			{
				// If the log entry cannot be recorded, log the error with the runner and rethrow the exception to indicate that this call was made after the test had finished running.
				// This should only happen if the log entry was made from a different thread after the test has completed.
				// TODO: Add call to runner's log function here.
				// TODO: Remember that if this is run from a raw thread and the user doesn't catch it, it will terminate the program. This will only function from framework-mananged thread wrappers. See partest::thread once it exists.
				throw;
			}
		}

		void setTestFile(PARTEST_STRING_PARAM fileName)
		{
			LifetimeGuard guard;
			if(!guard.isAlive())
			{
				// If the framework context is no longer alive, log the error with the runner and throw an exception to indicate that this call was made after the test had finished running.
				// This should only happen if the subtest was added from a different thread after the test has completed.

				return; // TODO: Add call to runner's log function here.
			}
			m_currentFrame->setTestFile(fileName);
		}

		void setTestLine(unsigned line)
		{
			LifetimeGuard guard;
			if(!guard.isAlive())
			{
				// If the framework context is no longer alive, log the error with the runner and throw an exception to indicate that this call was made after the test had finished running.
				// This should only happen if the subtest was added from a different thread after the test has completed.

				return; // TODO: Add call to runner's log function here.
			}
			m_currentFrame->setTestLine(line);
		}
	};
}
#endif
