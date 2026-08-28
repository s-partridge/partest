#ifndef PARTEST_JUNIT_LOGGER_H
#define PARTEST_JUNIT_LOGGER_H

#include <vector>
#include <fstream>

#include <partest/eventreporter.h>
#include <partest/testframe.h>
#include <partest/runner.h>
#include <partest/assertionparser.h>
#include <partest/xml/xmlnodes.h>

namespace partest
{
	class JUnitLogger : public EventReporterInterface, public TestFrameReaderInterface
	{
		static bool shouldCreateLocalTestCaseNode(const TestFrame *test)
		{
			return (test->assertionCount() > 0 || test->subtestCount() == 0 || test->getStatus() == TestStatus::Aborted);
		}

		static bool shouldCreateExpectedFailureNode(const TestFrame *test)
		{
			return test->getEffectiveResult() == TestResult::ExpectedFailure;
		}

		static bool shouldCreateAbortNode(const TestFrame *testFrame)
		{
			return testFrame->getStatus() == TestStatus::Aborted;
		}

		static bool shouldCreateFailureNode(const TestFrame *testFrame)
		{
			return testFrame->hasFailures();
		}

		static bool isTestAStub(const TestFrame *testFrame)
		{
			return testFrame->assertionCount() == 0 && testFrame->subtestCount() == 0;
		}

		static bool didPassUnexpectedly(const TestFrame *testFrame)
		{
			return testFrame->getEffectiveResult() == TestResult::UnexpectedPass;
		}

		std::unique_ptr<xml::TestSuitesNode> m_root;
		std::string m_reportPath;
		AssertionParser m_assertionParser;

		void sumTestSuitesNode(const TestFrameView *testFrame)
		{
			if(testFrame->parentId() == NO_TEST_ID)
			{
				m_root->tests += testFrame->subtestCount();
				m_root->assertions += testFrame->assertionCount();
				m_root->failures += testFrame->subtestFailureCount();
				//TODO: Expose error count as aggeregate of failed tests with abort status
				//m_root->errors += testFrame->subtestErrorCount();
				m_root->skipped += testFrame->testSkippedCount();
				m_root->time += testFrame->duration();
			}
		}

		void buildTestSuiteNode(const TestFrame *testFrame, xml::TestSuiteNode *node)
		{
			node->name = testFrame->metadata.name;
			node->file = testFrame->metadata.file;
			node->tests = testFrame->subtestCount();
			node->assertions = testFrame->assertionCount();
			node->failures = testFrame->getTestFailureCount();
			node->time = testFrame->endTime() - testFrame->startTime();
			node->timestamp = testFrame->timestamp();
 		}

		void buildTestCaseNode(const TestFrame *testFrame, xml::TestCaseNode *node, PARTEST_STRING_PARAM parentTestName)
		{
			node->name = testFrame->metadata.name;
			node->file = testFrame->metadata.file;
			node->line = testFrame->metadata.line;
			node->classname = parentTestName;
			node->assertions = testFrame->assertionCount();
			node->time = testFrame->endTime() - testFrame->startTime();
			if(testFrame->getStatus() == TestStatus::Skipped)
			{
				xml::SkippedNode *skippedNode = static_cast<xml::SkippedNode *>(node->addChild(partest::make_unique<xml::SkippedNode>())); // NOLINT(*-pro-type-static-cast-downcast)
				buildSkippedNode(testFrame, skippedNode);
			}
			else if(shouldCreateAbortNode(testFrame))
			{
				xml::ErrorNode *errorNode = static_cast<xml::ErrorNode *>(node->addChild(partest::make_unique<xml::ErrorNode>())); // NOLINT(*-pro-type-static-cast-downcast)
				buildAbortedNode(testFrame, errorNode);
			}
			else if(shouldCreateFailureNode(testFrame))
			{
				xml::FailureNode *failureNode = static_cast<xml::FailureNode *>(node->addChild(partest::make_unique<xml::FailureNode>())); // NOLINT(*-pro-type-static-cast-downcast)
				buildFailureNode(testFrame, failureNode);
			}
			else if(shouldCreateExpectedFailureNode(testFrame))
			{
				// This isn't exactly a failure. Add a log node to indicate that the test failed by design
				// So no failure node. Log node instead.
				xml::SystemOutNode *logNode = static_cast<xml::SystemOutNode *>(node->addChild(partest::make_unique<xml::SystemOutNode>())); // NOLINT(*-pro-type-static-cast-downcast)
				buildExpectedFailureNode(testFrame, logNode);
			}
		}

		void buildSkippedNode(const TestFrame *testFrame, xml::SkippedNode *node)
		{
			node->message = "Test skipped";
		}

		void buildAbortedNode(const TestFrame *testFrame, xml::ErrorNode *node)
		{
			node->body = "Test was aborted due to an unexpected error or failure.";
			node->message = "Test aborted";
			node->type = "Aborted";

			const std::deque<LogEntry> &logs = testFrame->getLogs();
			// Find any logs with level Error or type EXCEPTION and add them to the error node body.
			for(auto log : logs)
			{
				if(log.level == LogLevel::Error || log.type == LOG_TYPE_EXCEPTION)
				{
					node->body += '\n' + log.message;
				}
			}
		}

		void buildExpectedFailureNode(const TestFrame *testFrame, xml::SystemOutNode *node)
		{
			std::string nodeBody;

			// Get the first assertion that failed and add it to the node body.
			for(TestFrame::AssertionConstIter assertion = testFrame->assertionsBegin(); assertion != testFrame->assertionsEnd(); ++assertion)
			{
				if(!assertion->passed())
				{
					nodeBody = m_assertionParser.parseAssertion(*assertion);
					break;
				}
			}

			if(nodeBody.empty())
			{
				node->body = "Test ran with ExpectFailure: Condition satisfied by failure in subtest";
			}
			else
			{
				node->body = "Test ran with ExpectFailure: Satisfied by assertion failure\n" + nodeBody;
			}
		}

		void buildFailureNode(const TestFrame *testFrame, xml::FailureNode *node)
		{
			// Record the first failed assertion.
			for(TestFrame::AssertionConstIter assertion = testFrame->assertionsBegin(); assertion != testFrame->assertionsEnd(); ++assertion)
			{
				if(!assertion->passed())
				{
					node->message = "Assertion Failed: (" + assertion->getCondition() + ')';
					node->type = assertion->assertType();
					node->body = m_assertionParser.parseAssertion(*assertion);
					break;
				}
			}

			// Determine failure mode. Did the test pass unexpectedly?
			if(testFrame->getEffectiveResult() == TestResult::UnexpectedPass)
			{
				node->message = "Test passed unexpectedly";
				node->type = "UnexpectedPass";
				if(!isTestAStub(testFrame))
					node->body = "Test ran with ExpectFailure: This test was expected to fail, but it passed. Check the test conditions and update the test flags if necessary.";
				else
					node->body = "Test ran with ExpectFailure: This test was expected to fail, but it passed. Check the test conditions and update the test flags if necessary. Also check that this test is not an empty stub.";
			}
			// Must have been a subtest failure
			else if(node->body.empty())
			{
				node->message = "Subtest failed";
				node->type = "SubtestFailure";
				node->body = "One or more subtests failed.";
			}
		}

		void readSubtree(const TestFrame *testFrame, xml::JUnitXMLNode *node, PARTEST_STRING_PARAM parentTestName)
		{
			TestFrame::TestFrameConstIter subtest = testFrame->subtestsBegin();

			while(subtest != testFrame->subtestsEnd())
			{
				const TestFrame *subtestFrame = *subtest;
				// Recurse and create nested TestSuite nodes if subtests exist.
				if(subtestFrame->hasSubtests())
				{
					// Create a node for the test suite
					xml::TestSuiteNode *suiteNode = static_cast<xml::TestSuiteNode *>(node->addChild(partest::make_unique<xml::TestSuiteNode>())); // NOLINT(*-pro-type-static-cast-downcast)
					buildTestSuiteNode(subtestFrame, suiteNode);

					// Populate it with a test case for its local results
					if(shouldCreateLocalTestCaseNode(subtestFrame))
					{
						xml::TestCaseNode *testNode = static_cast<xml::TestCaseNode *>(suiteNode->addChild(partest::make_unique<xml::TestCaseNode>())); // NOLINT(*-pro-type-static-cast-downcast)
						buildTestCaseNode(subtestFrame, testNode, parentTestName);
					}
					else if(shouldCreateExpectedFailureNode(subtestFrame))
					{
						// This isn't exactly a failure. Add a log node to indicate that the test failed by design
						// So no failure node. Log node instead.
						xml::SystemOutNode *logNode = static_cast<xml::SystemOutNode *>(suiteNode->addChild(partest::make_unique<xml::SystemOutNode>())); // NOLINT(*-pro-type-static-cast-downcast)
						buildExpectedFailureNode(subtestFrame, logNode);
					}
					// Add subtests as nested TestSuite nodes
					readSubtree(subtestFrame, suiteNode, suiteNode->name);
				}
				// Otherwise create a TestCase
				else
				{
					xml::TestCaseNode *testNode = static_cast<xml::TestCaseNode *>(node->addChild(partest::make_unique<xml::TestCaseNode>())); // NOLINT(*-pro-type-static-cast-downcast)
					buildTestCaseNode(subtestFrame, testNode, parentTestName);
				}
				++subtest;
			}
		}

	public:
		explicit JUnitLogger(PARTEST_STRING_PARAM reportPath)
			: EventReporterInterface(), TestFrameReaderInterface(),
			  m_root(make_unique<xml::TestSuitesNode>()),
			  m_reportPath(reportPath), m_assertionParser(simple::makeAssertionParser())
		{
			m_root->timestamp = std::chrono::system_clock::now();
		}

		// EventReporter functions
		void onTestBegin(const Event &event, const BeginTestPayload &payload) override {}

		void onTestEnd(const Event &event, const EndTestPayload &payload) override 
		{
			// Aggregate statistics from top-level tests only.
			sumTestSuitesNode(&payload.testFrame);
		}

		void onAssertion(const Event &event, const AssertionPayload &payload) override {}
		void onLog(const Event &event, const LogPayload &payload) override {}
		void onPassthrough(const Event &event, const PassthroughPayload &payload) override {}

		// On suite end, aggregate logs for test suites.
		void onDie(const Event &event, const DiePayload &payload) override
		{
			TestRunner::getInstance().readAllTests(this);
			writeToFile();
		}

		// Called for each test to be read
		void readTree(const TestFrame &root) override
		{
			// addChild always returns a raw pointer to the node it was just passed.
			xml::TestSuiteNode *node = static_cast<xml::TestSuiteNode *>(m_root->addChild(partest::make_unique<xml::TestSuiteNode>())); // NOLINT(*-pro-type-static-cast-downcast)
			buildTestSuiteNode(&root, node);
			readSubtree(&root, node, node->name);

			std::cout << "Reading test frame: " << root.id() << std::endl;
		}

		void writeToFile() const
		{
			std::ofstream xmlFile(m_reportPath);

			if(!xmlFile.is_open())
			{
				std::cerr << "ERROR: Could not open path for JUnit report: " << m_reportPath << std::endl;
				return;
			}

			// XML Header
			xmlFile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
			// File body
			xmlFile << *m_root;

			if (xmlFile.fail())
			{
				std::cerr << "ERROR: An error occurred while attempting to write to JUnit report: " << m_reportPath << std::endl;
			}
		}
	};
}

#endif