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

#ifndef __LIBMEMCACHED_AUTO_H__
#define __LIBMEMCACHED_AUTO_H__

#ifdef __cplusplus
extern "C" {
#endif

LIBMEMCACHED_API
memcached_return_t memcached_increment(memcached_st *ptr,
                                     const char *key, size_t key_length,
                                     uint32_t offset,
                                     uint64_t *value);
LIBMEMCACHED_API
memcached_return_t memcached_decrement(memcached_st *ptr,
                                     const char *key, size_t key_length,
                                     uint32_t offset,
                                     uint64_t *value);

LIBMEMCACHED_API
memcached_return_t memcached_increment_by_key(memcached_st *ptr,
                                            const char *master_key, size_t master_key_length,
                                            const char *key, size_t key_length,
                                            uint64_t offset,
                                            uint64_t *value);

LIBMEMCACHED_API
memcached_return_t memcached_decrement_by_key(memcached_st *ptr,
                                            const char *master_key, size_t master_key_length,
                                            const char *key, size_t key_length,
                                            uint64_t offset,
                                            uint64_t *value);

LIBMEMCACHED_API
memcached_return_t memcached_increment_with_initial(memcached_st *ptr,
                                                  const char *key,
                                                  size_t key_length,
                                                  uint64_t offset,
                                                  uint64_t initial,
                                                  time_t expiration,
                                                  uint64_t *value);
LIBMEMCACHED_API
memcached_return_t memcached_decrement_with_initial(memcached_st *ptr,
                                                  const char *key,
                                                  size_t key_length,
                                                  uint64_t offset,
                                                  uint64_t initial,
                                                  time_t expiration,
                                                  uint64_t *value);
LIBMEMCACHED_API
memcached_return_t memcached_increment_with_initial_by_key(memcached_st *ptr,
                                                         const char *master_key,
                                                         size_t master_key_length,
                                                         const char *key,
                                                         size_t key_length,
                                                         uint64_t offset,
                                                         uint64_t initial,
                                                         time_t expiration,
                                                         uint64_t *value);
LIBMEMCACHED_API
memcached_return_t memcached_decrement_with_initial_by_key(memcached_st *ptr,
                                                         const char *master_key,
                                                         size_t master_key_length,
                                                         const char *key,
                                                         size_t key_length,
                                                         uint64_t offset,
                                                         uint64_t initial,
                                                         time_t expiration,
                                                         uint64_t *value);

#ifdef __cplusplus
}
#endif

#endif /* __LIBMEMCACHED_AUTO_H__ */
