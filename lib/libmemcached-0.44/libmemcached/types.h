/* LibMemcached
 * Copyright (C) 2006-2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: Types for libmemcached
 *
 */

#ifndef __LIBMEMCACHED_TYPES_H__
#define __LIBMEMCACHED_TYPES_H__

typedef struct memcached_st memcached_st;
typedef struct memcached_stat_st memcached_stat_st;
typedef struct memcached_analysis_st memcached_analysis_st;
typedef struct memcached_result_st memcached_result_st;

// All of the flavors of memcache_server_st
typedef struct memcached_server_st memcached_server_st;
typedef const struct memcached_server_st *memcached_server_instance_st;
typedef struct memcached_server_st *memcached_server_list_st;

typedef struct memcached_callback_st memcached_callback_st;

// The following two structures are internal, and never exposed to users.
typedef struct memcached_string_st memcached_string_st;
typedef struct memcached_continuum_item_st memcached_continuum_item_st;


#ifdef __cplusplus
extern "C" {
#endif

typedef memcached_return_t (*memcached_clone_fn)(memcached_st *destination, const memcached_st *source);
typedef memcached_return_t (*memcached_cleanup_fn)(const memcached_st *ptr);

/**
  Memory allocation functions.
*/
typedef void (*memcached_free_fn)(const memcached_st *ptr, void *mem, void *context);
typedef void *(*memcached_malloc_fn)(const memcached_st *ptr, const size_t size, void *context);
typedef void *(*memcached_realloc_fn)(const memcached_st *ptr, void *mem, const size_t size, void *context);
typedef void *(*memcached_calloc_fn)(const memcached_st *ptr, size_t nelem, const size_t elsize, void *context);


typedef memcached_return_t (*memcached_execute_fn)(const memcached_st *ptr, memcached_result_st *result, void *context);
typedef memcached_return_t (*memcached_server_fn)(const memcached_st *ptr, memcached_server_instance_st server, void *context);
typedef memcached_return_t (*memcached_stat_fn)(memcached_server_instance_st server,
                                                const char *key, size_t key_length,
                                                const char *value, size_t value_length,
                                                void *context);

/**
  Trigger functions.
*/
typedef memcached_return_t (*memcached_trigger_key_fn)(const memcached_st *ptr,
                                                       const char *key, size_t key_length,
                                                       memcached_result_st *result);
typedef memcached_return_t (*memcached_trigger_delete_key_fn)(const memcached_st *ptr,
                                                              const char *key, size_t key_length);

typedef memcached_return_t (*memcached_dump_fn)(const memcached_st *ptr,
                                                const char *key,
                                                size_t key_length,
                                                void *context);

#ifdef __cplusplus
}
#endif

/**
  @note The following definitions are just here for backwards compatibility.
*/
typedef memcached_return_t memcached_return;
typedef memcached_server_distribution_t memcached_server_distribution;
typedef memcached_behavior_t memcached_behavior;
typedef memcached_callback_t memcached_callback;
typedef memcached_hash_t memcached_hash;
typedef memcached_connection_t memcached_connection;
typedef memcached_clone_fn memcached_clone_func;
typedef memcached_cleanup_fn memcached_cleanup_func;
typedef memcached_execute_fn memcached_execute_function;
typedef memcached_server_fn memcached_server_function;
typedef memcached_trigger_key_fn memcached_trigger_key;
typedef memcached_trigger_delete_key_fn memcached_trigger_delete_key;
typedef memcached_dump_fn memcached_dump_func;

#endif /* __LIBMEMCACHED_TYPES_H__ */
