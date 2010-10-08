/* LibMemcached
 * Copyright (C) 2006-2009 Brian Aker 
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: Interface for memcached server.
 *
 * Author: Trond Norbye
 *
 */

/**
 * @file
 * @brief Visibility control macros
 */

#ifndef __LIBMEMCACHED_VISIBILITY_H__
#define __LIBMEMCACHED_VISIBILITY_H__

/**
 *
 * LIBMEMCACHED_API is used for the public API symbols. It either DLL imports or
 * DLL exports (or does nothing for static build).
 *
 * LIBMEMCACHED_LOCAL is used for non-api symbols.
 */

#if defined(BUILDING_LIBMEMCACHED)
# if defined(HAVE_VISIBILITY) && HAVE_VISIBILITY
#  define LIBMEMCACHED_API __attribute__ ((visibility("default")))
#  define LIBMEMCACHED_LOCAL  __attribute__ ((visibility("hidden")))
# elif defined (__SUNPRO_C) && (__SUNPRO_C >= 0x550)
#  define LIBMEMCACHED_API __global
#  define LIBMEMCACHED_LOCAL __hidden
# elif defined(_MSC_VER)
#  define LIBMEMCACHED_API extern __declspec(dllexport) 
#  define LIBMEMCACHED_LOCAL
# else
#  define LIBMEMCACHED_API
#  define LIBMEMCACHED_LOCAL
# endif /* defined(HAVE_VISIBILITY) */
#else  /* defined(BUILDING_LIBMEMCACHED) */
# if defined(_MSC_VER)
#  define LIBMEMCACHED_API extern __declspec(dllimport) 
#  define LIBMEMCACHED_LOCAL
# else
#  define LIBMEMCACHED_API
#  define LIBMEMCACHED_LOCAL
# endif /* defined(_MSC_VER) */
#endif /* defined(BUILDING_LIBMEMCACHED) */

#endif /* __LIBMEMCACHED_VISIBILITY_H__ */
