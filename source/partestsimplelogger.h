#ifndef PARTEST_SIMPLE_REPORTER_H
#define PARTEST_SIMPLE_REPORTER_H

#include <ostream>
#include <iostream>
#include <string>

#include "partestreporter.h"

namespace partest
{
	class SimpleReporter : public EventReporterInterface
	{
		std::ostream &m_out;

		bool m_showPassedAssertions = false;
		LogLevel m_verbosity;

	public:
		explicit SimpleReporter(std::ostream &out = std::cout, bool showPassedAssertions = false, LogLevel verbosity = LogLevel::Error)
			: EventReporterInterface(), m_out(out), m_showPassedAssertions(showPassedAssertions), m_verbosity(verbosity) { }

		void onTestBegin(const EventBeginTest &event) override
		{
			m_out << "Begin: " << event.getTestName() << std::endl;
		}

		// Called when a test ends
		void onTestEnd(const EventEndTest &event) override
		{
			m_out << "End: " << event.getTestName() << std::endl;
		}

		// Called when an assertion is made
		void onAssertion(const EventAssertion &event) override
		{
			const AssertionResult &result = event.getAssertionResult();
			
			if(m_showPassedAssertions || !result.passed)
			{
				m_out << result.message;
			}

		}
		// Called when a log entry is made
		void onLog(const EventLog &event) override
		{
			const LogEntry& logEntry = event.getLogEntry();

			if(logEntry.level <= m_verbosity)
				m_out << logEntry.message;
		}

		// Called when a passthrough event is received
		void onPassthrough(const EventPassthrough &event) override
		{
			if(m_verbosity >= LogLevel::Info)
				event.getMessage();
		}
		// Called when a die event is received
		void onDie(const EventDie &event) override
		{
			if(m_verbosity >= LogLevel::Info)
				m_out << "Tests Completed" << std::endl;
		}
	};
}

#endif