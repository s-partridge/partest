#ifndef XML_NODE_TESTS_H
#define XML_NODE_TESTS_H

#include <partest/testbase.h>
#include <partest/xml/xmlnodes.h>
#include <ctime>

class XMLNodeTests : public partest::TestBase
{
public:
	XMLNodeTests() : partest::TestBase("XMLNodeTests", "Validation for JUnit XML node types")
	{
		addTest("TestTimeConversion", "Ensure that clock to string conversion is correct",
			partest::TEST_FLAGS_INHERIT,
			[this]() { testTimeConversion(); });

		addTest("TestBasicXMLNode", "Ensure base node functionality is correct",
			partest::TEST_FLAGS_INHERIT,
			[this]() { tesXMLNode(); });
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
		//constexpr const char *JUNIT_ERROR = "error";*/

	void testTimeConversion()
	{
		// July 4, 2026, 12:01 AM EST
		std::chrono::system_clock::time_point date = std::chrono::system_clock::from_time_t(0);

		std::string timestamp = partest::xml::toIso8601(date);

		ASSERT_EQUAL("1970-01-01T00:00:00", timestamp);

		// July 4, 2026, 12:01 AM EST
		date = std::chrono::system_clock::from_time_t(1783141260);

		timestamp = partest::xml::toIso8601(date);

		ASSERT_EQUAL("2026-07-04T05:01:00", timestamp);


	}
	void tesXMLNode()
	{
		const char *rootName = "node";
		const char *childName = "child";
		const char *subchildName = "subchild";
		const unsigned childCount = 3;
		const char *expectedChain = "<node><child><subchild></subchild></child><child><subchild></subchild></child><child><subchild></subchild></child></node>";
		partest::xml::JUnitXMLNode rootNode(rootName);

		ASSERT_EQUAL(rootName, rootNode.nodeTag);

		for(unsigned x = 0; x < childCount; ++x)
		{
			auto child = rootNode.addChild(partest::make_unique<partest::xml::JUnitXMLNode>(childName));
			ASSERT_EQUAL(childName, child->nodeTag);

			auto subchild = child->addChild(partest::make_unique<partest::xml::JUnitXMLNode>(subchildName));
			ASSERT_EQUAL(subchildName, subchild->nodeTag);
		}

		std::ostringstream oss;
		oss << rootNode;

		ASSERT_EQUAL(expectedChain, oss.str());
	}
	void testTestSuitesNode() {}
	void testTestSuiteNode() {}
	void testTestCaseNode() {}
	void testPropertiesNode() {}
	void testPropertyNode() {}
	void testSystemOutNode() {}
	void testSystemErrNode() {}

	// This covers skipped, failure, and error nodes
	void testResultNodes()
	{
		// TestSkipped
		subtest("SkippedNodeTest",[&]()
		{

		});

		// TestFailure
		subtest("FailureNodeTest",[&]()
		{

		});

		// TestError
		subtest("ErrorNodeTest",[&]()
		{

		});
	}
};

#endif