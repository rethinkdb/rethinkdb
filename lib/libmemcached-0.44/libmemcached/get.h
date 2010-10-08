/* LibMemcached
 * Copyright (C) 2006-2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: Get functions for libmemcached
 *
 */

#ifndef __LIBMEMCACHED_GET_H__
#define __LIBMEMCACHED_GET_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Public defines */
LIBMEMCACHED_API
char *memcached_get(memcached_st *ptr,
                    const char *key, size_t key_length,
                    size_t *value_length,
                    uint32_t *flags,
                    memcached_return_t *error);

LIBMEMCACHED_API
memcached_return_t memcached_mget(memcached_st *ptr,
                                  const char * const *keys,
                                  const size_t *key_length,
                                  size_t number_of_keys);

LIBMEMCACHED_API
char *memcached_get_by_key(memcached_st *ptr,
                           const char *master_key, size_t master_key_length,
                           const char *key, size_t key_length,
                           size_t *value_length,
                           uint32_t *flags,
                           memcached_return_t *error);

LIBMEMCACHED_API
memcached_return_t memcached_mget_by_key(memcached_st *ptr,
                                         const char *master_key,
                                         size_t master_key_length,
                                         const char * const *keys,
                                         const size_t *key_length,
                                         const size_t number_of_keys);

LIBMEMCACHED_API
char *memcached_fetch(memcached_st *ptr,
                      char *key,
                      size_t *key_length,
                      size_t *value_length,
                      uint32_t *flags,
                      memcached_return_t *error);

LIBMEMCACHED_API
memcached_result_st *memcached_fetch_result(memcached_st *ptr,
                                            memcached_result_st *result,
                                            memcached_return_t *error);

LIBMEMCACHED_API
memcached_return_t memcached_mget_execute(memcached_st *ptr,
                                          const char * const *keys,
                                          const size_t *key_length,
                                          const size_t number_of_keys,
                                          memcached_execute_fn *callback,
                                          void *context,
                                          const uint32_t number_of_callbacks);

LIBMEMCACHED_API
memcached_return_t memcached_mget_execute_by_key(memcached_st *ptr,
                                                 const char *master_key,
                                                 size_t master_key_length,
                                                 const char * const *keys,
                                                 const size_t *key_length,
                                                 size_t number_of_keys,
                                                 memcached_execute_fn *callback,
                                                 void *context,
                                                 const uint32_t number_of_callbacks);

#ifdef __cplusplus
}
#endif

#endif /* __LIBMEMCACHED_GET_H__ */
