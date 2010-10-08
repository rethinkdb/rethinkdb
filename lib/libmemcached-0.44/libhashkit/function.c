/* HashKit
 * Copyright (C) 2010 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include "common.h"

static hashkit_return_t _set_function(struct hashkit_function_st *self, hashkit_hash_algorithm_t hash_algorithm)
{
  switch (hash_algorithm)
  {
  case HASHKIT_HASH_DEFAULT:
    self->function= hashkit_one_at_a_time;
    break;
  case HASHKIT_HASH_MD5:
    self->function= hashkit_md5;
    break;
  case HASHKIT_HASH_CRC:
    self->function= hashkit_crc32;
    break;
  case HASHKIT_HASH_FNV1_64:
    self->function= hashkit_fnv1_64;
    break;
  case HASHKIT_HASH_FNV1A_64:
    self->function= hashkit_fnv1a_64;
    break;
  case HASHKIT_HASH_FNV1_32:
    self->function= hashkit_fnv1_32;
    break;
  case HASHKIT_HASH_FNV1A_32:
    self->function= hashkit_fnv1a_32;
    break;
  case HASHKIT_HASH_HSIEH:
#ifdef HAVE_HSIEH_HASH
    self->function= hashkit_hsieh;
    break;    
#else
    return HASHKIT_FAILURE;
#endif
  case HASHKIT_HASH_MURMUR:
    self->function= hashkit_murmur;
    break;    
  case HASHKIT_HASH_JENKINS:
    self->function= hashkit_jenkins;
    break;    
  case HASHKIT_HASH_CUSTOM:
  case HASHKIT_HASH_MAX:
  default:
    return HASHKIT_FAILURE;
  }

  self->context= NULL;

  return HASHKIT_SUCCESS;
}

hashkit_return_t hashkit_set_function(hashkit_st *self, hashkit_hash_algorithm_t hash_algorithm)
{
  return _set_function(&self->base_hash, hash_algorithm);
}

hashkit_return_t hashkit_set_distribution_function(hashkit_st *self, hashkit_hash_algorithm_t hash_algorithm)
{
  return _set_function(&self->distribution_hash, hash_algorithm);
}

static hashkit_return_t _set_custom_function(struct hashkit_function_st *self, hashkit_hash_fn function, void *context)
{
  if (function)
  {
    self->function= function;
    self->context= context;

    return HASHKIT_SUCCESS;
  }

  return HASHKIT_FAILURE;
}

hashkit_return_t hashkit_set_custom_function(hashkit_st *self, hashkit_hash_fn function, void *context)
{
  return _set_custom_function(&self->base_hash, function, context);
}

hashkit_return_t hashkit_set_custom_distribution_function(hashkit_st *self, hashkit_hash_fn function, void *context)
{
  return _set_custom_function(&self->distribution_hash, function, context);
}

static hashkit_hash_algorithm_t get_function_type(const hashkit_hash_fn function)
{
  if (function == hashkit_one_at_a_time)
  {
    return HASHKIT_HASH_DEFAULT;
  }
  else if (function == hashkit_md5)
  {
    return HASHKIT_HASH_MD5;
  }
  else if (function == hashkit_crc32)
  {
    return HASHKIT_HASH_CRC;
  }
  else if (function == hashkit_fnv1_64)
  {
    return HASHKIT_HASH_FNV1_64;
  }
  else if (function == hashkit_fnv1a_64)
  {
    return HASHKIT_HASH_FNV1A_64;
  }
  else if (function == hashkit_fnv1_32)
  {
    return HASHKIT_HASH_FNV1_32;
  }
  else if (function == hashkit_fnv1a_32)
  {
    return HASHKIT_HASH_FNV1A_32;
  }
#ifdef HAVE_HSIEH_HASH
  else if (function == hashkit_hsieh)
  {
    return HASHKIT_HASH_HSIEH;
  }
#endif
  else if (function == hashkit_murmur)
  {
    return HASHKIT_HASH_MURMUR;
  }
  else if (function == hashkit_jenkins)
  {
    return HASHKIT_HASH_JENKINS;
  }

  return HASHKIT_HASH_CUSTOM;
}

hashkit_hash_algorithm_t hashkit_get_function(const hashkit_st *self)
{
  return get_function_type(self->base_hash.function);
}

hashkit_hash_algorithm_t hashkit_get_distribution_function(const hashkit_st *self)
{
  return get_function_type(self->distribution_hash.function);
}
