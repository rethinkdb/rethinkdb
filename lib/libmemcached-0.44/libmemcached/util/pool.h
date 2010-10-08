/* LibMemcached
 * Copyright (C) 2006-2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: Connection pool implementation for libmemcached.
 *
 */


#ifndef __LIMEMCACHED_UTIL_POOL_H__
#define __LIMEMCACHED_UTIL_POOL_H__

#include <libmemcached/memcached.h>

#ifdef __cplusplus
extern "C" {
#endif

struct memcached_pool_st;
typedef struct memcached_pool_st memcached_pool_st;

LIBMEMCACHED_API
memcached_pool_st *memcached_pool_create(memcached_st* mmc, uint32_t initial,
                                         uint32_t max);
LIBMEMCACHED_API
memcached_st* memcached_pool_destroy(memcached_pool_st* pool);
LIBMEMCACHED_API
memcached_st* memcached_pool_pop(memcached_pool_st* pool,
                                 bool block,
                                 memcached_return_t* rc);
LIBMEMCACHED_API
memcached_return_t memcached_pool_push(memcached_pool_st* pool,
                                       memcached_st* mmc);

LIBMEMCACHED_API
memcached_return_t memcached_pool_behavior_set(memcached_pool_st *ptr,
                                               memcached_behavior_t flag,
                                               uint64_t data);
LIBMEMCACHED_API
memcached_return_t memcached_pool_behavior_get(memcached_pool_st *ptr,
                                               memcached_behavior_t flag,
                                               uint64_t *value);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __LIMEMCACHED_UTIL_POOL_H__ */
