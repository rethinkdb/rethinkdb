/* LibMemcached
 * Copyright (C) 2010 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: hash interface code
 *
 */

#ifndef __MEMCACHED_HASH_H__
#define __MEMCACHED_HASH_H__

#ifdef __cplusplus
extern "C" {
#endif

/* The two public hash bits */
LIBMEMCACHED_API
uint32_t memcached_generate_hash_value(const char *key, size_t key_length, memcached_hash_t hash_algorithm);

LIBMEMCACHED_API
const hashkit_st *memcached_get_hashkit(const memcached_st *ptr);

LIBMEMCACHED_API
memcached_return_t memcached_set_hashkit(memcached_st *ptr, hashkit_st *hashk);

LIBMEMCACHED_API
uint32_t memcached_generate_hash(const memcached_st *ptr, const char *key, size_t key_length);

LIBMEMCACHED_LOCAL
uint32_t memcached_generate_hash_with_redistribution(memcached_st *ptr, const char *key, size_t key_length);

LIBMEMCACHED_API
void memcached_autoeject(memcached_st *ptr);

#ifdef __cplusplus
}
#endif

#endif /* __MEMCACHED_HASH_H__ */
