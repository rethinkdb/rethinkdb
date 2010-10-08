/* LibMemcached
 * Copyright (C) 2006-2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: Storage related functions, aka set, replace,..
 *
 */

#ifndef __LIBMEMCACHED_STORAGE_H__
#define __LIBMEMCACHED_STORAGE_H__

#include "libmemcached/memcached.h"

#ifdef __cplusplus
extern "C" {
#endif

/* All of the functions for adding data to the server */
LIBMEMCACHED_API
memcached_return_t memcached_set(memcached_st *ptr, const char *key, size_t key_length,
                                 const char *value, size_t value_length,
                                 time_t expiration,
                                 uint32_t  flags);
LIBMEMCACHED_API
memcached_return_t memcached_add(memcached_st *ptr, const char *key, size_t key_length,
                                 const char *value, size_t value_length,
                                 time_t expiration,
                                 uint32_t  flags);
LIBMEMCACHED_API
memcached_return_t memcached_replace(memcached_st *ptr, const char *key, size_t key_length,
                                     const char *value, size_t value_length,
                                     time_t expiration,
                                     uint32_t  flags);
LIBMEMCACHED_API
memcached_return_t memcached_append(memcached_st *ptr,
                                    const char *key, size_t key_length,
                                    const char *value, size_t value_length,
                                    time_t expiration,
                                    uint32_t flags);
LIBMEMCACHED_API
memcached_return_t memcached_prepend(memcached_st *ptr,
                                     const char *key, size_t key_length,
                                     const char *value, size_t value_length,
                                     time_t expiration,
                                     uint32_t flags);
LIBMEMCACHED_API
memcached_return_t memcached_cas(memcached_st *ptr,
                                 const char *key, size_t key_length,
                                 const char *value, size_t value_length,
                                 time_t expiration,
                                 uint32_t flags,
                                 uint64_t cas);

LIBMEMCACHED_API
memcached_return_t memcached_set_by_key(memcached_st *ptr,
                                        const char *master_key, size_t master_key_length,
                                        const char *key, size_t key_length,
                                        const char *value, size_t value_length,
                                        time_t expiration,
                                        uint32_t flags);

LIBMEMCACHED_API
memcached_return_t memcached_add_by_key(memcached_st *ptr,
                                        const char *master_key, size_t master_key_length,
                                        const char *key, size_t key_length,
                                        const char *value, size_t value_length,
                                        time_t expiration,
                                        uint32_t flags);

LIBMEMCACHED_API
memcached_return_t memcached_replace_by_key(memcached_st *ptr,
                                            const char *master_key, size_t master_key_length,
                                            const char *key, size_t key_length,
                                            const char *value, size_t value_length,
                                            time_t expiration,
                                            uint32_t flags);

LIBMEMCACHED_API
memcached_return_t memcached_prepend_by_key(memcached_st *ptr,
                                            const char *master_key, size_t master_key_length,
                                            const char *key, size_t key_length,
                                            const char *value, size_t value_length,
                                            time_t expiration,
                                            uint32_t flags);

LIBMEMCACHED_API
memcached_return_t memcached_append_by_key(memcached_st *ptr,
                                           const char *master_key, size_t master_key_length,
                                           const char *key, size_t key_length,
                                           const char *value, size_t value_length,
                                           time_t expiration,
                                           uint32_t flags);

LIBMEMCACHED_API
memcached_return_t memcached_cas_by_key(memcached_st *ptr,
                                        const char *master_key, size_t master_key_length,
                                        const char *key, size_t key_length,
                                        const char *value, size_t value_length,
                                        time_t expiration,
                                        uint32_t flags,
                                        uint64_t cas);

#ifdef __cplusplus
}
#endif

#endif /* __LIBMEMCACHED_STORAGE_H__ */
