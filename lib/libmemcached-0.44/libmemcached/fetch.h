/* LibMemcached
 * Copyright (C) 2010 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: Work with fetching results
 *
 */

#ifndef __LIBMEMCACHED_FETCH_H__
#define __LIBMEMCACHED_FETCH_H__

#ifdef __cplusplus
extern "C" {
#endif

LIBMEMCACHED_API
memcached_return_t memcached_fetch_execute(memcached_st *ptr,
                                           memcached_execute_fn *callback,
                                           void *context,
                                           uint32_t number_of_callbacks);

#ifdef __cplusplus
}
#endif

#endif /* __LIBMEMCACHED_FETCH_H__ */
