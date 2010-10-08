/* LibMemcached
 * Copyright (C) 2010 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: returns a human readable string for the error message
 *
 */

#ifndef __LIBMEMCACHED_STRERROR_H__
#define __LIBMEMCACHED_STRERROR_H__

#ifdef __cplusplus
extern "C" {
#endif

LIBMEMCACHED_API
const char *memcached_strerror(memcached_st *ptr, memcached_return_t rc);

#ifdef __cplusplus
}
#endif

#endif /* __LIBMEMCACHED_STRERROR_H__ */
