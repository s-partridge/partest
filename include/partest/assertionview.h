#ifndef PARTEST_ASSERTION_VIEW_H
#define PARTEST_ASSERTION_VIEW_H

#include <partest/assertresult.h>

namespace partest
{
	class AssertionResultView
	{
		const AssertionResult *m_assertionResult;

	public:
		AssertionResultView(const AssertionResult &assertionResult) : m_assertionResult(&assertionResult) {}

		unsigned id() const noexcept { return m_assertionResult->id(); }
		bool passed() const noexcept { return m_assertionResult->passed; }
		PARTEST_STRING_PARAM assertType() const noexcept { return m_assertionResult->assertType; }
		PARTEST_STRING_PARAM condition() const noexcept { return m_assertionResult->condition; }
		PARTEST_STRING_PARAM message() const noexcept { return m_assertionResult->message; }
		PARTEST_STRING_PARAM file() const noexcept { return m_assertionResult->file; }
		int line() const noexcept { return m_assertionResult->line; }

		bool hasMetadata(PARTEST_STRING_PARAM key) const noexcept { return m_assertionResult->hasMetadata(key); }
		std::string getMetadata(PARTEST_STRING_PARAM key) const {return m_assertionResult->getMetadata(key); }

		AssertionResult::MetadataConstIter metadataBegin() const noexcept { return m_assertionResult->metadataBegin(); }
		AssertionResult::MetadataConstIter metadataEnd() const noexcept { return m_assertionResult->metadataEnd(); }
	};
}

#endif