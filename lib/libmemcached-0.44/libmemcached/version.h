/* LibMemcached
 * Copyright (C) 2010 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: Find version information
 *
 */

#ifndef __LIBMEMCACHED_VERSION_H__
#define __LIBMEMCACHED_VERSION_H__

#ifdef __cplusplus
extern "C" {
#endif

LIBMEMCACHED_API
memcached_return_t memcached_version(memcached_st *ptr);

LIBMEMCACHED_API
const char * memcached_lib_version(void);

#ifdef __cplusplus
}
#endif

#endif /* __LIBMEMCACHED_VERSION_H__ */
