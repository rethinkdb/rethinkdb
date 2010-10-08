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

#include "common.h"

inline static memcached_return_t _string_check(memcached_string_st *string, size_t need)
{
  if (need && need > (size_t)(string->current_size - (size_t)(string->end - string->string)))
  {
    size_t current_offset= (size_t) (string->end - string->string);
    char *new_value;
    size_t adjust;
    size_t new_size;

    /* This is the block multiplier. To keep it larger and surive division errors we must round it up */
    adjust= (need - (size_t)(string->current_size - (size_t)(string->end - string->string))) / MEMCACHED_BLOCK_SIZE;
    adjust++;

    new_size= sizeof(char) * (size_t)((adjust * MEMCACHED_BLOCK_SIZE) + string->current_size);
    /* Test for overflow */
    if (new_size < need)
      return MEMCACHED_MEMORY_ALLOCATION_FAILURE;

    new_value= libmemcached_realloc(string->root, string->string, new_size);

    if (new_value == NULL)
      return MEMCACHED_MEMORY_ALLOCATION_FAILURE;

    string->string= new_value;
    string->end= string->string + current_offset;

    string->current_size+= (MEMCACHED_BLOCK_SIZE * adjust);
  }

  return MEMCACHED_SUCCESS;
}

static inline void _init_string(memcached_string_st *self)
{
  self->current_size= 0;
  self->end= self->string= NULL;
}

memcached_string_st *memcached_string_create(const memcached_st *memc, memcached_string_st *self, size_t initial_size)
{
  memcached_return_t rc;

  WATCHPOINT_ASSERT(memc);

  /* Saving malloc calls :) */
  if (self)
  {
    WATCHPOINT_ASSERT(self->options.is_initialized == false);

    self->options.is_allocated= false;
  }
  else
  {
    self= libmemcached_malloc(memc, sizeof(memcached_string_st));

    if (self == NULL)
    {
      return NULL;
    }

    self->options.is_allocated= true;
  }
  self->root= memc;

  _init_string(self);

  rc=  _string_check(self, initial_size);
  if (rc != MEMCACHED_SUCCESS)
  {
    libmemcached_free(memc, self);
    return NULL;
  }

  self->options.is_initialized= true;

  WATCHPOINT_ASSERT(self->string == self->end);

  return self;
}

memcached_return_t memcached_string_append_character(memcached_string_st *string,
                                                     char character)
{
  memcached_return_t rc;

  rc=  _string_check(string, 1);

  if (rc != MEMCACHED_SUCCESS)
    return rc;

  *string->end= character;
  string->end++;

  return MEMCACHED_SUCCESS;
}

memcached_return_t memcached_string_append(memcached_string_st *string,
                                           const char *value, size_t length)
{
  memcached_return_t rc;

  rc= _string_check(string, length);

  if (rc != MEMCACHED_SUCCESS)
    return rc;

  WATCHPOINT_ASSERT(length <= string->current_size);
  WATCHPOINT_ASSERT(string->string);
  WATCHPOINT_ASSERT(string->end >= string->string);

  memcpy(string->end, value, length);
  string->end+= length;

  return MEMCACHED_SUCCESS;
}

char *memcached_string_c_copy(memcached_string_st *string)
{
  char *c_ptr;

  if (memcached_string_length(string) == 0)
    return NULL;

  c_ptr= libmemcached_malloc(string->root, (memcached_string_length(string)+1) * sizeof(char));

  if (c_ptr == NULL)
    return NULL;

  memcpy(c_ptr, memcached_string_value(string), memcached_string_length(string));
  c_ptr[memcached_string_length(string)]= 0;

  return c_ptr;
}

memcached_return_t memcached_string_reset(memcached_string_st *string)
{
  string->end= string->string;

  return MEMCACHED_SUCCESS;
}

void memcached_string_free(memcached_string_st *ptr)
{
  if (ptr == NULL)
    return;

  if (ptr->string)
  {
    libmemcached_free(ptr->root, ptr->string);
  }

  if (memcached_is_allocated(ptr))
  {
    libmemcached_free(ptr->root, ptr);
  }
  else
  {
    ptr->options.is_initialized= false;
  }
}

memcached_return_t memcached_string_check(memcached_string_st *string, size_t need)
{
  return _string_check(string, need);
}

size_t memcached_string_length(const memcached_string_st *self)
{
  return (size_t)(self->end - self->string);
}

size_t memcached_string_size(const memcached_string_st *self)
{
  return self->current_size;
}

const char *memcached_string_value(const memcached_string_st *self)
{
  return self->string;
}

char *memcached_string_value_mutable(const memcached_string_st *self)
{
  return self->string;
}

void memcached_string_set_length(memcached_string_st *self, size_t length)
{
  self->end= self->string + length;
}
