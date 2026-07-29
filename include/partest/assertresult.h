#ifndef PARTEST_ASSERT_RESULT_H
#define PARTEST_ASSERT_RESULT_H

#include <atomic>
#include <string>
#include <functional>
#include <unordered_map>

#include <partest/common.h>

namespace partest
{
	class AssertionResult;

	class MetaKey
	{
		const unsigned m_id; // Unique ID for the key instance, used to reference values stored in an assertionResult metadata map

		static unsigned nextId() noexcept {
		
			static std::atomic<unsigned> keyCount(0);
			return keyCount.fetch_add(1, std::memory_order_relaxed);
		}
	public:
		explicit MetaKey() noexcept : m_id(nextId()) {}
		MetaKey(const MetaKey &other) noexcept : m_id(other.m_id) {}
		unsigned id() const noexcept { return m_id; }

		bool operator==(const MetaKey &rhs) const noexcept { return m_id == rhs.m_id; }
		bool operator!=(const MetaKey &rhs) const noexcept { return m_id != rhs.m_id; }
	};
}

//
// Hash specialization for MetaKey, used as the key for AssertionResult metadata

template<>
struct std::hash<partest::MetaKey>
{
	size_t operator()(const partest::MetaKey &key) const
		noexcept(noexcept(std::hash<unsigned>{}(key.id())))
	{
		std::hash<unsigned> hash;
		return hash(key.id());
	}
};

namespace partest
{
	namespace MetaKeys
	{
		namespace detail
		{
			// Singleton TLU-safe generators for fixed framework keys
			inline const MetaKey& getActual() noexcept { static const MetaKey actual; return actual; }
			inline const MetaKey& getExpected() noexcept { static const MetaKey expected; return expected; }
			inline const MetaKey& getEpsilon() noexcept { static const MetaKey epsilon; return epsilon; }
			inline const MetaKey& getFloor() noexcept { static const MetaKey floor; return floor; }
			inline const MetaKey& getCeiling() noexcept { static const MetaKey ceiling; return ceiling; }
			inline const MetaKey& getContainer() noexcept { static const MetaKey container; return container; }

			inline const MetaKey& getFullExpr() noexcept { static const MetaKey fullExpr; return fullExpr; }
			inline const MetaKey& getExprA() noexcept { static const MetaKey exprA; return exprA; }
			inline const MetaKey& getExprB() noexcept { static const MetaKey exprB; return exprB; }
			inline const MetaKey& getExprC() noexcept { static const MetaKey exprC; return exprC; }
		}

		static const MetaKey Actual = detail::getActual();		// Used for the actual value evaluated by an assertion
		static const MetaKey Expected = detail::getExpected();	// Used for the expected value from an assertion
		static const MetaKey Epsilon = detail::getEpsilon();		// The tolerance value in assertions like ASSERT_APPROX_EQUAL
		static const MetaKey Floor = detail::getFloor();			// Floor value for range assertions
		static const MetaKey Ceiling = detail::getCeiling();		// Ceiling value for range assertions
		static const MetaKey Container = detail::getContainer();	// The container used for ASSERT_CONTAINS

		// For string representations of the expression and its raw arguments, in addition to their evaluated values
		static const MetaKey FullExpr = detail::getFullExpr();			// Used for the first expression passed to an assertion
		static const MetaKey ExprA = detail::getExprA();			// Used for the first expression passed to an assertion
		static const MetaKey ExprB = detail::getExprB();			// Used for the second expression passed to an assertion
		static const MetaKey ExprC = detail::getExprC();			// Used for the third expression passed to an assertion
	}

	class AssertionResultView
	{
		using MetadataConstIter = std::unordered_map<MetaKey, std::string>::const_iterator;
		const AssertionResult *m_assertionResult;

	public:
		AssertionResultView(const AssertionResult &assertionResult);

		unsigned id() const noexcept;
		bool passed() const noexcept;
		PARTEST_STRING_PARAM assertType() const noexcept;
		PARTEST_STRING_PARAM file() const noexcept;
		int line() const noexcept;

		bool hasMetadata(const MetaKey &key) const noexcept;
		std::string getMetadata(const MetaKey &key) const;
		MetadataConstIter metadataBegin() const noexcept;
		MetadataConstIter metadataEnd() const noexcept;
	};

	class AssertionResult
	{
	private:
		std::unordered_map<MetaKey, std::string> m_metadata; // Custom metadata associated with this assertion result
		
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
		using MetadataConstIter = std::unordered_map<MetaKey, std::string>::const_iterator;
		// Get the unique ID for this assertion result
		unsigned int id() const noexcept { return m_id; }

		// Whether the assertion passed or failed
		bool passed;
		// The string name of the assertion type (e.g., "ASSERT_TRUE", "ASSERT_EQUAL")
		// This is provided by ASSERT_TRUE_STR, ASSERT_EQUAL_STR, etc.
		std::string assertType;
		// The file where the assertion was made. Typically provided by the __FILE__ macros
		std::string file;
		// The line number where the assertion was made. Typically provided by the __LINE__ macros
		int line;

		/**
		* Constructor for AssertionResult. Handles both equality and non-equality assertions.
		* 
		* @param passed Whether the assertion passed or failed.
		* @param assertType The name of the assertion type (e.g., "ASSERT_TRUE", "ASSERT_EQUAL").
		* @param condition The condition that was evaluated. This is typically the text of the expression passed to the assertion macro.
		* @param file The file where the assertion was made. Typically provided by the __FILE__ macro.
		* @param line The line number where the assertion was made. Typically provided by the __LINE__ macro.
		*/
		AssertionResult(
			bool passed,
			PARTEST_STRING_PARAM assertType,
			PARTEST_STRING_PARAM file,
			int line)
				: m_id(nextId()), passed(passed), assertType(assertType),
				 file(file), line(line) {}

		virtual ~AssertionResult() = default;

		std::string getCondition() const { return getMetadata(MetaKeys::FullExpr); }

		/**
		* Set custom metadata key-value pair for this assertion result
		* 
		* @param key The metadata key
		* @param value The metadata value
		*/
		void setMetadata(const MetaKey &key, PARTEST_STRING_PARAM value) { m_metadata[key] = value; }

		template<typename T>
		void setMetadata(const MetaKey &key, const T &value) { m_metadata[key] = maybeStringify(value); }

		/**
		* Check whether metadata exists for the given key
		* 
		* @param key The metadata key
		* @return true if a value exists for `key`, false otherwise
		*/
		bool hasMetadata(MetaKey key) const noexcept { return m_metadata.find(key) != m_metadata.end(); }

		/**
		* Get the value of a metadata key for this assertion result
		* 
		* @param key The metadata key
		* @return The metadata value, or an empty string if the key does not exist
		*/
		std::string getMetadata(MetaKey key) const
		{
			MetadataConstIter it = m_metadata.find(key);
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

	inline PARTEST_STRING_PARAM AssertionResultView::file() const noexcept { return m_assertionResult->file; }
	inline int AssertionResultView::line() const noexcept { return m_assertionResult->line; }

	inline bool AssertionResultView::hasMetadata(const MetaKey &key) const noexcept { return m_assertionResult->hasMetadata(key); }
	inline std::string AssertionResultView::getMetadata(const MetaKey &key) const {return m_assertionResult->getMetadata(key); }

	inline AssertionResult::MetadataConstIter AssertionResultView::metadataBegin() const noexcept { return m_assertionResult->metadataBegin(); }
	inline AssertionResult::MetadataConstIter AssertionResultView::metadataEnd() const noexcept { return m_assertionResult->metadataEnd(); }
}
#endif