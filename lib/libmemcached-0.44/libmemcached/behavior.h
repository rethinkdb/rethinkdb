/* LibMemcached
 * Copyright (C) 2006-2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: Change the behavior of the memcached connection.
 *
 */

#ifndef __LIBMEMCACHED_BEHAVIOR_H__
#define __LIBMEMCACHED_BEHAVIOR_H__

#ifdef __cplusplus
extern "C" {
#endif

LIBMEMCACHED_API
memcached_return_t memcached_behavior_set(memcached_st *ptr, const memcached_behavior_t flag, uint64_t data);

LIBMEMCACHED_API
uint64_t memcached_behavior_get(memcached_st *ptr, const memcached_behavior_t flag);

LIBMEMCACHED_API
memcached_return_t memcached_behavior_set_distribution(memcached_st *ptr, memcached_server_distribution_t type);

LIBMEMCACHED_API
memcached_server_distribution_t memcached_behavior_get_distribution(memcached_st *ptr);

LIBMEMCACHED_API
memcached_return_t memcached_behavior_set_key_hash(memcached_st *ptr, memcached_hash_t type);

LIBMEMCACHED_API
memcached_hash_t memcached_behavior_get_key_hash(memcached_st *ptr);

LIBMEMCACHED_API
memcached_return_t memcached_behavior_set_distribution_hash(memcached_st *ptr, memcached_hash_t type);

LIBMEMCACHED_API
memcached_hash_t memcached_behavior_get_distribution_hash(memcached_st *ptr);

LIBMEMCACHED_LOCAL
bool _is_auto_eject_host(const memcached_st *ptr);


#ifdef __cplusplus
}
#endif

#endif /* __LIBMEMCACHED_BEHAVIOR_H__ */
