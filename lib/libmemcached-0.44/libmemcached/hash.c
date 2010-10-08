/* LibMemcached
 * Copyright (C) 2006-2010 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: 
 *
 */

#include "common.h"


uint32_t memcached_generate_hash_value(const char *key, size_t key_length, memcached_hash_t hash_algorithm)
{
  return libhashkit_digest(key, key_length, (hashkit_hash_algorithm_t)hash_algorithm);
}

static inline uint32_t generate_hash(const memcached_st *ptr, const char *key, size_t key_length)
{
  return hashkit_digest(&ptr->hashkit, key, key_length);
}

static uint32_t dispatch_host(const memcached_st *ptr, uint32_t hash)
{
  switch (ptr->distribution)
  {
  case MEMCACHED_DISTRIBUTION_CONSISTENT:
  case MEMCACHED_DISTRIBUTION_CONSISTENT_KETAMA:
  case MEMCACHED_DISTRIBUTION_CONSISTENT_KETAMA_SPY:
    {
      uint32_t num= ptr->continuum_points_counter;
      WATCHPOINT_ASSERT(ptr->continuum);

      hash= hash;
      memcached_continuum_item_st *begin, *end, *left, *right, *middle;
      begin= left= ptr->continuum;
      end= right= ptr->continuum + num;

      while (left < right)
      {
        middle= left + (right - left) / 2;
        if (middle->value < hash)
          left= middle + 1;
        else
          right= middle;
      }
      if (right == end)
        right= begin;
      return right->index;
    }
  case MEMCACHED_DISTRIBUTION_MODULA:
    return hash % memcached_server_count(ptr);
  case MEMCACHED_DISTRIBUTION_RANDOM:
    return (uint32_t) random() % memcached_server_count(ptr);
  case MEMCACHED_DISTRIBUTION_CONSISTENT_MAX:
  default:
    WATCHPOINT_ASSERT(0); /* We have added a distribution without extending the logic */
    return hash % memcached_server_count(ptr);
  }
  /* NOTREACHED */
}

/*
  One version is public and will not modify the distribution hash, the other will.
*/
static inline uint32_t _generate_hash_wrapper(const memcached_st *ptr, const char *key, size_t key_length)
{
  WATCHPOINT_ASSERT(memcached_server_count(ptr));

  if (memcached_server_count(ptr) == 1)
    return 0;

  if (ptr->flags.hash_with_prefix_key)
  {
    size_t temp_length= ptr->prefix_key_length + key_length;
    char temp[temp_length];

    if (temp_length > MEMCACHED_MAX_KEY -1)
      return 0;

    strncpy(temp, ptr->prefix_key, ptr->prefix_key_length);
    strncpy(temp + ptr->prefix_key_length, key, key_length);

    return generate_hash(ptr, temp, temp_length);
  }
  else
  {
    return generate_hash(ptr, key, key_length);
  }
}

static inline void _regen_for_auto_eject(memcached_st *ptr)
{
  if (_is_auto_eject_host(ptr) && ptr->next_distribution_rebuild)
  {
    struct timeval now;

    if (gettimeofday(&now, NULL) == 0 &&
        now.tv_sec > ptr->next_distribution_rebuild)
    {
      run_distribution(ptr);
    }
  }
}

void memcached_autoeject(memcached_st *ptr)
{
  _regen_for_auto_eject(ptr);
}

uint32_t memcached_generate_hash_with_redistribution(memcached_st *ptr, const char *key, size_t key_length)
{
  uint32_t hash= _generate_hash_wrapper(ptr, key, key_length);

  _regen_for_auto_eject(ptr);

  return dispatch_host(ptr, hash);
}

uint32_t memcached_generate_hash(const memcached_st *ptr, const char *key, size_t key_length)
{
  return dispatch_host(ptr, _generate_hash_wrapper(ptr, key, key_length));
}

const hashkit_st *memcached_get_hashkit(const memcached_st *ptr)
{
  return &ptr->hashkit;
}

memcached_return_t memcached_set_hashkit(memcached_st *self, hashkit_st *hashk)
{
  hashkit_free(&self->hashkit);
  hashkit_clone(&self->hashkit, hashk);

  return MEMCACHED_SUCCESS;
}
