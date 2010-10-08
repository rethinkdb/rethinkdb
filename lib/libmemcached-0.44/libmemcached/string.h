/* LibMemcached
 * Copyright (C) 2006-2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: String structure used for libmemcached.
 *
 */

#ifndef __LIBMEMCACHED_STRING_H__
#define __LIBMEMCACHED_STRING_H__

/**
  Strings are always under our control so we make some assumptions
  about them.

  1) is_initialized is always valid.
  2) A string once intialized will always be, until free where we
     unset this flag.
  3) A string always has a root.
*/

struct memcached_string_st {
  char *end;
  char *string;
  size_t current_size;
  const memcached_st *root;
  struct {
    bool is_allocated:1;
    bool is_initialized:1;
  } options;
};

#ifdef __cplusplus
extern "C" {
#endif

LIBMEMCACHED_LOCAL
memcached_string_st *memcached_string_create(const memcached_st *ptr,
                                             memcached_string_st *string,
                                             size_t initial_size);
LIBMEMCACHED_LOCAL
memcached_return_t memcached_string_check(memcached_string_st *string, size_t need);

LIBMEMCACHED_LOCAL
char *memcached_string_c_copy(memcached_string_st *string);

LIBMEMCACHED_LOCAL
memcached_return_t memcached_string_append_character(memcached_string_st *string,
                                                     char character);
LIBMEMCACHED_LOCAL
memcached_return_t memcached_string_append(memcached_string_st *string,
                                           const char *value, size_t length);
LIBMEMCACHED_LOCAL
memcached_return_t memcached_string_reset(memcached_string_st *string);

LIBMEMCACHED_LOCAL
void memcached_string_free(memcached_string_st *string);

LIBMEMCACHED_LOCAL
size_t memcached_string_length(const memcached_string_st *self);

LIBMEMCACHED_LOCAL
size_t memcached_string_size(const memcached_string_st *self);

LIBMEMCACHED_LOCAL
const char *memcached_string_value(const memcached_string_st *self);

LIBMEMCACHED_LOCAL
char *memcached_string_value_mutable(const memcached_string_st *self);

LIBMEMCACHED_LOCAL
void memcached_string_set_length(memcached_string_st *self, size_t length);

#ifdef __cplusplus
}
#endif


#endif /* __LIBMEMCACHED_STRING_H__ */
