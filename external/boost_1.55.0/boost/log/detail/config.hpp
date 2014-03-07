/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   config.hpp
 * \author Andrey Semashev
 * \date   08.03.2007
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html. In this file
 *         internal configuration macros are defined.
 */

#ifndef BOOST_LOG_DETAIL_CONFIG_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_CONFIG_HPP_INCLUDED_

// This check must be before any system headers are included, or __MSVCRT_VERSION__ may get defined to 0x0600
#if defined(__MINGW32__) && !defined(__MSVCRT_VERSION__)
// Target MinGW headers to at least MSVC 7.0 runtime by default. This will enable some useful functions.
#define __MSVCRT_VERSION__ 0x0700
#endif

#include <limits.h> // To bring in libc macros
#include <boost/config.hpp>

#if defined(BOOST_NO_RTTI)
#   error Boost.Log: RTTI is required by the library
#endif

#if !defined(BOOST_WINDOWS)
#   ifndef BOOST_LOG_WITHOUT_DEBUG_OUTPUT
#       define BOOST_LOG_WITHOUT_DEBUG_OUTPUT
#   endif
#   ifndef BOOST_LOG_WITHOUT_EVENT_LOG
#       define BOOST_LOG_WITHOUT_EVENT_LOG
#   endif
#endif

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if defined(BOOST_MSVC)
    // For some reason MSVC 9.0 fails to link the library if static integral constants are defined in cpp
#   define BOOST_LOG_BROKEN_STATIC_CONSTANTS_LINKAGE
#   if _MSC_VER <= 1310
        // MSVC 7.1 sometimes fails to match out-of-class template function definitions with
        // their declarations if the return type or arguments of the functions involve typename keyword
        // and depend on the template parameters.
#       define BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
#   endif
#   if _MSC_VER <= 1400
        // Older MSVC versions reject friend declarations for class template specializations
#       define BOOST_LOG_BROKEN_FRIEND_TEMPLATE_SPECIALIZATIONS
#   endif
#   if _MSC_VER <= 1600
        // MSVC up to 10.0 attempts to invoke copy constructor when initializing a const reference from rvalue returned from a function.
        // This fails when the returned value cannot be copied (only moved):
        //
        // class base {};
        // class derived : public base { BOOST_MOVABLE_BUT_NOT_COPYABLE(derived) };
        // derived foo();
        // base const& var = foo(); // attempts to call copy constructor of derived
#       define BOOST_LOG_BROKEN_REFERENCE_FROM_RVALUE_INIT
#   endif
#   if !defined(_STLPORT_VERSION)
        // MSVC 9.0 mandates packaging of STL classes, which apparently affects alignment and
        // makes alignment_of< T >::value no longer be a power of 2 for types that derive from STL classes.
        // This breaks type_with_alignment and everything that relies on it.
        // This doesn't happen with non-native STLs, such as STLPort. Strangely, this doesn't show with
        // STL classes themselves or most of the user-defined derived classes.
        // Not sure if that happens with other MSVC versions.
        // See: http://svn.boost.org/trac/boost/ticket/1946
#       define BOOST_LOG_BROKEN_STL_ALIGNMENT
#   endif
#endif

#if defined(BOOST_INTEL) || defined(__SUNPRO_CC)
    // Intel compiler and Sun Studio 12.3 have problems with friend declarations for nested class templates
#   define BOOST_LOG_NO_MEMBER_TEMPLATE_FRIENDS
#endif

#if defined(BOOST_MSVC) && BOOST_MSVC <= 1600
    // MSVC cannot interpret constant expressions in certain contexts, such as non-type template parameters
#   define BOOST_LOG_BROKEN_CONSTANT_EXPRESSIONS
#endif

#if defined(__CYGWIN__)
    // Boost.ASIO is broken on Cygwin
#   define BOOST_LOG_NO_ASIO
#endif

#if !defined(BOOST_LOG_USE_NATIVE_SYSLOG) && defined(BOOST_LOG_NO_ASIO)
#   ifndef BOOST_LOG_WITHOUT_SYSLOG
#       define BOOST_LOG_WITHOUT_SYSLOG
#   endif
#endif

#if defined(__GNUC__) && (__GNUC__ == 4 && __GNUC_MINOR__ <= 2)
    // GCC 4.1 and 4.2 have buggy anonymous namespaces support, which interferes with symbol linkage
#   define BOOST_LOG_ANONYMOUS_NAMESPACE namespace anonymous {} using namespace anonymous; namespace anonymous
#else
#   define BOOST_LOG_ANONYMOUS_NAMESPACE namespace
#endif

#if defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) || (defined(__GNUC__) && (__GNUC__ == 4 && __GNUC_MINOR__ <= 6))
// GCC up to 4.6 (inclusively) did not support expanding template argument packs into non-variadic template arguments
#define BOOST_LOG_NO_CXX11_ARG_PACKS_TO_NON_VARIADIC_ARGS_EXPANSION
#endif

#if defined(_MSC_VER)
#   define BOOST_LOG_NO_VTABLE __declspec(novtable)
#elif defined(__GNUC__)
#   define BOOST_LOG_NO_VTABLE
#else
#   define BOOST_LOG_NO_VTABLE
#endif

// An MS-like compilers' extension that allows to optimize away the needless code
#if defined(_MSC_VER)
#   define BOOST_LOG_ASSUME(expr) __assume(expr)
#else
#   define BOOST_LOG_ASSUME(expr)
#endif

// The statement marking unreachable branches of code to avoid warnings
#if defined(BOOST_CLANG)
#   if __has_builtin(__builtin_unreachable)
#       define BOOST_LOG_UNREACHABLE() __builtin_unreachable()
#   endif
#elif defined(__GNUC__)
#   if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5))
#       define BOOST_LOG_UNREACHABLE() __builtin_unreachable()
#   endif
#elif defined(_MSC_VER)
#   define BOOST_LOG_UNREACHABLE() __assume(0)
#endif
#if !defined(BOOST_LOG_UNREACHABLE)
#   define BOOST_LOG_UNREACHABLE()
#endif

// Some compilers support a special attribute that shows that a function won't return
#if defined(__GNUC__) || (defined(__SUNPRO_CC) && __SUNPRO_CC >= 0x590)
    // GCC and Sun Studio 12 support attribute syntax
#   define BOOST_LOG_NORETURN __attribute__((noreturn))
#elif defined (_MSC_VER)
    // Microsoft-compatible compilers go here
#   define BOOST_LOG_NORETURN __declspec(noreturn)
#else
    // The rest compilers might emit bogus warnings about missing return statements
    // in functions with non-void return types when throw_exception is used.
#   define BOOST_LOG_NORETURN
#endif

// cxxabi.h availability macro
#if defined(BOOST_CLANG)
#   if defined(__has_include) && __has_include(<cxxabi.h>)
#       define BOOST_LOG_HAS_CXXABI_H
#   endif
#elif defined(__GNUC__) && !defined(__QNX__)
#   define BOOST_LOG_HAS_CXXABI_H
#endif

#if !defined(BOOST_LOG_BUILDING_THE_LIB)

// Detect if we're dealing with dll
#   if defined(BOOST_LOG_DYN_LINK) || defined(BOOST_ALL_DYN_LINK)
#        define BOOST_LOG_DLL
#   endif

#   if defined(BOOST_LOG_DLL)
#       if defined(BOOST_SYMBOL_IMPORT)
#           define BOOST_LOG_API BOOST_SYMBOL_IMPORT
#       elif defined(BOOST_HAS_DECLSPEC)
#           define BOOST_LOG_API __declspec(dllimport)
#       endif
#   endif
#   ifndef BOOST_LOG_API
#       define BOOST_LOG_API
#   endif
//
// Automatically link to the correct build variant where possible.
//
#   if !defined(BOOST_ALL_NO_LIB)
#       if !defined(BOOST_LOG_NO_LIB)
#          define BOOST_LIB_NAME boost_log
#          if defined(BOOST_LOG_DLL)
#              define BOOST_DYN_LINK
#          endif
#          include <boost/config/auto_link.hpp>
#       endif
        // In static-library builds compilers ignore auto-link comments from Boost.Log binary to
        // other Boost libraries. We explicitly add comments here for other libraries.
        // In dynamic-library builds this is not needed.
#       if !defined(BOOST_LOG_DLL)
#           include <boost/system/config.hpp>
#           include <boost/filesystem/config.hpp>
#           if !defined(BOOST_DATE_TIME_NO_LIB) && !defined(BOOST_DATE_TIME_SOURCE)
#               define BOOST_LIB_NAME boost_date_time
#               if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_DATE_TIME_DYN_LINK)
#                   define BOOST_DYN_LINK
#               endif
#               include <boost/config/auto_link.hpp>
#           endif
            // Boost.Thread's config is included below, if needed
#       endif
#   endif  // auto-linking disabled

#else // !defined(BOOST_LOG_BUILDING_THE_LIB)

#   if defined(BOOST_LOG_DLL)
#       if defined(BOOST_SYMBOL_EXPORT)
#           define BOOST_LOG_API BOOST_SYMBOL_EXPORT
#       elif defined(BOOST_HAS_DECLSPEC)
#           define BOOST_LOG_API __declspec(dllexport)
#       endif
#   endif
#   ifndef BOOST_LOG_API
#       define BOOST_LOG_API BOOST_SYMBOL_VISIBLE
#   endif

#endif // !defined(BOOST_LOG_BUILDING_THE_LIB)

// By default we provide support for both char and wchar_t
#if !defined(BOOST_LOG_WITHOUT_CHAR)
#   define BOOST_LOG_USE_CHAR
#endif
#if !defined(BOOST_LOG_WITHOUT_WCHAR_T)
#   define BOOST_LOG_USE_WCHAR_T
#endif

#if !defined(BOOST_LOG_DOXYGEN_PASS)
    // Check if multithreading is supported
#   if !defined(BOOST_LOG_NO_THREADS) && !defined(BOOST_HAS_THREADS)
#       define BOOST_LOG_NO_THREADS
#   endif // !defined(BOOST_LOG_NO_THREADS) && !defined(BOOST_HAS_THREADS)
#endif // !defined(BOOST_LOG_DOXYGEN_PASS)

#if !defined(BOOST_LOG_NO_THREADS)
    // We need this header to (i) enable auto-linking with Boost.Thread and
    // (ii) to bring in configuration macros of Boost.Thread.
#   include <boost/thread/detail/config.hpp>
#endif // !defined(BOOST_LOG_NO_THREADS)

#if !defined(BOOST_LOG_NO_THREADS)
#   define BOOST_LOG_EXPR_IF_MT(expr) expr
#else
#   undef BOOST_LOG_USE_COMPILER_TLS
#   define BOOST_LOG_EXPR_IF_MT(expr)
#endif // !defined(BOOST_LOG_NO_THREADS)

#if defined(BOOST_LOG_USE_COMPILER_TLS)
#   if defined(__GNUC__) || defined(__SUNPRO_CC)
#       define BOOST_LOG_TLS __thread
#   elif defined(BOOST_MSVC)
#       define BOOST_LOG_TLS __declspec(thread)
#   else
#       undef BOOST_LOG_USE_COMPILER_TLS
#   endif
#endif // defined(BOOST_LOG_USE_COMPILER_TLS)

namespace boost {

// Setup namespace name
#if !defined(BOOST_LOG_DOXYGEN_PASS)
#   if defined(BOOST_LOG_DLL)
#       if defined(BOOST_LOG_NO_THREADS)
#           define BOOST_LOG_VERSION_NAMESPACE v2_st
#       else
#           if defined(BOOST_THREAD_PLATFORM_PTHREAD)
#               define BOOST_LOG_VERSION_NAMESPACE v2_mt_posix
#           elif defined(BOOST_THREAD_PLATFORM_WIN32)
#               if defined(BOOST_LOG_USE_WINNT6_API)
#                   define BOOST_LOG_VERSION_NAMESPACE v2_mt_nt6
#               else
#                   define BOOST_LOG_VERSION_NAMESPACE v2_mt_nt5
#               endif // defined(BOOST_LOG_USE_WINNT6_API)
#           else
#               define BOOST_LOG_VERSION_NAMESPACE v2_mt
#           endif
#       endif // defined(BOOST_LOG_NO_THREADS)
#   else
#       if defined(BOOST_LOG_NO_THREADS)
#           define BOOST_LOG_VERSION_NAMESPACE v2s_st
#       else
#           if defined(BOOST_THREAD_PLATFORM_PTHREAD)
#               define BOOST_LOG_VERSION_NAMESPACE v2s_mt_posix
#           elif defined(BOOST_THREAD_PLATFORM_WIN32)
#               if defined(BOOST_LOG_USE_WINNT6_API)
#                   define BOOST_LOG_VERSION_NAMESPACE v2s_mt_nt6
#               else
#                   define BOOST_LOG_VERSION_NAMESPACE v2s_mt_nt5
#               endif // defined(BOOST_LOG_USE_WINNT6_API)
#           else
#               define BOOST_LOG_VERSION_NAMESPACE v2s_mt
#           endif
#       endif // defined(BOOST_LOG_NO_THREADS)
#   endif // defined(BOOST_LOG_DLL)


namespace log {

#   if !defined(BOOST_NO_CXX11_INLINE_NAMESPACES)

inline namespace BOOST_LOG_VERSION_NAMESPACE {}
}

#       define BOOST_LOG_OPEN_NAMESPACE namespace log { inline namespace BOOST_LOG_VERSION_NAMESPACE {
#       define BOOST_LOG_CLOSE_NAMESPACE }}

#   else

namespace BOOST_LOG_VERSION_NAMESPACE {}

using namespace BOOST_LOG_VERSION_NAMESPACE
#       if defined(__GNUC__) && (__GNUC__ >= 4 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)) && !defined(__clang__)
__attribute__((__strong__))
#       endif
;

}

#       define BOOST_LOG_OPEN_NAMESPACE namespace log { namespace BOOST_LOG_VERSION_NAMESPACE {
#       define BOOST_LOG_CLOSE_NAMESPACE }}
#   endif

#else // !defined(BOOST_LOG_DOXYGEN_PASS)

namespace log {}
#   define BOOST_LOG_OPEN_NAMESPACE namespace log {
#   define BOOST_LOG_CLOSE_NAMESPACE }

#endif // !defined(BOOST_LOG_DOXYGEN_PASS)

} // namespace boost

#endif // BOOST_LOG_DETAIL_CONFIG_HPP_INCLUDED_
