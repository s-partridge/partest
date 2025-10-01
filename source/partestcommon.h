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

namespace partest
{
// For C++20, include <concepts> and define a `requires` clause macro
#if PARTEST_CPP_VERSION >= 20
#include <concepts>
#define PARTEST_ENABLE_IF_INVOCABLE(MaybeInvocable) typename Func> requires std::invocable<MaybeInvocable

// For C++17, use the standard library trait
#elif PARTEST_CPP_VERSION >= 17
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