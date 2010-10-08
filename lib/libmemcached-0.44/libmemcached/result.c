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

/*
  memcached_result_st are used to internally represent the return values from
  memcached. We use a structure so that long term as identifiers are added
  to memcached we will be able to absorb new attributes without having
  to addjust the entire API.
*/
#include "common.h"

static inline void _result_init(memcached_result_st *self,
                                const memcached_st *memc)
{
  self->item_flags= 0;
  self->item_expiration= 0;
  self->key_length= 0;
  self->item_cas= 0;
  self->root= memc;
  self->item_key[0]= 0;
}

memcached_result_st *memcached_result_create(const memcached_st *memc,
                                             memcached_result_st *ptr)
{
  WATCHPOINT_ASSERT(memc);

  /* Saving malloc calls :) */
  if (ptr)
  {
    ptr->options.is_allocated= false;
  }
  else
  {
    ptr= libmemcached_malloc(memc, sizeof(memcached_result_st));

    if (ptr == NULL)
      return NULL;

    ptr->options.is_allocated= true;
  }

  ptr->options.is_initialized= true;

  _result_init(ptr, memc);

  ptr->root= memc;
  WATCHPOINT_SET(ptr->value.options.is_initialized= false);
  memcached_string_create(memc, &ptr->value, 0);
  WATCHPOINT_ASSERT_INITIALIZED(&ptr->value);
  WATCHPOINT_ASSERT(ptr->value.string == NULL);

  return ptr;
}

void memcached_result_reset(memcached_result_st *ptr)
{
  ptr->key_length= 0;
  memcached_string_reset(&ptr->value);
  ptr->item_flags= 0;
  ptr->item_cas= 0;
  ptr->item_expiration= 0;
}

void memcached_result_free(memcached_result_st *ptr)
{
  if (ptr == NULL)
    return;

  memcached_string_free(&ptr->value);

  if (memcached_is_allocated(ptr))
  {
    WATCHPOINT_ASSERT(ptr->root); // Without a root, that means that result was not properly initialized.
    libmemcached_free(ptr->root, ptr);
  }
  else
  {
    ptr->options.is_initialized= false;
  }
}

memcached_return_t memcached_result_set_value(memcached_result_st *ptr,
                                              const char *value,
                                              size_t length)
{
  return memcached_string_append(&ptr->value, value, length);
}

const char *memcached_result_key_value(const memcached_result_st *self)
{
  return self->key_length ? self->item_key : NULL;
}

size_t memcached_result_key_length(const memcached_result_st *self)
{
  return self->key_length;
}

const char *memcached_result_value(const memcached_result_st *self)
{
  const memcached_string_st *sptr= &self->value;
  return memcached_string_value(sptr);
}

size_t memcached_result_length(const memcached_result_st *self)
{
  const memcached_string_st *sptr= &self->value;
  return memcached_string_length(sptr);
}

uint32_t memcached_result_flags(const memcached_result_st *self)
{
  return self->item_flags;
}

uint64_t memcached_result_cas(const memcached_result_st *self)
{
  return self->item_cas;
}

void memcached_result_set_flags(memcached_result_st *self, uint32_t flags)
{
  self->item_flags= flags;
}

void memcached_result_set_expiration(memcached_result_st *self, time_t expiration)
{
  self->item_expiration= expiration;
}
