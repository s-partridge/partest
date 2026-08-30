#ifndef PARTEST_ASSERTION_PARSER_H
#define PARTEST_ASSERTION_PARSER_H

#include <unordered_map>
#include <functional>
#include <sstream>
#include <string>

#include <partest/common.h>
#include <partest/assert.h>
#include <partest/assertresult.h>

namespace partest
{
	class AssertionParser
	{
		std::unordered_map<std::string, std::function<std::string(const partest::AssertionResult &)>> m_functionMap;

	public:
		using AssertionReader = std::function<std::string(const partest::AssertionResult &)>;
		using AssertionReaderConstIter = std::unordered_map<std::string, AssertionReader>::const_iterator;

		explicit AssertionParser() = default;

		void addFunction(PARTEST_STRING_PARAM key, const AssertionReader &readerFunction)
		{
			m_functionMap[PARTEST_STRING_PARAM_TO_STRING(key)] = readerFunction;
		}

		bool hasFunction(PARTEST_STRING_PARAM key) const
		{ 
			 return m_functionMap.find(PARTEST_STRING_PARAM_TO_STRING(key)) != m_functionMap.end();
		}
		
		std::string parseAssertion(const AssertionResult &assertion) const
		{
			AssertionReaderConstIter func = m_functionMap.find(assertion.assertType());
			if(func != m_functionMap.end())
			{
				return func->second(assertion);
			}
			
			// Todo: Maybe raise exception if assertion parse doesn't exist?
			return "ERROR: Unknown assertion type <" + assertion.assertType() + ">";
		}
	};

	namespace simple
	{
		inline bool isProbablyLambda(PARTEST_STRING_PARAM func)
		{
			return func == "operator ()" || func == "operator()";
		}

		inline void appendSourceInfo(std::ostream &s, const partest::AssertionResult &assertion)
		{
			s << " at: " << getFilename(assertion.file);
			s << ":" << assertion.line;
			
			// Include function name if it's not anonymous scope
			if(!isProbablyLambda(assertion.func))
				s << " in: " << assertion.func;
		}
		
		// Generic assertion parser functions
		inline std::string parseAssertTrue(const partest::AssertionResult &assertion)
		{
			std::ostringstream oss;

			if(assertion.passed())
			{
				oss << "PASSED: " << assertion.assertType() << "(" << assertion.getCondition() << ')';
			}
			else
			{
				oss << "FAILED: " << assertion.assertType() << '(' << assertion.getMetadata(MetaKeys::FullExpr) << ')'
					<< "\n actual: " << assertion.getMetadata(MetaKeys::Actual)
					<< "\n expected: " << assertion.getMetadata(MetaKeys::Expected);
			}

			oss << std::endl;
			appendSourceInfo(oss, assertion);

			return oss.str();
		}
		// Identical logic to AssertTrue
		inline std::string parseAssertFalse(const partest::AssertionResult &assertion) { return parseAssertTrue(assertion); }

		inline std::string parseAssertEqual(const partest::AssertionResult &assertion)
		{
			std::ostringstream oss;

			if(assertion.passed())
			{
				oss << assertion.assertType() << " PASSED:\n (" << assertion.getCondition() << ')';
			}
			else
			{
				oss << "FAILED: " << assertion.assertType() << '(' << assertion.getMetadata(MetaKeys::FullExpr) << ')'
					<< "\n Actual (" << assertion.getMetadata(MetaKeys::ExprA) 
					<< "):\n " << assertion.getMetadata(MetaKeys::Actual)
					<< "\n Expected (" << assertion.getMetadata(MetaKeys::ExprB)
					<< "):\n " << assertion.getMetadata(MetaKeys::Expected);
			}
			oss << std::endl;
			appendSourceInfo(oss, assertion);

			return oss.str();
		}
		inline std::string parseAssertNotEqual(const partest::AssertionResult &assertion) { return parseAssertEqual(assertion); }

		inline std::string parseAssertApproxEqual(const partest::AssertionResult &assertion)
		{
			std::ostringstream oss;

			if(assertion.passed())
			{
				oss << assertion.assertType() << " PASSED:\n (" << assertion.getCondition() << ')';
			}
			else
			{
				oss << "FAILED: " << assertion.assertType() << '(' << assertion.getMetadata(MetaKeys::FullExpr) << ')'
					<< "\n Actual (" << assertion.getMetadata(MetaKeys::ExprA) 
					<< "):\n " << assertion.getMetadata(MetaKeys::Actual)
					<< "\n Not within tolerance (" << assertion.getMetadata(MetaKeys::ExprB) << ") +- (" << assertion.getMetadata(MetaKeys::ExprC)
					<< "):\n " << assertion.getMetadata(MetaKeys::Expected) << " +-" << assertion.getMetadata(MetaKeys::Epsilon);
			}
			oss << std::endl;
			appendSourceInfo(oss, assertion);

			return oss.str();

		}
		inline std::string parseAssertApproxNotEqual(const partest::AssertionResult &assertion) 
		{
			std::ostringstream oss;

			if(assertion.passed())
			{
				oss << assertion.assertType() << " PASSED:\n (" << assertion.getCondition() << ')';
			}
			else
			{
				oss << "FAILED: " << assertion.assertType() << '(' << assertion.getMetadata(MetaKeys::FullExpr) << ')'
					<< "\n Actual (" << assertion.getMetadata(MetaKeys::ExprA) 
					<< "):\n " << assertion.getMetadata(MetaKeys::Actual)
					<< "\n Should not be within tolerance (" << assertion.getMetadata(MetaKeys::ExprB) << ") +- (" << assertion.getMetadata(MetaKeys::ExprC)
					<< "):\n " << assertion.getMetadata(MetaKeys::Expected) << " +-" << assertion.getMetadata(MetaKeys::Epsilon);
			}
			oss << std::endl;
			appendSourceInfo(oss, assertion);

			return oss.str();
		}

		inline std::string parseAssertGreater(const partest::AssertionResult &assertion) { return parseAssertTrue(assertion); }
		inline std::string parseAssertGreaterEqual(const partest::AssertionResult &assertion) { return parseAssertTrue(assertion); }
		inline std::string parseAssertLess(const partest::AssertionResult &assertion) { return parseAssertTrue(assertion); }
		inline std::string parseAssertLessEqual(const partest::AssertionResult &assertion) { return parseAssertTrue(assertion); }
		inline std::string parseAssertThrows(const partest::AssertionResult &assertion)
		{
			std::ostringstream oss;

			if(assertion.passed())
			{
				oss << assertion.assertType() << " PASSED:\n (" << assertion.getCondition() << ')';
			}
			else
			{
				oss << "FAILED: " << assertion.assertType() << '(' << assertion.getMetadata(MetaKeys::FullExpr) << ')'
					<< "\n (" << assertion.getMetadata(MetaKeys::ExprA)
					<< ") triggered: " << assertion.getMetadata(MetaKeys::Actual)
					<< "\n Should have triggered: (" << assertion.getMetadata(MetaKeys::Expected) << ")";
			}
			oss << std::endl;
			appendSourceInfo(oss, assertion);

			return oss.str();
		}
		inline std::string parseAssertNothrow(const partest::AssertionResult &assertion)
		{
			std::ostringstream oss;

			if(assertion.passed())
			{
				oss << assertion.assertType() << " PASSED:\n (" << assertion.getCondition() << ')';
			}
			else
			{
				oss << "FAILED: " << assertion.assertType() << '(' << assertion.getMetadata(MetaKeys::FullExpr) << ')'
					<< "\n (" << assertion.getMetadata(MetaKeys::ExprA)
					<< ") triggered: " << assertion.getMetadata(MetaKeys::Actual)
					<< "\n Should have triggered: (" << assertion.getMetadata(MetaKeys::Expected) << ")";
			}
			oss << std::endl;
			appendSourceInfo(oss, assertion);

			return oss.str();
		}

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
			parser.addFunction(ASSERT_THROWS_STR, parseAssertThrows);
			parser.addFunction(ASSERT_NOTHROW_STR, parseAssertNothrow);
			return parser;
		}
	}
}

#endif