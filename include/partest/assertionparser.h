#ifndef PARTEST_ASSERTION_PARSER_H
#define PARTEST_ASSERTION_PARSER_H

#include <unordered_map>
#include <functional>

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

	// Generic assertion parser functions
}

#endif