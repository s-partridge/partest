#ifndef PARTEST_COMMON_H
#define PARTEST_COMMON_H

#include <partest/compat.h>

// Macro pair to stringify a macro value. The first macro is needed to ensure that the argument is expanded before stringification.
#define PARTEST_STRINGIFY_HELPER(x) #x
#define PARTEST_STRINGIFY_MACRO(x) PARTEST_STRINGIFY_HELPER(x)

#define _PARTEST_VERSION_MAJOR 0
#define _PARTEST_VERSION_MINOR 3
#define _PARTEST_VERSION_PATCH 1

// Expand the version numbers into a string literal
// This requires first converting the numbers to string literals, then concatenating them

PARTEST_INLINE_VAR_17 constexpr unsigned PARTEST_VERSION_MAJOR = _PARTEST_VERSION_MAJOR;
PARTEST_INLINE_VAR_17 constexpr unsigned PARTEST_VERSION_MINOR = _PARTEST_VERSION_MINOR;
PARTEST_INLINE_VAR_17 constexpr unsigned PARTEST_VERSION_PATCH = _PARTEST_VERSION_PATCH;

PARTEST_INLINE_VAR_17 constexpr const char* PARTEST_VERSION_STRING = 
    PARTEST_STRINGIFY_MACRO(_PARTEST_VERSION_MAJOR) "." 
    PARTEST_STRINGIFY_MACRO(_PARTEST_VERSION_MINOR) "." 
    PARTEST_STRINGIFY_MACRO(_PARTEST_VERSION_PATCH);

#undef _PARTEST_VERSION_MAJOR
#undef _PARTEST_VERSION_MINOR
#undef _PARTEST_VERSION_PATCH

// Includes required regardless of C++ version
#include <memory>
#include <string>
#include <sstream>
#include <type_traits>

// Includes required for C++20 and later
#if PARTEST_CPP_VERSION >= 20
#include <format>
#include <concepts>
// Includes only required prior to C++20
#else 
#include <cstring>
#endif

// Includes required for C++17 and later
#if PARTEST_CPP_VERSION >= 17
#include <string_view>
#endif

// Use std::string_view for C++17 and later, const std::string& for C++11 and C++14
#if PARTEST_CPP_VERSION >= 17
	#define PARTEST_STRING_PARAM std::string_view
	#define PARTEST_STRING_PARAM_TO_STRING(PARTEST_STRING_PARAM) std::string(PARTEST_STRING_PARAM)
#else
	#define PARTEST_STRING_PARAM const std::string&
	#define PARTEST_STRING_PARAM_TO_STRING(PARTEST_STRING_PARAM) (PARTEST_STRING_PARAM)	
#endif

namespace partest
{
/**
* SHIMS
* C++11 is missing some simple utility functions that are available in later versions. These are provided here for compatibility.
*/
#if PARTEST_CPP_VERSION >= 14
	using std::make_unique;
	using std::decay_t;
	using std::enable_if_t;
#else
	template<typename T, typename... Args>
	std::unique_ptr<T> make_unique(Args&&... args)
	{
		return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
	}
	template<typename T>
	using decay_t = typename std::decay<T>::type;

	template<bool B, typename T = void>
	using enable_if_t = typename std::enable_if<B, T>::type;
#endif

	namespace traits
	{
		// Traits for raw character types
		template <typename T> struct is_char_type : std::false_type {};
		template <> struct is_char_type<char>     : std::true_type {};
		template <> struct is_char_type<wchar_t>  : std::true_type {};
		template <> struct is_char_type<char16_t>  : std::true_type {};
		template <> struct is_char_type<char32_t>  : std::true_type {};

#if PARTEST_CPP_VERSION >= 20
		template <> struct is_char_type<char8_t>  : std::true_type {};
#endif

		// Traits for cstring types
		template <typename T> struct is_cstring_type_impl : std::false_type {};
		// Remove low-level const (and volatile) to simply evaluate the basic type
		template <typename T> struct is_cstring_type_impl<T*> : is_char_type<typename std::remove_cv<T>::type> {};

		// Decay the type first to handle arrays, references, and top-level const (the const keyword in `const T *const`)
		template <typename T> struct is_cstring_type : is_cstring_type_impl<typename partest::decay_t<T>> {};

		// Import std::to_string into this namespace for ADL
		using std::to_string;

		// Trait to check if T has a to_string function defined
		template<typename T, typename = void>
		struct has_to_string : std::false_type {};

		// Specialization that does the checking. Checks for std::to_string(T)
		// but also allows for ADL to find a to_string in the same namespace as T
		template<typename T>
		struct has_to_string<T, decltype(void(static_cast<std::string>(to_string(std::declval<T>()))))>
			: std::true_type {};

		// Trait to check if T can be streamed to an std::ostream
		template<typename T, typename = void>
		struct is_streamable : std::false_type {};

		// Specialization that does the checking
		template<typename T>
		struct is_streamable<T, decltype(void(std::declval<std::ostream&>() << std::declval<T>()))>
			: std::true_type {};
	}

#if PARTEST_CPP_VERSION >= 20
	namespace concepts
	{
		template <typename T>
		concept to_stringable = requires(T t) { to_string(t); };
		template <typename T>
		concept streamable = requires(T t, std::ostream & os) { os << t; };
	}
#endif

	/**
	* Convert a value to string if possible
	* Try to use std::to_string if available, otherwise fall back to streaming to std::ostream
	* Single function using if constexpr for C++17 and later
	* 
	* Specialization for const char* to handle null pointers gracefully
	* 
	* @param value The value to convert to string
	* @return The string representation of the value, or a placeholder if not convertible
	*/
	inline std::string maybeStringify(const char* value)
	{
		if(value == nullptr)
			return "<nullptr>";
		else
			return std::string(value);
	}

	/**
	* Convert a value to string if possible
	* Try to use std::to_string if available, otherwise fall back to streaming to std::ostream
	* Single function using if constexpr for C++17 and later
	* 
	* Specialization for char* to handle null pointers gracefully
	* 
	* @param value The value to convert to string
	* @return The string representation of the value, or a placeholder if not convertible
	*/
	inline std::string maybeStringify(char *value)
	{
		return maybeStringify(value);
	}

#if PARTEST_CPP_VERSION >= 20
	/**
	* Convert a value to string if possible
	* Try to use std::to_string if available, otherwise fall back to streaming to std::ostream
	* Uses concepts in C++20 and later
	* 
	* @param value The value to convert to string
	* @return The string representation of the value, or a placeholder if not convertible
	*/
	template<typename T>
	std::string maybeStringify(const T& value)
	{
		if constexpr(concepts::streamable<T>)
		{
			std::ostringstream out;
			out << value;
			return out.str();
		}
		else if constexpr(concepts::to_stringable<T>)
		{
			// For correct ADL lookup
			using std::to_string;
			return static_cast<std::string>(to_string(value));
		}
		else
		{
			// Fallback for types that cannot be converted to string
			return std::format("<unprintable type: {}>", typeid(T).name());
		}
	}

#elif PARTEST_CPP_VERSION >= 17
	/**
	* Convert a value to string if possible
	* Try to use std::to_string if available, otherwise fall back to streaming to std::ostream
	* Single function using if constexpr for C++17 and later
	* 
	* @param value The value to convert to string
	* @return The string representation of the value, or a placeholder if not convertible
	*/
	template<typename T>
	std::string maybeStringify(const T& value)
	{
		if constexpr(traits::is_streamable<T>::value)
		{
			std::ostringstream out;
			out << value;
			return out.str();
		}
		else if constexpr(traits::has_to_string<T>::value)
		{
			// For correct ADL lookup
			using std::to_string;
			return static_cast<std::string>(to_string(value));
		}
		else
		{
			// Fallback for types that cannot be converted to string
			const char *typeName = typeid(T).name();

			std::string result;
			result.reserve(19 + strlen(typeName) + 1);
			result += "<unprintable type: ";
			result += typeName;
			result += ">";
			return result;
		}
	}
#else
	/**
	* Convert a value to string if possible
	* Try to use std::to_string if available, otherwise fall back to streaming to std::ostream
	* 
	* @param value The value to convert to string
	* @return The string representation of the value, or a placeholder if not convertible
	*/
	template <typename T>
	typename std::enable_if<traits::is_streamable<T>::value, std::string>::type
	maybeStringify(const T& value)
	{
		std::ostringstream out;
		out << value;
		return out.str();
	}

	/**
	* Convert a value to string if possible
	* Try to use std::to_string if available, otherwise fall back to streaming to std::ostream
	* 
	* @param value The value to convert to string
	* @return The string representation of the value, or a placeholder if not convertible
	*/
	template <typename T>
	typename std::enable_if<!traits::is_streamable<T>::value && traits::has_to_string<T>::value, std::string>::type
	maybeStringify(const T& value)
	{
		// For correct ADL lookup
		using std::to_string;
		return static_cast<std::string>(to_string(value));
	}

	/**
	* Convert a value to string if possible
	* Try to use std::to_string if available, otherwise fall back to streaming to std::ostream
	* 
	* @param value The value to convert to string
	* @return The string representation of the value, or a placeholder if not convertible
	*/
	template <typename T>
	typename std::enable_if<!traits::has_to_string<T>::value && !traits::is_streamable<T>::value, std::string>::type
	maybeStringify(const T& value)
	{
		const char *typeName = typeid(T).name();

		std::string result;
		result.reserve(19 + strlen(typeName) + 1);
		result += "<unprintable type: ";
		result += typeName;
		result += '>';
		return result;
	}
#endif


#if PARTEST_CPP_VERSION >= 20
// For C++20, use concepts
	#define PARTEST_ENABLE_IF_INVOCABLE(MaybeInvocable) std::invocable MaybeInvocable

#elif PARTEST_CPP_VERSION >= 17
// For C++17, use the standard library trait
	#define PARTEST_ENABLE_IF_INVOCABLE(MaybeInvocable) typename MaybeInvocable, typename = std::enable_if_t<std::is_invocable_v<MaybeInvocable>>

// For C++11, C++14, define our own trait and use it
#else
	// Trait to check if a type is callable (i.e., can be invoked like a function)
	// Used to constrain the addTest and subtest functions to only accept callable types
	namespace traits
	{
		// Primary template handles all types
		template<typename MaybeInvocable>
		// Specialization that does the checking
		struct is_callable
		{
		private:
			// SFINAE test for callable types
			// decltype is a language keyword, not a function call. When provided, the type is that of the second argument.
			// If T can't be resolved to a function taking zero parameters,
			// the entire template is discarded and the only remaining possibility is std::false_type
			template<typename T>
			static std::true_type check(decltype(std::declval<T>()(), 0));
			template<typename T>
			static std::false_type check(...);
		public:
			// Value is true if MaybeInvocable is callable, false otherwise
			// The 0 here is a dummy value that forces check<T> to be evaluated with one argument, rather than the (...) overload.
			// if MaybeInvocable is not a callable type with zero arguments, the second overload is chosen because it does not match declval<T>()(),
			// which requires T to be callable with no arguments.
			// Thus, it falls back to the ellipsis version, which always returns false_type.
			static constexpr bool value = decltype(check<MaybeInvocable>(0))::value;
		};

		// NOLINTNEXTLINE(bugprone-macro-parentheses)
		// Helper macro to enable functions only if the provided type is callable
		// If is_callable evaluates to true, the function is enabled and can be instantiated
		// If is_callable evaluates to false, type does not exist, causing a substitution failure
		#define PARTEST_ENABLE_IF_INVOCABLE(MaybeInvocable) typename MaybeInvocable, typename = typename std::enable_if<partest::traits::is_callable<MaybeInvocable>::value>::type
	}
#endif // PARTEST_CPP_VERSION
}

#endif // PARTESTCOMMON_H