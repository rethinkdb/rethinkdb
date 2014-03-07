/*
 ******************************************************************************
 *
 *   Copyright (C) 1997-2010, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 ******************************************************************************
 *
 *  FILE NAME : platform.h
 *
 *   Date        Name        Description
 *   05/13/98    nos         Creation (content moved here from ptypes.h).
 *   03/02/99    stephen     Added AS400 support.
 *   03/30/99    stephen     Added Linux support.
 *   04/13/99    stephen     Reworked for autoconf.
 ******************************************************************************
 */

 /**
  * \file
  * \brief Configuration constants for the Windows platform
  */
  
/** Define the platform we're on. */
#ifndef U_WINDOWS
#define U_WINDOWS
#endif

#if _MSC_VER >= 1700
#include <stdint.h>
#endif

#if defined(__BORLANDC__)
#define U_HAVE_PLACEMENT_NEW 0
#define __STDC_CONSTANT_MACROS
#endif

/** _MSC_VER is used to detect the Microsoft compiler. */
#if defined(_MSC_VER)
#define U_INT64_IS_LONG_LONG 0
#else
#define U_INT64_IS_LONG_LONG 1
#endif

/** Define whether inttypes.h is available */
#ifndef U_HAVE_INTTYPES_H
#   if defined(__BORLANDC__) || defined(__MINGW32__)
#       define U_HAVE_INTTYPES_H 1
#   else
#       define U_HAVE_INTTYPES_H 0
#   endif
#endif

/**
 * Define what support for C++ streams is available.
 *     If U_IOSTREAM_SOURCE is set to 199711, then &lt;iostream&gt; is available
 * (1997711 is the date the ISO/IEC C++ FDIS was published), and then
 * one should qualify streams using the std namespace in ICU header
 * files.
 *     If U_IOSTREAM_SOURCE is set to 198506, then &lt;iostream.h&gt; is
 * available instead (198506 is the date when Stroustrup published
 * "An Extensible I/O Facility for C++" at the summer USENIX conference).
 *     If U_IOSTREAM_SOURCE is 0, then C++ streams are not available and
 * support for them will be silently suppressed in ICU.
 *
 */

#ifndef U_IOSTREAM_SOURCE
#define U_IOSTREAM_SOURCE 199711
#endif

/** @{
 * Determines whether specific types are available */
#ifndef U_HAVE_INT8_T
#define U_HAVE_INT8_T U_HAVE_INTTYPES_H
#endif

#ifndef U_HAVE_UINT8_T
#define U_HAVE_UINT8_T U_HAVE_INTTYPES_H
#endif

#ifndef U_HAVE_INT16_T
#define U_HAVE_INT16_T U_HAVE_INTTYPES_H
#endif

#ifndef U_HAVE_UINT16_T
#define U_HAVE_UINT16_T U_HAVE_INTTYPES_H
#endif

#ifndef U_HAVE_INT32_T
#define U_HAVE_INT32_T U_HAVE_INTTYPES_H
#endif

#ifndef U_HAVE_UINT32_T
#define U_HAVE_UINT32_T U_HAVE_INTTYPES_H
#endif

#ifndef U_HAVE_INT64_T
#define U_HAVE_INT64_T U_HAVE_INTTYPES_H
#endif

#ifndef U_HAVE_UINT64_T
#define U_HAVE_UINT64_T U_HAVE_INTTYPES_H
#endif

/** @} */

/** Define 64 bit limits */
#if !U_INT64_IS_LONG_LONG
# ifndef INT64_C
#  define INT64_C(x) ((int64_t)x)
# endif
# ifndef UINT64_C
#  define UINT64_C(x) ((uint64_t)x)
# endif
/** else use the umachine.h definition */
#endif

/*===========================================================================*/
/** @{
 * Generic data types                                                        */
/*===========================================================================*/

/** If your platform does not have the <inttypes.h> header, you may
   need to edit the typedefs below. */
#if U_HAVE_INTTYPES_H
#include <inttypes.h>
#else /* U_HAVE_INTTYPES_H */

#if ! U_HAVE_INT8_T
typedef signed char int8_t;
#endif

#if ! U_HAVE_UINT8_T
typedef unsigned char uint8_t;
#endif

#if ! U_HAVE_INT16_T
typedef signed short int16_t;
#endif

#if ! U_HAVE_UINT16_T
typedef unsigned short uint16_t;
#endif

#if ! U_HAVE_INT32_T
typedef signed int int32_t;
#endif

#if ! U_HAVE_UINT32_T
typedef unsigned int uint32_t;
#endif

#if ! U_HAVE_INT64_T
#if U_INT64_IS_LONG_LONG
    typedef signed long long int64_t;
#else
    typedef signed __int64 int64_t;
#endif
#endif

#if ! U_HAVE_UINT64_T
#if U_INT64_IS_LONG_LONG
    typedef unsigned long long uint64_t;
#else
    typedef unsigned __int64 uint64_t;
#endif
#endif
#endif

/**
 * @}
 */

/*===========================================================================*/
/** Compiler and environment features                                         */
/*===========================================================================*/

/** Define whether namespace is supported */
#ifndef U_HAVE_NAMESPACE
#define U_HAVE_NAMESPACE 1
#endif

/** Determines the endianness of the platform */
#define U_IS_BIG_ENDIAN 0

/** 1 or 0 to enable or disable threads.  If undefined, default is: enable threads. */
#ifndef ICU_USE_THREADS
#define ICU_USE_THREADS 1
#endif

/** 0 or 1 to enable or disable auto cleanup of libraries. If undefined, default is: disabled. */
#ifndef UCLN_NO_AUTO_CLEANUP
#define UCLN_NO_AUTO_CLEANUP 1
#endif

/* On strong memory model CPUs (e.g. x86 CPUs), we use a safe & quick double check mutex lock. */
/**
Microsoft can define _M_IX86, _M_AMD64 (before Visual Studio 8) or _M_X64 (starting in Visual Studio 8). 
Intel can define _M_IX86 or _M_X64
*/
#if defined(_M_IX86) || defined(_M_AMD64) || defined(_M_X64) || (defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__)))
#define UMTX_STRONG_MEMORY_MODEL 1
#endif

/** Enable or disable debugging options **/
#ifndef U_DEBUG
#ifdef _DEBUG
#define U_DEBUG 1
#else
#define U_DEBUG 0
#endif
#endif

/** Enable or disable release options **/
#ifndef U_RELEASE
#ifdef NDEBUG
#define U_RELEASE 1
#else
#define U_RELEASE 0
#endif
#endif

/** Determine whether to disable renaming or not. This overrides the
   setting in umachine.h which is for all platforms. */
#ifndef U_DISABLE_RENAMING
#define U_DISABLE_RENAMING 0
#endif

/** Determine whether to override new and delete. */
#ifndef U_OVERRIDE_CXX_ALLOCATION
#define U_OVERRIDE_CXX_ALLOCATION 1
#endif
/** Determine whether to override placement new and delete for STL. */
#ifndef U_HAVE_PLACEMENT_NEW
#define U_HAVE_PLACEMENT_NEW 1
#endif
/** Determine whether to override new and delete for MFC. */
#if !defined(U_HAVE_DEBUG_LOCATION_NEW) && defined(_MSC_VER)
#define U_HAVE_DEBUG_LOCATION_NEW 1
#endif

/** Determine whether to enable tracing. */
#ifndef U_ENABLE_TRACING
#define U_ENABLE_TRACING 0
#endif

/** Do we allow ICU users to use the draft APIs by default? */
#ifndef U_DEFAULT_SHOW_DRAFT
#define U_DEFAULT_SHOW_DRAFT 1
#endif

/** @{ Define the library suffix in a C syntax. */
#ifndef U_HAVE_LIB_SUFFIX
#define U_HAVE_LIB_SUFFIX 0
#endif
#ifndef U_LIB_SUFFIX_C_NAME
#define U_LIB_SUFFIX_C_NAME
#endif
#ifndef U_LIB_SUFFIX_C_NAME_STRING
#define U_LIB_SUFFIX_C_NAME_STRING ""
#endif
/** @} */

/*===========================================================================*/
/** @{ Information about wchar support                                           */
/*===========================================================================*/

#define U_HAVE_WCHAR_H 1
#define U_SIZEOF_WCHAR_T 2

#define U_HAVE_WCSCPY 1

/** @} */

/**
 * \def U_DECLARE_UTF16
 * Do not use this macro. Use the UNICODE_STRING or U_STRING_DECL macros
 * instead.
 * @internal
 */
#if 1
#define U_DECLARE_UTF16(string) L ## string
#endif

/*===========================================================================*/
/** @{ Information about POSIX support                                           */
/*===========================================================================*/

/**
 * @internal 
 */
#if 1
#define U_TZSET         _tzset
#endif
/**
 * @internal 
 */
#if 1
#define U_TIMEZONE      _timezone
#endif
/**
 * @internal 
 */
#if 1
#define U_TZNAME        _tzname
#endif
/**
 * @internal 
 */
#if 1
#define U_DAYLIGHT      _daylight
#endif

#define U_HAVE_MMAP 0
#define U_HAVE_POPEN 0

#ifndef U_ENABLE_DYLOAD
#define U_ENABLE_DYLOAD 1
#endif


/** @} */

/*===========================================================================*/
/** @{ Symbol import-export control                                              */
/*===========================================================================*/

#ifdef U_STATIC_IMPLEMENTATION
#define U_EXPORT
#else
#define U_EXPORT __declspec(dllexport)
#endif
#define U_EXPORT2 __cdecl
#define U_IMPORT __declspec(dllimport)
/** @} */

/*===========================================================================*/
/** @{ Code alignment and C function inlining                                    */
/*===========================================================================*/

#ifndef U_INLINE
#   ifdef __cplusplus
#       define U_INLINE inline
#   else
#       define U_INLINE __inline
#   endif
#endif

#if defined(_MSC_VER) && defined(_M_IX86) && !defined(_MANAGED)
#define U_ALIGN_CODE(val)    __asm      align val
#else
#define U_ALIGN_CODE(val)
#endif

/**
 * Flag for workaround of MSVC 2003 optimization bugs
 */
#if defined(_MSC_VER) && (_MSC_VER < 1400)
#define U_HAVE_MSVC_2003_OR_EARLIER
#endif


/** @} */

/*===========================================================================*/
/** @{ Programs used by ICU code                                                 */
/*===========================================================================*/

#ifndef U_MAKE
#define U_MAKE  "nmake"
#define U_MAKE_IS_NMAKE 1
#endif

/** @} */

