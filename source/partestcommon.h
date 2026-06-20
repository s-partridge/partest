#ifndef PARTESTCOMMON_H
#define PARTESTCOMMON_H

#define PARTEST_VERSION_MAJOR 0
#define PARTEST_VERSION_MINOR 2
#define PARTEST_VERSION_PATCH 0

#define PARTEST_VERSION_STRING "0.2.0"

#if defined(_MSVC_LANG)
    // _MSVC_LANG is the definitive way to check for MSVC.
    // It's defined regardless of the /Zc:__cplusplus flag.
    #if _MSVC_LANG >= 202002L
        #define PARTEST_CPP_VERSION 20
    #elif _MSVC_LANG >= 201703L
        #define PARTEST_CPP_VERSION 17
    #elif _MSVC_LANG >= 201402L
        #define PARTEST_CPP_VERSION 14
	#else
		// Default to 11 for older MSVC versions.
		#define PARTEST_CPP_VERSION 11
    #endif
#elif defined(__cplusplus)
    // For Clang, GCC, and other compliant compilers.
    #if __cplusplus >= 202002L
        #define PARTEST_CPP_VERSION 20
    #elif __cplusplus >= 201703L
        #define PARTEST_CPP_VERSION 17
	#elif __cplusplus >= 201402L
        #define PARTEST_CPP_VERSION 14
	#else
		// Default to 11 for older versions.
		#define PARTEST_CPP_VERSION 11
	#endif
#else
    // Fallback if we can't determine the version. Assume the minimum.
    #define PARTEST_CPP_VERSION 11
#endif // defined(_MSVC_LANG)

/**
* constexpr usage by C++ standard version
C++11	Keyword introduced.
        Only a single return statement. No locals, no loops, no branching except ternary operator. Simple compile-time math, basic constant objects.
C++14	Allowed local variables, loops, if/switch. Writing normal-looking functions that run at compile time.
C++17	if constexpr, constexpr lambdas. Cleaner and more powerful template metaprogramming.
C++20	Allowed mutation of *this, virtual, new/delete, try/catch. Modifying objects at compile time, compile-time containers (std::vector, std::string).
*/

// --- C++11 constexpr support ---
#if PARTEST_CPP_VERSION >= 11
    #define PARTEST_CONSTEXPR_11 constexpr
#else
    #define PARTEST_CONSTEXPR_11
#endif

// --- C++14 constexpr support ---
#if PARTEST_CPP_VERSION >= 14
    #define PARTEST_CONSTEXPR_14 constexpr
#else
    #define PARTEST_CONSTEXPR_14
#endif

// --- C++17 constexpr support ---
#if PARTEST_CPP_VERSION >= 17
    #define PARTEST_CONSTEXPR_17 constexpr
#else
    #define PARTEST_CONSTEXPR_17
#endif

// --- C++20 constexpr support ---
#if PARTEST_CPP_VERSION >= 20
    #define PARTEST_CONSTEXPR_20 constexpr
#else
    #define PARTEST_CONSTEXPR_20
#endif

#include <string>
#include <ostream>

// Use std::string_view for C++17 and later, const std::string& for C++11 and C++14
#if PARTEST_CPP_VERSION >= 17
    #include <string_view>
	#define PARTEST_STRING_PARAM std::string_view
	#define PARTEST_STRING_PARAM_TO_STRING(PARTEST_STRING_PARAM) std::string(PARTEST_STRING_PARAM)
#else
	#define PARTEST_STRING_PARAM const std::string&
	#define PARTEST_STRING_PARAM_TO_STRING(PARTEST_STRING_PARAM) (PARTEST_STRING_PARAM)	
#endif

namespace partest
{
	namespace traits
	{
		// Import std::to_string into this namespace for ADL
		using std::to_string;
		// Trait to check if T has a to_string function defined
		template<typename T, typename = void>
		struct has_to_string : std::false_type {};

		// Specialization that does the checking. Checks for std::to_string(T)
		// but also allows for ADL to find a to_string in the same namespace as T
		template<typename T>
		struct has_to_string<T, decltype(void(to_string(std::declval<T>())))>
			: std::true_type {};

		// Trait to check if T can be streamed to an std::ostream
		template<typename T, typename = void>
		struct is_streamable : std::false_type {};

		// Specialization that does the checking
		template<typename T>
		struct is_streamable<T, decltype(void(std::declval<std::ostream&>() << std::declval<T>()))>
			: std::true_type {};
	}

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
		return maybeStringify(static_cast<const char *>(value));
	}

#if PARTEST_CPP_VERSION >= 17
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
		if constexpr(traits::has_to_string<T>::value)
		{
			return std::to_string(value);
		}
		else if constexpr(traits::is_streamable<T>::value)
		{
			std::ostream out;
			out << value;
			return out.rdbuf()->str();
		}
		else
		{
			// Fallback for types that cannot be converted to string
			return std::string("<unprintable type: " + typeid(T).name() + ">");
		}
	}
#else
	/**
	* Convert a value to string if possible
	* Try to use std::to_string if available, otherwise fall back to streaming to std::ostream
	* Single function using if constexpr for C++17 and later
	* 
	* @param value The value to convert to string
	* @return The string representation of the value, or a placeholder if not convertible
	*/
	template <typename T, typename = typename std::enable_if<traits::has_to_string<T>::value>::type>
	std::string maybeStringify(const T& value)
	{
		using std::to_string;
		return to_string(value);
	}

	/**
	* Convert a value to string if possible
	* Try to use std::to_string if available, otherwise fall back to streaming to std::ostream
	* Single function using if constexpr for C++17 and later
	* 
	* @param value The value to convert to string
	* @return The string representation of the value, or a placeholder if not convertible
	*/
	template <typename T, typename = typename std::enable_if<!traits::has_to_string<T>::value && traits::is_streamable<T>::value>::type>
	std::string maybeStringify(const T& value)
	{
		std::ostream out;
		out << value;
		return out.str();
	}

	/**
	* Convert a value to string if possible
	* Try to use std::to_string if available, otherwise fall back to streaming to std::ostream
	* Single function using if constexpr for C++17 and later
	* 
	* @param value The value to convert to string
	* @return The string representation of the value, or a placeholder if not convertible
	*/
	template <typename T, typename = typename std::enable_if<!traits::has_to_string<T>::value && !traits::is_streamable<T>::value>::type>
	std::string maybeStringify(const T& value)
	{
		return std::string("<unprintable type: " + typeid(T).name() + ">");
	}
#endif


#if PARTEST_CPP_VERSION >= 20
// For C++20, include <concepts> and define a `requires` clause macro
	#include <concepts>
	#define PARTEST_ENABLE_IF_INVOCABLE(MaybeInvocable) typename Func> requires std::invocable<MaybeInvocable

#elif PARTEST_CPP_VERSION >= 17
// For C++17, use the standard library trait
	#include <type_traits>
	#define PARTEST_ENABLE_IF_INVOCABLE(MaybeInvocable) typename Func, typename = std::enable_if_t<std::is_invocable_v<MaybeInvocable>>

// For C++11, C++14, define our own trait and use it
#else
	#include <type_traits>
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
			template<typename T>
			static auto check(int) -> decltype(std::declval<T>()(), std::true_type());
			template<typename T>
			static auto check(...) -> std::false_type;
		public:
			// Value is true if MaybeInvocable is callable, false otherwise
			// Passing 0 here prefers the first overload of check if it is valid
			// if MaybeInvocable is not a callable type with zero arguments, the second overload is chosen because it does not match declval<T>()(),
			// which requires T to be callable with no arguments.
			// Thus, it falls back to the ellipsis version, which always returns false_type.
			static constexpr bool value = decltype(check<MaybeInvocable>(0))::value;
		};

		// Helper macro to enable functions only if the provided type is callable
		// If is_callable evaluates to true, the function is enabled and can be instantiated
		// If is_callable evaluates to false, type does not exist, causing a substitution failure
		#define PARTEST_ENABLE_IF_INVOCABLE(MaybeInvocable) typename MaybeInvocable, typename = typename std::enable_if<partest::traits::is_callable<MaybeInvocable>::value>::type
	}
#endif // PARTEST_CPP_VERSION
}

#endif // PARTESTCOMMON_H