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
		addTest("TimeConversion", "Ensure that clock to string conversion is correct",
			partest::TEST_FLAGS_INHERIT,
			PARTEST_CTX(this) { testTimeConversion(ctx); });

		addTest("BasicXMLNode", "Ensure base node functionality is correct",
			partest::TEST_FLAGS_INHERIT,
			PARTEST_CTX(this) { tesXMLNode(ctx); });

		addTest("TestSuitesNode", "Ensure testsuites node is handled correctly",
			partest::TEST_FLAGS_SKIP,
			PARTEST_CTX(this) { testTestSuitesNode(ctx); });

		addTest("TestSuiteNode", "Ensure testsuite node is handled correctly",
			partest::TEST_FLAGS_SKIP,
			PARTEST_CTX(this) { testTestSuiteNode(ctx); });

		addTest("TestCaseNode", "Ensure testcase node is handled correctly",
			partest::TEST_FLAGS_SKIP,
			PARTEST_CTX(this) { testTestCaseNode(ctx); });
		
		addTest("PropertiesNode", "Ensure properties node is handled correctly",
			partest::TEST_FLAGS_SKIP,
			PARTEST_CTX(this) { testPropertiesNode(ctx); });
		
		addTest("PropertyNode", "Ensure property node is handled correctly",
			partest::TEST_FLAGS_SKIP,
			PARTEST_CTX(this) { testPropertyNode(ctx); });

		addTest("LoggingNodes", "Ensure system-out node is handled correctly",
			partest::TEST_FLAGS_SKIP,
			PARTEST_CTX(this) { testLoggingNodes(ctx); });

		addTest("ResultNodes", "Ensure result nodes are handled correctly",
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

	void testTimeConversion(TestContext &ctx)
	{
		std::chrono::system_clock::time_point date = std::chrono::system_clock::from_time_t(0);

		std::string timestamp = partest::toIso8601(date);

		ASSERT_EQUAL(timestamp, "1970-01-01T00:00:00Z");

		// July 4, 2026, 12:01 AM EST (in UTC)
		date = std::chrono::system_clock::from_time_t(1783141260);

		timestamp = partest::toIso8601(date);

		ASSERT_EQUAL(timestamp, "2026-07-04T05:01:00Z");
	}

	void tesXMLNode(TestContext &ctx)
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