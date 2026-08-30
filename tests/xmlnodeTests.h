#ifndef XML_NODE_TESTS_H
#define XML_NODE_TESTS_H

#include <partest/testbase.h>
#include <partest/xml/xmlnodes.h>
#include <partest/stringops.h>
#include <ctime>

class XMLNodeTests : public partest::TestBase
{
public:
	XMLNodeTests() : partest::TestBase("XMLNodeTests", "Validation for JUnit XML node types")
	{
		addTest("TimeConversion", "Clock to string conversion tests", partest::TEST_FLAGS_INHERIT,
			PARTEST_CTX(this) { testToIso8601(ctx); });

		addTest("XMLNode",
			partest::TEST_FLAGS_INHERIT,
			PARTEST_CTX(this) { testXMLNode(ctx); });

		addTest("XMLSelfClosingNode",
			partest::TEST_FLAGS_INHERIT,
			PARTEST_CTX(this) { testXMLSelfClosingNode(ctx); });
		addTest("XMLContainerNode",
			partest::TEST_FLAGS_INHERIT,
			PARTEST_CTX(this) { tesXMLContainerNode(ctx); });

		addTest("TestSuitesNode",
			partest::TEST_FLAGS_SKIP,
			PARTEST_CTX(this) { testTestSuitesNode(ctx); });

		addTest("TestSuiteNode",
			partest::TEST_FLAGS_SKIP,
			PARTEST_CTX(this) { testTestSuiteNode(ctx); });

		addTest("TestCaseNode",
			partest::TEST_FLAGS_SKIP,
			PARTEST_CTX(this) { testTestCaseNode(ctx); });
		
		addTest("PropertiesNode",
			partest::TEST_FLAGS_SKIP,
			PARTEST_CTX(this) { testPropertiesNode(ctx); });
		
		addTest("PropertyNode",
			partest::TEST_FLAGS_SKIP,
			PARTEST_CTX(this) { testPropertyNode(ctx); });

		addTest("LoggingNodes",
			partest::TEST_FLAGS_SKIP,
			PARTEST_CTX(this) { testLoggingNodes(ctx); });

		addTest("ResultNodes",
			partest::TEST_FLAGS_SKIP,
			PARTEST_CTX(this) { testResultNodes(ctx); });
	}

		//constexpr const char *JUNIT_TESTSUITES = "testsuites";
		//constexpr const char *JUNIT_TESTSUITE = "testsuite";
		//constexpr const char *JUNIT_TESTCASE = "testcase";
		//constexpr const char *JUNIT_PROPERTIES = "properties";
		//constexpr const char *JUNIT_PROPERTY = "property";
		//constexpr const char *JUNIT_SYSTEM_OUT = "system-out";
		//constexpr const char *JUNIT_SYSTEM_ERR = "system-err";
		//constexpr const char *JUNIT_SKIPPED = "skipped";
		//constexpr const char *JUNIT_FAILURE = "failure";
		//constexpr const char *JUNIT_ERROR = "error";

	void testToIso8601(TestContext &ctx)
	{
		std::chrono::system_clock::time_point date = std::chrono::system_clock::from_time_t(0);

		std::string timestamp = partest::toIso8601(date);

		ASSERT_EQUAL(timestamp, "1970-01-01T00:00:00Z");

		// July 4, 2026, 12:01 AM EST (in UTC)
		date = std::chrono::system_clock::from_time_t(1783141260);

		timestamp = partest::toIso8601(date);

		ASSERT_EQUAL(timestamp, "2026-07-04T05:01:00Z");
	}

	void testXMLNode(TestContext &ctx)
	{
		// Create dummy for abstract class
		class DummyNode : public partest::xml::XMLNode
		{
			protected:
				std::string bodyText() const override { return "    sample\n"; }
			public:
				explicit DummyNode(PARTEST_STRING_PARAM nodeTag) : XMLNode(nodeTag) {}

				// Accessors for node internal testing
				void setDepth(unsigned newDepth) { updateDepth(newDepth); }
				unsigned getDepth() const { return depth; }
				
				std::string getOpenTag() const { return openTag(); }
				std::string getBodyText() const { return bodyText(); }
				std::string getCloseTag() const { return closeTag(); }

				std::string fullTag() const { return makeWholeTag(); }
		};

		const char *nodeName = "dummy";
		const char *expectedChain = "<dummy>\n    sample\n</dummy>\n";
		const char *chainWithIndent = "        <dummy>\n    sample\n        </dummy>\n";

		ctx.subtest("doesConstructorSetName", PARTEST_CTX(nodeName)
		{
			DummyNode node("dummy");
			ASSERT_EQUAL(node.nodeTag, "dummy");
		});

		ctx.subtest("doesBuildCorrectStrings", PARTEST_CTX(nodeName, expectedChain)
		{
			DummyNode node(nodeName);
			ASSERT_EQUAL(node.getOpenTag(), "<dummy>");
			ASSERT_EQUAL(node.getBodyText(), "    sample\n");
			ASSERT_EQUAL(node.getCloseTag(), "</dummy>");
			ASSERT_EQUAL(node.fullTag(), expectedChain);
		});

		ctx.subtest("doesBuildWithEmptyName", PARTEST_CTX()
		{
			DummyNode node("");
			ASSERT_EQUAL(node.nodeTag, "");
			ASSERT_EQUAL(node.getOpenTag(), "<>");
			ASSERT_EQUAL(node.getBodyText(), "    sample\n");
			ASSERT_EQUAL(node.getCloseTag(), "</>");
			ASSERT_EQUAL(node.fullTag(), "<>\n    sample\n</>\n");
		});

		// TODO: Add string sanitization tests for stringops.
		//  sanitizeAttrib is probably correct, but the underlying sanitizeText function doesn't return correctly for some control characters.
		ctx.subtest("doesSanitizationWork", partest::TEST_FLAGS_INHERIT.withExpectFailure(), PARTEST_CTX()
		{
			DummyNode node("<dummy>");
			ASSERT_EQUAL(node.nodeTag, "&lt;dummy&gt;");

			// List all classes of sanitization
			//lt, gt, amp, apos, quot
			// other tokens sanitized by the algorithm are: \n, \r, \t, and control characters
			DummyNode node2("<dummy>&\"'\n\r\t");
			ASSERT_EQUAL(node2.nodeTag, "&lt;dummy&gt;&amp;&quot;&apos;\n&#13;\t");
			// control characters. Those allowed by xml should pass. Those excluded by xml should be replaced with a space
			DummyNode node3("\x01\x02\x03\x04\x05\x06\x07\x08\x0B\x0C\x0E\x0F");
			ASSERT_EQUAL(node3.nodeTag, "         \n  ");
		});

		ctx.subtest("doesSetCorrectDepth", PARTEST_CTX(nodeName)
		{
			DummyNode node(nodeName);
			ASSERT_EQUAL(node.getDepth(), 0);
			node.setDepth(3);
			ASSERT_EQUAL(node.getDepth(), 3);
		});

		ctx.subtest("doesIndentationWork", PARTEST_CTX(nodeName, chainWithIndent)
		{
			DummyNode node(nodeName);
			node.setDepth(2);
			ASSERT_EQUAL(node.getOpenTag(), "        <dummy>");
			ASSERT_EQUAL(node.getCloseTag(), "        </dummy>");
			ASSERT_EQUAL(node.fullTag(), chainWithIndent);
		});

		ctx.subtest("doesStreamOperatorWork", PARTEST_CTX(nodeName, expectedChain)
		{
			DummyNode node(nodeName);
			std::ostringstream oss;
			oss << node;
			ASSERT_EQUAL(oss.str(), expectedChain);
		});
	}

	void testXMLSelfClosingNode(TestContext &ctx)
	{
		class DummySelfClosingNode : public partest::xml::XMLSelfClosingNode
		{
			public:
				explicit DummySelfClosingNode(PARTEST_STRING_PARAM nodeTag) : partest::xml::XMLSelfClosingNode(nodeTag) {}

				std::string getOpenTag() { return openTag(); }
				std::string getBodyText() { return bodyText(); }
				std::string getCloseTag() { return closeTag(); }
		};

		const char *nodeName = "node";
		const char *expectedChain = "<node />\n";
		DummySelfClosingNode node(nodeName);
		
		std::ostringstream oss;
		oss << node;

		ASSERT_EQUAL(nodeName, node.nodeTag);
		ASSERT_EQUAL(node.getOpenTag(), "<node />");
		ASSERT_EQUAL(node.getBodyText(), "");
		ASSERT_EQUAL(node.getCloseTag(), "");
		ASSERT_EQUAL(oss.str(), expectedChain);
	}

	void tesXMLContainerNode(TestContext &ctx)
	{
		const char *rootName = "node";
		const char *childName = "child";
		const char *subchildName = "subchild";
		const unsigned childCount = 3;
		const char *expectedChain = "<node>\n    <child>\n        <subchild>\n        </subchild>\n    </child>\n    <child>\n        <subchild>\n        </subchild>\n    </child>\n    <child>\n        <subchild>\n        </subchild>\n    </child>\n</node>\n";
		partest::xml::XMLContainerNode rootNode(rootName);

		ASSERT_EQUAL(rootName, rootNode.nodeTag);

		for(unsigned x = 0; x < childCount; ++x)
		{
			//static cast to correct type, since addChild returns a non - owning pointer to the base class
			partest::xml::XMLContainerNode *child = static_cast<partest::xml::XMLContainerNode *>(rootNode.addChild(partest::make_unique<partest::xml::XMLContainerNode>(childName)));
			ASSERT_EQUAL(child->nodeTag, childName);

			//static cast to correct type, since addChild returns a non - owning pointer to the base class
			partest::xml::XMLContainerNode *subchild = static_cast<partest::xml::XMLContainerNode *>(child->addChild(partest::make_unique<partest::xml::XMLContainerNode>(subchildName)));
			ASSERT_EQUAL(subchild->nodeTag, subchildName);
		}

		std::ostringstream oss;
		oss << rootNode;

		ASSERT_EQUAL(oss.str(), expectedChain);
	}
	void testTestSuitesNode(TestContext &ctx) {}
	void testTestSuiteNode(TestContext &ctx) {}
	void testTestCaseNode(TestContext &ctx) {}
	void testPropertiesNode(TestContext &ctx) {}
	void testPropertyNode(TestContext &ctx) {}
	
	void testLoggingNodes(TestContext &ctx)
	{
		ctx.subtest("SystemOutNodeTest", PARTEST_CTX()
		{
		});
		
		ctx.subtest("SystemErrNodeTest", PARTEST_CTX()
		{
		});
	}

	// This covers skipped, failure, and error nodes
	void testResultNodes(TestContext &ctx)
	{
		// TestSkipped
		ctx.subtest("SkippedNodeTest", PARTEST_CTX()
		{

		});

		// TestFailure
		ctx.subtest("FailureNodeTest", PARTEST_CTX()
		{

		});

		// TestError
		ctx.subtest("ErrorNodeTest", PARTEST_CTX()
		{

		});
	}
};

#endif