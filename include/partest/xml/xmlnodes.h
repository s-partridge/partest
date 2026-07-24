#ifndef PARTEST_XML_NODES_H
#define PARTEST_XML_NODES_H

#include <string>
#include <iomanip>
#include <chrono>
#include <vector>
#include <memory>

#include <partest/common.h>

namespace partest
{
	namespace xml
	{
		constexpr const char *JUNIT_TESTSUITES = "testsuites";
		constexpr const char *JUNIT_TESTSUITE = "testsuite";
		constexpr const char *JUNIT_TESTCASE = "testcase";
		constexpr const char *JUNIT_PROPERTIES = "properties";
		constexpr const char *JUNIT_PROPERTY = "property";
		constexpr const char *JUNIT_SYSTEM_OUT = "system-out";
		constexpr const char *JUNIT_SYSTEM_ERR = "system-err";
		constexpr const char *JUNIT_SKIPPED = "skipped";
		constexpr const char *JUNIT_FAILURE = "failure";
		constexpr const char *JUNIT_ERROR = "error";

		// Standard datetime expected by JUnit
		std::string toIso8601(std::chrono::system_clock::time_point timePoint)
		{
			time_t time = std::chrono::system_clock::to_time_t(timePoint);
			std::tm calendarTime = *std::gmtime(&time);
			std::ostringstream out;
			out << std::put_time(&calendarTime, "%Y-%m-%dT%H:%M:%S");
			return out.str();
		}

		// Root type for any XML node tree.
		// Any XML node requires a tag and the ability to nest further nodes
		struct JUnitXMLNode
		{
		protected:
			std::vector<std::unique_ptr<JUnitXMLNode>> children;

			void walkChildren(std::ostream &out) const
			{
				if(children.empty())
					return;

				for(size_t i = 0; i < children.size(); ++i)
				{
					out << *children[i];
				}
			}

			virtual std::string openTag() const { return '<' + nodeTag + '>'; }
			virtual void bodyText(std::ostream &out) const { walkChildren(out); }
			virtual std::string closeTag() const { return "</" + nodeTag + '>'; }

		public:
			std::string nodeTag;

			JUnitXMLNode(PARTEST_STRING_PARAM nodeTag) : nodeTag(nodeTag) {}
			virtual ~JUnitXMLNode() = default;

			void addChild(std::unique_ptr<JUnitXMLNode> child) { children.push_back(std::move(child)); }

			friend std::ostream &operator<<(std::ostream &out, const JUnitXMLNode &rhs);
		};

		std::ostream &operator<<(std::ostream &out, const JUnitXMLNode &rhs)
		{
			out << rhs.openTag();
			rhs.bodyText(out);
			out << rhs.closeTag();
			return out;
		}

		// Root node for a JUnit XML file, contains metrics for the entire test suite
		struct TestSuitesNode : public JUnitXMLNode
		{
			std::string name = "";		// Name of the test suite (e.g. class name or folder name)
			unsigned tests = 0;			// Total number of tests
			unsigned failures = 0;		// Total number of failed tests
			unsigned errors = 0;		// Total number of errored tests
			unsigned skipped = 0;		// Total number of skipped tests
			unsigned assertions = 0;	// Total number of assertions for all tests

			// Aggregated time of all tests in this file in seconds
			std::chrono::steady_clock::duration time = std::chrono::steady_clock::duration(0);
			// Date and time of when the test suite was executed (in ISO 8601 format)
			std::chrono::system_clock::time_point timestamp;

			TestSuitesNode(PARTEST_STRING_PARAM nodeTag = JUNIT_TESTSUITES) : JUnitXMLNode(nodeTag) {}

			std::string openTag() const override
			{
				std::chrono::duration<double> seconds = time;

				std::ostringstream out;
				out << '<' << nodeTag
					<< " name=\"" << name << "\""
					<< " tests=\"" << tests << "\""
					<< " failures=\"" << failures << "\""
					<< " assertions=\"" << assertions << "\""
					<< " time=\"" << seconds.count() << "\""
					<< " timestamp=\"" << toIso8601(timestamp) << "\""
					<< ">";

				return out.str();
			}
		};

		// Root node for an individual suite within a JUnit XML file, representing (generally) one test file
		// TestSuite nodes can contain other TestSuite nodes
		struct TestSuiteNode : public TestSuitesNode
		{
			std::string file = "";      // Source code file for this test suite

			TestSuiteNode(PARTEST_STRING_PARAM nodeTag = JUNIT_TESTSUITE) : TestSuitesNode(nodeTag) {}
		};

		struct TestCaseNode : public JUnitXMLNode
		{
			std::string name = "";		// The name of this test case, often the function name
			std::string classname = "";	// The name of the parent class/folder, often the same as the suite's name
			unsigned assertions = 0;	// Number of assertions checked during test case execution
        
			// Execution time of the test in seconds
			std::chrono::steady_clock::duration time = std::chrono::steady_clock::duration(0);

			std::string file = "";		// Source code file of this test case
			int line = 1;					// Source code line number of the start of this test case

			TestCaseNode(PARTEST_STRING_PARAM nodeTag = JUNIT_TESTCASE) : JUnitXMLNode(nodeTag) {}
		};

		// Properties node, optional, containing individual property nodes
		struct PropertiesNode : public JUnitXMLNode
		{
			PropertiesNode(PARTEST_STRING_PARAM nodeTag = JUNIT_PROPERTIES) : JUnitXMLNode(nodeTag) {}
		};

		// Property node, may contain either a value in the XML, or a text body
		struct PropertyNode : public PropertiesNode
		{
			std::string name = "";
			std::string value = "";
			bool valueAsBody = false;

			PropertyNode(PARTEST_STRING_PARAM nodeTag = JUNIT_PROPERTY) : PropertiesNode(nodeTag) {}
		};

		// Suite or Test level log from stdout
		struct SystemOutNode : public JUnitXMLNode
		{
			std::string body = "";
			SystemOutNode(PARTEST_STRING_PARAM nodeTag = JUNIT_SYSTEM_OUT) : JUnitXMLNode(nodeTag) {}
		};

		// Suite or Test level log from stderr
		struct SystemErrNode : public JUnitXMLNode
		{
			std::string body = "";
			SystemErrNode(PARTEST_STRING_PARAM nodeTag = JUNIT_SYSTEM_ERR) : JUnitXMLNode(nodeTag) {}
		};

		// Used to report that a test was skipped
		struct SkippedNode : public JUnitXMLNode
		{
			std::string message = "";

			SkippedNode(PARTEST_STRING_PARAM nodeTag = JUNIT_SKIPPED) : JUnitXMLNode(nodeTag) {}
		};

		// Used to report the results of a failed test, usually from a failed assertion
		struct FailureNode : public JUnitXMLNode
		{
			std::string message = "";
			std::string type = "";
			std::string body = "";

			FailureNode(PARTEST_STRING_PARAM nodeTag = JUNIT_FAILURE) : JUnitXMLNode(nodeTag) {}
		};

		// Same as failure node, but represents an unexpected error during test execution
		struct ErrorNode : public JUnitXMLNode
		{
			std::string message = "";
			std::string type = "";
			std::string body = "";

			ErrorNode(PARTEST_STRING_PARAM nodeTag = JUNIT_ERROR) : JUnitXMLNode(nodeTag) {}
		};
	}
}

#endif