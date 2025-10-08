#ifndef PARTESTCOMMON_H
#define PARTESTCOMMON_H

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

/**
* String parameter type by C++ standard version
*/
#include <string>

// Use std::string_view for C++17 and later, const std::string& for C++11 and C++14
#if PARTEST_CPP_VERSION >= 17
    #include <string_view>
	#define PARTEST_STRING_PARAM std::string_view
#else
	#define PARTEST_STRING_PARAM const std::string&
#endif

namespace partest
{
// For C++20, include <concepts> and define a `requires` clause macro
#if PARTEST_CPP_VERSION >= 20
#include <concepts>
#define PARTEST_ENABLE_IF_INVOCABLE(MaybeInvocable) typename Func> requires std::invocable<MaybeInvocable

// For C++17, use the standard library trait
#elif PARTEST_CPP_VERSION >= 17
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