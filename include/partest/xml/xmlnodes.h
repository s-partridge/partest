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
		// TODO: Move this to a common location
		inline std::string toIso8601(std::chrono::system_clock::time_point timePoint)
		{
			time_t time = std::chrono::system_clock::to_time_t(timePoint);
			// TODO: replace gmtime call with centralized alternative that uses gmtime_s/_r as available.
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

			explicit JUnitXMLNode(PARTEST_STRING_PARAM nodeTag) : nodeTag(nodeTag) {}
			virtual ~JUnitXMLNode() = default;

			/**
			* Add a new child node to this node. Return non-owning pointer to the new child.
			*/
			JUnitXMLNode *addChild(std::unique_ptr<JUnitXMLNode> child)
			{
				children.push_back(std::move(child));
				return children.back().get();
			}

			friend std::ostream &operator<<(std::ostream &out, const JUnitXMLNode &rhs);
		};

		inline std::ostream &operator<<(std::ostream &out, const JUnitXMLNode &rhs)
		{
			out << rhs.openTag();
			rhs.bodyText(out);
			out << rhs.closeTag();
			return out;
		}

		// Root node for a JUnit XML file, contains metrics for the entire test suite
		struct TestSuitesNode : public JUnitXMLNode
		{
		protected:
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

		public:
			std::string name;			// Name of the test suite (e.g. class name or folder name)
			size_t tests = 0;			// Total number of tests
			size_t failures = 0;		// Total number of failed tests
			size_t errors = 0;			// Total number of errored tests
			size_t skipped = 0;			// Total number of skipped tests
			size_t assertions = 0;		// Total number of assertions for all tests

			// Aggregated time of all tests in this file in seconds
			std::chrono::steady_clock::duration time = std::chrono::steady_clock::duration(0);
			// Date and time of when the test suite was executed (in ISO 8601 format)
			std::chrono::system_clock::time_point timestamp;

			explicit TestSuitesNode(PARTEST_STRING_PARAM nodeTag = JUNIT_TESTSUITES) : JUnitXMLNode(nodeTag) {}
		};

		// Root node for an individual suite within a JUnit XML file, representing (generally) one test file
		// TestSuite nodes can contain other TestSuite nodes
		struct TestSuiteNode : public TestSuitesNode
		{
		protected:
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
					<< " file=\"" << file << "\""
					<< ">";

				return out.str();
			}

		public:

			std::string file;      // Source code file for this test suite

			explicit TestSuiteNode(PARTEST_STRING_PARAM nodeTag = JUNIT_TESTSUITE) : TestSuitesNode(nodeTag) {}
		};

		struct TestCaseNode : public JUnitXMLNode
		{
		protected:
			std::string openTag() const override
			{
				std::chrono::duration<double> seconds = time;

				std::ostringstream out;
				out << '<' << nodeTag
					<< " name=\"" << name << "\""
					<< " classname=\"" << classname << "\""
					<< " assertions=\"" << assertions << "\""
					<< " time=\"" << seconds.count() << "\""
					<< " file=\"" << file << "\""
					<< " line=\"" << line << "\""
					<< ">";

				return out.str();
			}

		public:
			std::string name;		// The name of this test case, often the function name
			std::string classname;	// The name of the parent class/folder, often the same as the suite's name
			size_t assertions = 0;	// Number of assertions checked during test case execution
        
			// Execution time of the test in seconds
			std::chrono::steady_clock::duration time = std::chrono::steady_clock::duration(0);

			std::string file;		// Source code file of this test case
			int line = 1;					// Source code line number of the start of this test case

			explicit TestCaseNode(PARTEST_STRING_PARAM nodeTag = JUNIT_TESTCASE) : JUnitXMLNode(nodeTag) {}
		};

		// Properties node, optional, containing individual property nodes
		struct PropertiesNode : public JUnitXMLNode
		{
			explicit PropertiesNode(PARTEST_STRING_PARAM nodeTag = JUNIT_PROPERTIES) : JUnitXMLNode(nodeTag) {}
		};

		// Property node, may contain either a value in the XML, or a text body
		struct PropertyNode : public PropertiesNode
		{
		protected:
			std::string openTag() const override
			{
				if(valueAsBody)
					return '<' + nodeTag + '>';
				// TODO: Escape value when used as part of tag
				return '<' + nodeTag + " value=\"" + value + "\">";
			}

			void bodyText(std::ostream &out) const override
			{
				if(valueAsBody)
					out << value;
			}

		public:
			std::string name;
			std::string value;
			bool valueAsBody = false;

			explicit PropertyNode(PARTEST_STRING_PARAM nodeTag = JUNIT_PROPERTY) : PropertiesNode(nodeTag) {}
		};

		// Suite or Test level log from stdout
		struct SystemOutNode : public JUnitXMLNode
		{
		protected:
			void bodyText(std::ostream &out) const override
			{
				out << body;
			}

		public:
			std::string body;

			explicit SystemOutNode(PARTEST_STRING_PARAM nodeTag = JUNIT_SYSTEM_OUT) : JUnitXMLNode(nodeTag) {}
		};

		// Suite or Test level log from stderr
		struct SystemErrNode : public JUnitXMLNode
		{
		protected:
			void bodyText(std::ostream &out) const override
			{
				out << body;
			}

		public:
			std::string body;

			explicit SystemErrNode(PARTEST_STRING_PARAM nodeTag = JUNIT_SYSTEM_ERR) : JUnitXMLNode(nodeTag) {}
		};

		// Used to report that a test was skipped
		struct SkippedNode : public JUnitXMLNode
		{
		protected:
			std::string openTag() const override
			{
				// TODO: Escape message
				return '<' + nodeTag + " message=\"" + message + "\" />";
			}

			std::string closeTag() const override
			{
				return "";
			}

		public:
			std::string message;

			explicit SkippedNode(PARTEST_STRING_PARAM nodeTag = JUNIT_SKIPPED) : JUnitXMLNode(nodeTag) {}
		};

		// Used to report the results of a failed test, usually from a failed assertion
		struct FailureNode : public JUnitXMLNode
		{
		protected:
			std::string openTag() const override
			{
				// TODO: Escape message
				return '<' + nodeTag
					+ " message=\"" + message
					+ "\" type=\"" + type 
					+ "\">";
			}

			void bodyText(std::ostream &out) const override
			{
				out << body;
			}

		public:
			std::string message;
			std::string type;
			std::string body;

			explicit FailureNode(PARTEST_STRING_PARAM nodeTag = JUNIT_FAILURE) : JUnitXMLNode(nodeTag) {}
		};

		// Same as failure node, but represents an unexpected error during test execution
		struct ErrorNode : public JUnitXMLNode
		{
		protected:
			std::string openTag() const override
			{
				// TODO: Escape message
				return '<' + nodeTag
					+ " message=\"" + message
					+ "\" type=\"" + type 
					+ "\">";
			}

			void bodyText(std::ostream &out) const override
			{
				out << body;
			}

		public:
			std::string message;
			std::string type;
			std::string body;

			explicit ErrorNode(PARTEST_STRING_PARAM nodeTag = JUNIT_ERROR) : JUnitXMLNode(nodeTag) {}
		};
	}
}

#endif