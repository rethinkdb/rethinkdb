/* HashKit
 * Copyright (C) 2006-2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include "common.h"

uint32_t libhashkit_one_at_a_time(const char *key, size_t key_length)
{
  return hashkit_one_at_a_time(key, key_length, NULL);
}

uint32_t libhashkit_fnv1_64(const char *key, size_t key_length)
{
  return hashkit_fnv1_64(key, key_length, NULL);
}

uint32_t libhashkit_fnv1a_64(const char *key, size_t key_length)
{
  return hashkit_fnv1a_64(key, key_length, NULL);
}

uint32_t libhashkit_fnv1_32(const char *key, size_t key_length)
{
  return hashkit_fnv1_32(key, key_length, NULL);
}

uint32_t libhashkit_fnv1a_32(const char *key, size_t key_length)
{
  return hashkit_fnv1a_32(key, key_length, NULL);
}

uint32_t libhashkit_crc32(const char *key, size_t key_length)
{
  return hashkit_crc32(key, key_length, NULL);
}

#ifdef HAVE_HSIEH_HASH
uint32_t libhashkit_hsieh(const char *key, size_t key_length)
{
  return hashkit_hsieh(key, key_length, NULL);
}
#endif

uint32_t libhashkit_murmur(const char *key, size_t key_length)
{
  return hashkit_murmur(key, key_length, NULL);
}

uint32_t libhashkit_jenkins(const char *key, size_t key_length)
{
  return hashkit_jenkins(key, key_length, NULL);
}

uint32_t libhashkit_md5(const char *key, size_t key_length)
{
  return hashkit_md5(key, key_length, NULL);
}

void libhashkit_md5_signature(const unsigned char *key, size_t length, unsigned char *result)
{
  md5_signature(key, (uint32_t)length, result);
}

