// File: compat.h
// Author: Samuel Partridge
//  
// Macros related to compatibility across Windows/Posix and C++ language standards
// Defines:
// PARTEST_CPP_VERSION - Value macro that reports the active C++ standard
// PARTEST_CONSTEXPR_11/14/17/20 - resolves to `constexpr` only from the specified language standard onward
// PARTEST_INLINE_VAR_17 - resolves to `inline` only from C++17 onward, for use with inline constexpr variables

#ifndef PARTEST_COMPAT_H
#define PARTEST_COMPAT_H

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
		// Default to 11 (Should only be possible with very old versions of MSVC, but just in case)
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

// --- C++20 constexpr support ---
#if PARTEST_CPP_VERSION >= 20
    #define PARTEST_CONSTEXPR_20 constexpr
#else
    #define PARTEST_CONSTEXPR_20
#endif

// --- C++17 constexpr support ---
#if PARTEST_CPP_VERSION >= 17
    #define PARTEST_CONSTEXPR_17 constexpr
#else
    #define PARTEST_CONSTEXPR_17
#endif

// --- C++14 constexpr support ---
#if PARTEST_CPP_VERSION >= 14
    #define PARTEST_CONSTEXPR_14 constexpr
#else
    #define PARTEST_CONSTEXPR_14
#endif

// --- C++11 constexpr support ---
#if PARTEST_CPP_VERSION >= 11
    #define PARTEST_CONSTEXPR_11 constexpr
#else
    #define PARTEST_CONSTEXPR_11
#endif

// --- C++17 inline variable support
#if PARTEST_CPP_VERSION >= 17
    #define PARTEST_INLINE_VAR_17 inline
#else
    #define PARTEST_INLINE_VAR_17
#endif

#endif