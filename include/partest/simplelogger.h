#ifndef PARTEST_SIMPLE_LOGGER_H
#define PARTEST_SIMPLE_LOGGER_H

#include <ostream>
#include <iostream>
#include <string>

#include <partest/fileops.h>
#include <partest/eventreporter.h>
#include <partest/assertionparser.h>

namespace partest
{
	namespace simple
	{
		inline void appendFileLine(std::ostream &s, const partest::AssertionResult &assertion)
		{
			s << "\n at: " << getFilename(assertion.file) << ":" << assertion.line << std::endl;
		}
		
		// Generic assertion parser functions
		inline std::string parseAssertTrue(const partest::AssertionResult &assertion)
		{
			std::ostringstream oss;

			if(assertion.passed())
			{
				oss << "PASSED: " << assertion.getCondition();
			}
			else
			{
				oss << "FAILED: " << assertion.getMetadata(MetaKeys::ExprA) 
					<< " was: " << assertion.getMetadata(MetaKeys::Actual)
					<< "; expected: " << assertion.getMetadata(MetaKeys::Expected);
			}

			appendFileLine(oss, assertion);

			return oss.str();
		}
		// Identical logic to AssertTrue
		inline std::string parseAssertFalse(const partest::AssertionResult &assertion) { return parseAssertTrue(assertion); }

		inline std::string parseAssertEqual(const partest::AssertionResult &assertion)
		{
			std::ostringstream oss;

			if(assertion.passed())
			{
				oss << "PASSED: " << assertion.getCondition();
			}
			else
			{
				oss << "FAILED: " << assertion.getMetadata(MetaKeys::ExprA) 
					<< " was: " << assertion.getMetadata(MetaKeys::Actual)
					<< "\n expected: " << assertion.getMetadata(MetaKeys::ExprB)
					<< " as: " << assertion.getMetadata(MetaKeys::Expected);
			}

			appendFileLine(oss, assertion);

			return oss.str();
		}
		inline std::string parseAssertNotEqual(const partest::AssertionResult &assertion) { return parseAssertEqual(assertion); }

		inline std::string parseAssertApproxEqual(const partest::AssertionResult &assertion) { return ""; }
		inline std::string parseAssertApproxNotEqual(const partest::AssertionResult &assertion) { return ""; }

		inline std::string parseAssertGreater(const partest::AssertionResult &assertion) { return ""; }
		inline std::string parseAssertGreaterEqual(const partest::AssertionResult &assertion) { return ""; }
		inline std::string parseAssertLess(const partest::AssertionResult &assertion) { return ""; }
		inline std::string parseAssertLessEqual(const partest::AssertionResult &assertion) { return ""; }

		inline AssertionParser makeAssertionParser()
		{
			AssertionParser parser;
			parser.addFunction(ASSERT_TRUE_STR, parseAssertTrue);
			parser.addFunction(ASSERT_FALSE_STR, parseAssertFalse);
			parser.addFunction(ASSERT_EQUAL_STR, parseAssertEqual);
			parser.addFunction(ASSERT_NOT_EQUAL_STR, parseAssertNotEqual);
			parser.addFunction(ASSERT_APPROX_EQUAL_STR, parseAssertApproxEqual);
			parser.addFunction(ASSERT_APPROX_NOT_EQUAL_STR, parseAssertApproxNotEqual);
			parser.addFunction(ASSERT_GREATER_STR, parseAssertGreater);
			parser.addFunction(ASSERT_GREATER_EQUAL_STR, parseAssertGreaterEqual);
			parser.addFunction(ASSERT_LESS_STR, parseAssertLess);
			parser.addFunction(ASSERT_LESS_EQUAL_STR, parseAssertLessEqual);
			return parser;
		}
	}

	class SimpleLogger : public EventReporterInterface
	{
		std::ostream &m_out;

		bool m_showPassedAssertions = false;
		LogLevel m_verbosity;
		AssertionParser m_assertionParser;

	public:
		explicit SimpleLogger(std::ostream &out = std::cout, bool showPassedAssertions = false, LogLevel verbosity = LogLevel::Error)
			: EventReporterInterface(), m_out(out), m_showPassedAssertions(showPassedAssertions), m_verbosity(verbosity), m_assertionParser(simple::makeAssertionParser())
		{ }

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
				m_out << m_assertionParser.parseAssertion(payload.assertionResult) << std::endl;
			}

		}
		// Called when a log entry is made
		void onLog(const Event &event, const LogPayload &payload) override
		{
			const LogEntry& logEntry = payload.logEntry;

			if(logEntry.level <= m_verbosity)
				m_out << logEntry.level << ' ' << logEntry.message;
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