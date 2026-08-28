#ifndef PARTEST_XML_NODES_H
#define PARTEST_XML_NODES_H

#include <string>
#include <iomanip>
#include <vector>
#include <memory>

#include <partest/common.h>
#include <partest/stringops.h>

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

		static std::string sanitizeText(PARTEST_STRING_PARAM text)
		{
			return sanitizeForXML(text, XMLEscapeTable::Mode::Plaintext);
		}
			
		static std::string sanitizeAttrib(PARTEST_STRING_PARAM attrib)
		{
			return sanitizeForXML(attrib, XMLEscapeTable::Mode::DoubleQuoted);
		}

		// Root type for any XML node tree.
		// Any XML node requires a tag
		struct XMLNode
		{
		protected:
			unsigned depth = 0;

			virtual std::string openTag() const { return makeIndent() + '<' + nodeTag + '>'; }
			virtual std::string bodyText() const { return ""; }
			virtual std::string closeTag() const { return makeIndent() + "</" + nodeTag + '>'; }

			virtual std::string makeWholeTag() const
			{
				std::string wholeTag = openTag() + "\n";
				wholeTag += bodyText();
				wholeTag += closeTag() + "\n";
				return wholeTag;
			}

			virtual void updateDepth(unsigned newDepth)
			{
				depth = newDepth;
			}

			std::string makeIndent() const
			{
				return std::string(depth * 4, ' ');
			}

		public:
			std::string nodeTag;

			explicit XMLNode(PARTEST_STRING_PARAM nodeTag) : nodeTag(sanitizeText(nodeTag)) {}
			virtual ~XMLNode() = default;
		
			friend struct XMLContainerNode;
			friend std::ostream &operator<<(std::ostream &out, const XMLNode &rhs);
		};

		inline std::ostream &operator<<(std::ostream &out, const XMLNode &rhs)
		{
			out << rhs.makeWholeTag();
			
			return out;
		}

		struct XMLSelfClosingNode : public XMLNode
		{
		protected:
			std::string openTag() const override { return makeIndent() + '<' + nodeTag + "/>"; }
			std::string bodyText() const final { return ""; }
			std::string closeTag() const final { return ""; }

			std::string makeWholeTag() const final { return openTag() + '\n'; }

		public:
			explicit XMLSelfClosingNode(PARTEST_STRING_PARAM nodeTag) : XMLNode(nodeTag) {}
			virtual ~XMLSelfClosingNode() = default;
		};

		struct XMLContainerNode : public XMLNode
		{
		protected:
			std::vector<std::unique_ptr<XMLNode>> children;

			std::string walkChildren() const
			{
				if(children.empty())
					return "";

				std::ostringstream out;

				for(size_t i = 0; i < children.size(); ++i)
				{
					out << *children[i];
				}
				
				return out.str();
			}

			virtual std::string bodyText(void) const { return walkChildren(); }

			void updateDepth(unsigned newDepth) override
			{
				depth = newDepth;
				for(auto &child : children)
				{
					child->updateDepth(newDepth + 1);
				}
			}

		public:
			explicit XMLContainerNode(PARTEST_STRING_PARAM nodeTag) : XMLNode(nodeTag) {}
			virtual ~XMLContainerNode() = default;

			/**
			* Add a new child node to this node. Return non-owning pointer to the new child.
			*/
			XMLNode *addChild(std::unique_ptr<XMLNode> child)
			{
				child->updateDepth(depth + 1);
				children.push_back(std::move(child));
				return children.back().get();
			}
		};

		// Root node for a JUnit XML file, contains metrics for the entire test suite
		struct TestSuitesNode : public XMLContainerNode
		{
		protected:
			std::string openTag() const override
			{
				std::chrono::duration<double> seconds = time;

				std::ostringstream out;
				out << makeIndent() << '<' << nodeTag
					<< " name=\"" << sanitizeAttrib(name) << "\""
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

			explicit TestSuitesNode(PARTEST_STRING_PARAM nodeTag = JUNIT_TESTSUITES) : XMLContainerNode(nodeTag) {}
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
				out << makeIndent() << '<' << nodeTag
					<< " name=\"" << sanitizeAttrib(name) << "\""
					<< " tests=\"" << tests << "\""
					<< " failures=\"" << failures << "\""
					<< " assertions=\"" << assertions << "\""
					<< " time=\"" << seconds.count() << "\""
					<< " timestamp=\"" << toIso8601(timestamp) << "\"";
				if(!file.empty())
					out << " file=\"" << sanitizeAttrib(file) << "\"";
				out << ">";

				return out.str();
			}

		public:

			std::string file;      // Source code file for this test suite

			explicit TestSuiteNode(PARTEST_STRING_PARAM nodeTag = JUNIT_TESTSUITE) : TestSuitesNode(nodeTag) {}
		};

		struct TestCaseNode : public XMLContainerNode
		{
		protected:
			std::string openTag() const override
			{
				std::chrono::duration<double> seconds = time;

				std::ostringstream out;
				out << makeIndent() << '<' << nodeTag
					<< " name=\"" << sanitizeAttrib(name) << "\""
					<< " classname=\"" << sanitizeAttrib(classname) << "\""
					<< " assertions=\"" << assertions << "\""
					<< " time=\"" << seconds.count() << "\"";
				if(!file.empty())
					out << " file=\"" << sanitizeAttrib(file) << "\"";
				if(line > 0)
					out << " line=\"" << line << "\"";
				out << ">";

				return out.str();
			}

		public:
			std::string name;		// The name of this test case, often the function name
			std::string classname;	// The name of the parent class/folder, often the same as the suite's name
			size_t assertions = 0;	// Number of assertions checked during test case execution
        
			// Execution time of the test in seconds
			std::chrono::steady_clock::duration time = std::chrono::steady_clock::duration(0);

			std::string file;		// Source code file of this test case
			int line = 0;			// Source code line number of the start of this test case

			explicit TestCaseNode(PARTEST_STRING_PARAM nodeTag = JUNIT_TESTCASE) : XMLContainerNode(nodeTag) {}
		};

		// Properties node, optional, containing individual property nodes
		struct PropertiesNode : public XMLContainerNode
		{
			explicit PropertiesNode(PARTEST_STRING_PARAM nodeTag = JUNIT_PROPERTIES) : XMLContainerNode(nodeTag) {}
		};

		// Property node, may contain either a value in the XML, or a text body
		struct PropertyNode : public XMLNode
		{
		protected:
			std::string openTag() const override
			{
				if(valueAsBody)
					return makeIndent() + '<' + nodeTag + '>';
				return makeIndent() + '<' + nodeTag + " value=\"" + sanitizeAttrib(value) + "\">";
			}

			std::string bodyText() const override
			{
				if(valueAsBody)
					return sanitizeText(value) + '\n';
				return "";
			}

		public:
			std::string name;
			std::string value;
			bool valueAsBody = false;

			explicit PropertyNode(PARTEST_STRING_PARAM nodeTag = JUNIT_PROPERTY) : XMLNode(nodeTag) {}
		};

		// Suite or Test level log from stdout
		struct SystemOutNode : public XMLNode
		{
		protected:
			std::string bodyText() const override
			{
				return sanitizeText(body) + '\n';
			}

		public:
			std::string body;

			explicit SystemOutNode(PARTEST_STRING_PARAM nodeTag = JUNIT_SYSTEM_OUT) : XMLNode(nodeTag) {}
		};

		// Suite or Test level log from stderr
		struct SystemErrNode : public XMLNode
		{
		protected:
			std::string bodyText() const override
			{
				return sanitizeText(body) + '\n';
			}

		public:
			std::string body;

			explicit SystemErrNode(PARTEST_STRING_PARAM nodeTag = JUNIT_SYSTEM_ERR) : XMLNode(nodeTag) {}
		};

		// Used to report that a test was skipped
		struct SkippedNode : public XMLSelfClosingNode
		{
		protected:
			std::string openTag() const override
			{
				return makeIndent() + '<' + nodeTag + " message=\"" + sanitizeAttrib(message) + "\" />";
			}

		public:
			std::string message;

			explicit SkippedNode(PARTEST_STRING_PARAM nodeTag = JUNIT_SKIPPED) : XMLSelfClosingNode(nodeTag) {}
		};

		// Used to report the results of a failed test, usually from a failed assertion
		struct FailureNode : public XMLNode
		{
		protected:
			std::string openTag() const override
			{
				return makeIndent() + '<' + nodeTag
					+ " message=\"" + sanitizeAttrib(message)
					+ "\" type=\"" + sanitizeAttrib(type)
					+ "\">";
			}

			std::string bodyText() const override
			{
				return sanitizeText(body) + '\n';
			}

		public:
			std::string message;	// General message about the failure
			std::string type;		// Failure type, generally assertion type
			std::string body;		// Detailed description of the failure

			explicit FailureNode(PARTEST_STRING_PARAM nodeTag = JUNIT_FAILURE) : XMLNode(nodeTag) {}
		};

		// Same as failure node, but represents an unexpected error during test execution
		struct ErrorNode : public XMLNode
		{
		protected:
			std::string openTag() const override
			{
				return makeIndent() + '<' + nodeTag
					+ " message=\"" + sanitizeAttrib(message)
					+ "\" type=\"" + sanitizeAttrib(type)
					+ "\">";
			}

			std::string bodyText() const override
			{
				return sanitizeText(body) + '\n';
			}

		public:
			std::string message;
			std::string type;
			std::string body;

			explicit ErrorNode(PARTEST_STRING_PARAM nodeTag = JUNIT_ERROR) : XMLNode(nodeTag) {}
		};
	}
}

#endif