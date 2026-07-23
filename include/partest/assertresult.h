#ifndef PARTEST_ASSERT_RESULT_H
#define PARTEST_ASSERT_RESULT_H

#include <map>
#include <atomic>
#include <string>

#include <partest/common.h>

namespace partest
{
	class AssertionResult;


	class AssertionResultView
	{
		using MetadataConstIter = std::map<std::string, std::string>::const_iterator;
		const AssertionResult *m_assertionResult;

	public:
		AssertionResultView(const AssertionResult &assertionResult);

		unsigned id() const noexcept;
		bool passed() const noexcept;
		PARTEST_STRING_PARAM assertType() const noexcept;
		PARTEST_STRING_PARAM condition() const noexcept;
		PARTEST_STRING_PARAM message() const noexcept;
		PARTEST_STRING_PARAM file() const noexcept;
		int line() const noexcept;

		bool hasMetadata(PARTEST_STRING_PARAM key) const noexcept;
		std::string getMetadata(PARTEST_STRING_PARAM key) const;
		MetadataConstIter metadataBegin() const noexcept;
		MetadataConstIter metadataEnd() const noexcept;
	};

	class AssertionResult
	{
	private:
		std::map<std::string, std::string> m_metadata; // Custom metadata associated with this assertion result
		
		unsigned int m_id; // Unique ID for this assertion result, used for tracking and filtering
		/**
		* Get a globally incrementing counter. Used internally to assign IDs to newly created test frames.
		* 
		* @return the next value for assertCount
		*/
		static unsigned int nextId() noexcept {
		
			static std::atomic<unsigned int> assertCount(0);
			return assertCount.fetch_add(1, std::memory_order_relaxed);
		}

	public:
		using MetadataConstIter = std::map<std::string, std::string>::const_iterator;
		// Get the unique ID for this assertion result
		unsigned int id() const noexcept { return m_id; }

		// Whether the assertion passed or failed
		bool passed;
		// The string name of the assertion type (e.g., "ASSERT_TRUE", "ASSERT_EQUAL")
		// This is provided by ASSERT_TRUE_STR, ASSERT_EQUAL_STR, etc.
		std::string assertType;
		// The condition that was evaluated. This is typically the text of the expression passed to the assertion macro
		// E.g., for ASSERT_TRUE(x > 0), this would be "x > 0"; for ASSERT_EQUAL(a, b), this would be "a, b"
		std::string condition;
		// Custom message associated with the assertion, if any
		std::string message;
		// The file where the assertion was made. Typically provided by the __FILE__ macros
		std::string file;
		// The line number where the assertion was made. Typically provided by the __LINE__ macros
		int line;

		
		AssertionResult() : AssertionResult(false, "", "", "", "", 0) {}

		AssertionResult(
			bool passed,
			PARTEST_STRING_PARAM assertType,
			PARTEST_STRING_PARAM condition,
			PARTEST_STRING_PARAM file,
			int line)
			: AssertionResult(passed, assertType, condition, "", file, line) {}

		/**
		* Constructor for AssertionResult. Handles both equality and non-equality assertions.
		* 
		* @param passed Whether the assertion passed or failed.
		* @param assertType The name of the assertion type (e.g., "ASSERT_TRUE", "ASSERT_EQUAL").
		* @param condition The condition that was evaluated. This is typically the text of the expression passed to the assertion macro.
		* @param message Custom message associated with the assertion, if any.
		* @param file The file where the assertion was made. Typically provided by the __FILE__ macro.
		* @param line The line number where the assertion was made. Typically provided by the __LINE__ macro.
		*/

		AssertionResult(
			bool passed,
			PARTEST_STRING_PARAM assertType,
			PARTEST_STRING_PARAM condition,
			PARTEST_STRING_PARAM message,
			PARTEST_STRING_PARAM file,
			int line)
				: m_id(nextId()), passed(passed), assertType(assertType),
				condition(condition), message(message), file(file), line(line) {}

		virtual ~AssertionResult() = default;

		/**
		* Set custom metadata key-value pair for this assertion result
		* 
		* @param key The metadata key
		* @param value The metadata value
		*/
		void setMetadata(PARTEST_STRING_PARAM key, PARTEST_STRING_PARAM value) { m_metadata[PARTEST_STRING_PARAM_TO_STRING(key)] = value; }

		template<typename T>
		void setMetadata(PARTEST_STRING_PARAM key, const T &value) { m_metadata[PARTEST_STRING_PARAM_TO_STRING(key)] = maybeStringify(value); }

		/**
		* Check whether metadata exists for the given key
		* 
		* @param key The metadata key
		* @return true if a value exists for `key`, false otherwise
		*/
		bool hasMetadata(PARTEST_STRING_PARAM key) const noexcept { return m_metadata.find(PARTEST_STRING_PARAM_TO_STRING(key)) != m_metadata.end(); }

		/**
		* Get the value of a metadata key for this assertion result
		* 
		* @param key The metadata key
		* @return The metadata value, or an empty string if the key does not exist
		*/
		std::string getMetadata(PARTEST_STRING_PARAM key) const
		{
			MetadataConstIter it = m_metadata.find(PARTEST_STRING_PARAM_TO_STRING(key));
			if(it != m_metadata.end())
			{
				return it->second;
			}
			return "";
		}

		MetadataConstIter metadataBegin() const noexcept { return m_metadata.cbegin(); }
		MetadataConstIter metadataEnd() const noexcept { return m_metadata.cend(); }
	};

	/**
	* AssertionResultView function definitions
	*/
	inline AssertionResultView::AssertionResultView(const AssertionResult &assertionResult) : m_assertionResult(&assertionResult) {}

	inline unsigned AssertionResultView::id() const noexcept { return m_assertionResult->id(); }
	inline bool AssertionResultView::passed() const noexcept { return m_assertionResult->passed; }
	inline PARTEST_STRING_PARAM AssertionResultView::assertType() const noexcept { return m_assertionResult->assertType; }
	inline PARTEST_STRING_PARAM AssertionResultView::condition() const noexcept { return m_assertionResult->condition; }
	inline PARTEST_STRING_PARAM AssertionResultView::message() const noexcept { return m_assertionResult->message; }
	inline PARTEST_STRING_PARAM AssertionResultView::file() const noexcept { return m_assertionResult->file; }
	inline int AssertionResultView::line() const noexcept { return m_assertionResult->line; }

	inline bool AssertionResultView::hasMetadata(PARTEST_STRING_PARAM key) const noexcept { return m_assertionResult->hasMetadata(key); }
	inline std::string AssertionResultView::getMetadata(PARTEST_STRING_PARAM key) const {return m_assertionResult->getMetadata(key); }

	inline AssertionResult::MetadataConstIter AssertionResultView::metadataBegin() const noexcept { return m_assertionResult->metadataBegin(); }
	inline AssertionResult::MetadataConstIter AssertionResultView::metadataEnd() const noexcept { return m_assertionResult->metadataEnd(); }
}
#endif