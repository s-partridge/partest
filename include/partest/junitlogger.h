#ifndef PARTEST_JUNIT_LOGGER_H
#define PARTEST_JUNIT_LOGGER_H

#include <vector>
#include <fstream>
#include <exception>

#include <partest/eventreporter.h>
#include <partest/testframe.h>
#include <partest/runner.h>
#include <partest/xml/xmlnodes.h>

namespace partest
{
	class JUnitLogger : public EventReporterInterface, public TestFrameReaderInterface
	{
		std::unique_ptr<xml::TestSuitesNode> m_root;
		std::string m_reportPath;

	public:
		JUnitLogger(PARTEST_STRING_PARAM reportPath)
			: EventReporterInterface(), TestFrameReaderInterface(),
			  m_root(make_unique<xml::TestSuitesNode>()),
			  m_reportPath(reportPath)
		{ }

		// EventReporter functions
		void onTestBegin(const Event &event, const BeginTestPayload &payload) override {}

		void onTestEnd(const Event &event, const EndTestPayload &payload) override 
		{
			// Aggregate statistics from top-level tests only.
			const TestFrameView &testFrame = payload.testFrame;
			if(testFrame.parentId() == NO_TEST_ID)
			{
				m_root->tests += testFrame.subtestCount();
				m_root->assertions += testFrame.assertionCount();
				m_root->failures += testFrame.subtestFailureCount();
				//TODO: Expose error count as aggeregate of failed tests with abort status
				//m_root->errors += testFrame.subtestErrorCount();
				//TODO: Expose skipped count
				//m_root->skipped += testFrame.skippedCount();
				m_root->time += testFrame.duration();
			}
		}

		void onAssertion(const Event &event, const AssertionPayload &payload) override {}
		void onLog(const Event &event, const LogPayload &payload) override {}
		void onPassthrough(const Event &event, const PassthroughPayload &payload) override {}

		// On suite end, aggregate logs for test suites.
		void onDie(const Event &event, const DiePayload &payload) override
		{
			PartestRunner::getInstance().readAllTests(this);
			m_root->timestamp = event.getTimestamp();
			writeToFile();
		}

		// Called for each test to be read
		void readTree(const TestFrame &root) override
		{
			std::cout << "Reading test frame: " << root.id() << std::endl;
		}

		void writeToFile()
		{
			std::ofstream xmlFile(m_reportPath);

			if(!xmlFile.is_open())
			{
				std::cerr << "ERROR: Could not open path for JUnit report: " << m_reportPath << std::endl;
				return;
			}

			xmlFile << *m_root;

			if (xmlFile.fail())
			{
				std::cerr << "ERROR: An error occurred while attempting to write to JUnit report: " << m_reportPath << std::endl;
			}
		}
	};
}

#endif