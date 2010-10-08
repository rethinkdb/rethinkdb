/* LibMemcached
 * Copyright (C) 2006-2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: Functions to manipulate the result structure.
 *
 */

#ifndef __LIBMEMCACHED_RESULT_H__
#define __LIBMEMCACHED_RESULT_H__

struct memcached_result_st {
  uint32_t item_flags;
  time_t item_expiration;
  size_t key_length;
  uint64_t item_cas;
  const memcached_st *root;
  memcached_string_st value;
  char item_key[MEMCACHED_MAX_KEY];
  struct {
    bool is_allocated:1;
    bool is_initialized:1;
  } options;
  /* Add result callback function */
};

#ifdef __cplusplus
extern "C" {
#endif

/* Result Struct */
LIBMEMCACHED_API
void memcached_result_free(memcached_result_st *result);

LIBMEMCACHED_API
void memcached_result_reset(memcached_result_st *ptr);

LIBMEMCACHED_API
memcached_result_st *memcached_result_create(const memcached_st *ptr,
                                             memcached_result_st *result);

LIBMEMCACHED_API
const char *memcached_result_key_value(const memcached_result_st *self);

LIBMEMCACHED_API
size_t memcached_result_key_length(const memcached_result_st *self);

LIBMEMCACHED_API
const char *memcached_result_value(const memcached_result_st *self);

LIBMEMCACHED_API
size_t memcached_result_length(const memcached_result_st *self);

LIBMEMCACHED_API
uint32_t memcached_result_flags(const memcached_result_st *self);

LIBMEMCACHED_API
uint64_t memcached_result_cas(const memcached_result_st *self);

LIBMEMCACHED_API
memcached_return_t memcached_result_set_value(memcached_result_st *ptr, const char *value, size_t length);

LIBMEMCACHED_API
void memcached_result_set_flags(memcached_result_st *self, uint32_t flags);

LIBMEMCACHED_API
void memcached_result_set_expiration(memcached_result_st *self, time_t expiration);

#ifdef __cplusplus
} // extern "C"
#endif


#endif /* __LIBMEMCACHED_RESULT_H__ */
