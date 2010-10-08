
/* HashKit
 * Copyright (C) 2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#ifndef HASHKIT_TYPES_H
#define HASHKIT_TYPES_H

#ifdef __cplusplus

extern "C" {
#endif

typedef enum {
  HASHKIT_SUCCESS,
  HASHKIT_FAILURE,
  HASHKIT_MEMORY_ALLOCATION_FAILURE,
  HASHKIT_MAXIMUM_RETURN /* Always add new error code before */
} hashkit_return_t;

typedef enum {
  HASHKIT_HASH_DEFAULT= 0, // hashkit_one_at_a_time()
  HASHKIT_HASH_MD5,
  HASHKIT_HASH_CRC,
  HASHKIT_HASH_FNV1_64,
  HASHKIT_HASH_FNV1A_64,
  HASHKIT_HASH_FNV1_32,
  HASHKIT_HASH_FNV1A_32,
  HASHKIT_HASH_HSIEH,
  HASHKIT_HASH_MURMUR,
  HASHKIT_HASH_JENKINS,
  HASHKIT_HASH_CUSTOM,
  HASHKIT_HASH_MAX
} hashkit_hash_algorithm_t;

/**
 * Hash distributions that are available to use.
 */
typedef enum
{
  HASHKIT_DISTRIBUTION_MODULA,
  HASHKIT_DISTRIBUTION_RANDOM,
  HASHKIT_DISTRIBUTION_KETAMA,
  HASHKIT_DISTRIBUTION_MAX /* Always add new values before this. */
} hashkit_distribution_t;


typedef struct hashkit_st hashkit_st;

typedef uint32_t (*hashkit_hash_fn)(const char *key, size_t key_length, void *context);

#ifdef __cplusplus
}
#endif

#endif /* HASHKIT_TYPES_H */
