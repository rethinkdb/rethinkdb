/*
 * Summary: interface for HashKit functions
 * Description: visibitliy macros for HashKit library
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in this directory for full text.
 * 
 * Author: Monty Taylor
 */

/**
 * @file
 * @brief Visibility control macros
 */

#ifndef HASHKIT_VISIBILITY_H
#define HASHKIT_VISIBILITY_H

/**
 *
 * HASHKIT_API is used for the public API symbols. It either DLL imports or
 * DLL exports (or does nothing for static build).
 *
 * HASHKIT_LOCAL is used for non-api symbols.
 */

#if defined(BUILDING_HASHKIT)
# if defined(HAVE_VISIBILITY) && HAVE_VISIBILITY
#  define HASHKIT_API __attribute__ ((visibility("default")))
#  define HASHKIT_LOCAL  __attribute__ ((visibility("hidden")))
# elif defined (__SUNPRO_C) && (__SUNPRO_C >= 0x550)
#  define HASHKIT_API __global
#  define HASHKIT_LOCAL __hidden
# elif defined(_MSC_VER)
#  define HASHKIT_API extern __declspec(dllexport) 
#  define HASHKIT_LOCAL
# else
#  define HASHKIT_API
#  define HASHKIT_LOCAL
# endif /* defined(HAVE_VISIBILITY) */
#else  /* defined(BUILDING_HASHKIT) */
# if defined(_MSC_VER)
#  define HASHKIT_API extern __declspec(dllimport) 
#  define HASHKIT_LOCAL
# else
#  define HASHKIT_API
#  define HASHKIT_LOCAL
# endif /* defined(_MSC_VER) */
#endif /* defined(BUILDING_HASHKIT) */

#endif /* HASHKIT_VISIBILITY_H */
