#ifndef PARTEST_SIMPLE_LOGGER_H
#define PARTEST_SIMPLE_LOGGER_H

#include <ostream>
#include <iostream>
#include <string>

#include <partest/eventreporter.h>

namespace partest
{
	class SimpleLogger : public EventReporterInterface
	{
		std::ostream &m_out;

		bool m_showPassedAssertions = false;
		LogLevel m_verbosity;

	public:
		explicit SimpleLogger(std::ostream &out = std::cout, bool showPassedAssertions = false, LogLevel verbosity = LogLevel::Error)
			: EventReporterInterface(), m_out(out), m_showPassedAssertions(showPassedAssertions), m_verbosity(verbosity) { }

		void onTestBegin(const Event &event, const BeginTestPayload &payload) override
		{
			m_out << "Begin: " << payload.testFrame.name() << std::endl;
		}

		// Called when a test ends
		void onTestEnd(const Event &event, const EndTestPayload &payload) override
		{
			m_out << "Ended: " << payload.testFrame.name() << "; " << payload.testFrame.result() << std::endl;
		}

		// Called when an assertion is made
		void onAssertion(const Event &event, const AssertionPayload &payload) override
		{
			if(m_showPassedAssertions || !payload.assertionResult.passed())
			{
				m_out << payload.assertionResult.message();
			}

		}
		// Called when a log entry is made
		void onLog(const Event &event, const LogPayload &payload) override
		{
			const LogEntry& logEntry = payload.logEntry;

			if(logEntry.level <= m_verbosity)
				m_out << logEntry.message;
		}

		// Called when a passthrough event is received
		void onPassthrough(const Event &event, const PassthroughPayload &payload) override
		{
			if(m_verbosity >= LogLevel::Info)
				m_out << payload.message;
		}
		// Called when a die event is received
		void onDie(const Event &event, const DiePayload &payload) override
		{
			if(m_verbosity >= LogLevel::Info)
				m_out << "Tests Completed" << std::endl;
		}
	};
}

#endif