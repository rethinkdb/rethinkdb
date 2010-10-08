/* HashKit
 * Copyright (C) 2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include "common.h"

/* FNV hash'es lifted from Dustin Sallings work */
static uint64_t FNV_64_INIT= UINT64_C(0xcbf29ce484222325);
static uint64_t FNV_64_PRIME= UINT64_C(0x100000001b3);
static uint32_t FNV_32_INIT= 2166136261UL;
static uint32_t FNV_32_PRIME= 16777619;

uint32_t hashkit_fnv1_64(const char *key, size_t key_length, void *context __attribute__((unused)))
{
  /* Thanks to pierre@demartines.com for the pointer */
  uint64_t hash= FNV_64_INIT;

  for (size_t x= 0; x < key_length; x++)
  {
    hash *= FNV_64_PRIME;
    hash ^= (uint64_t)key[x];
  }

  return (uint32_t)hash;
}

uint32_t hashkit_fnv1a_64(const char *key, size_t key_length, void *context __attribute__((unused)))
{
  uint32_t hash= (uint32_t) FNV_64_INIT;

  for (size_t x= 0; x < key_length; x++)
  {
    uint32_t val= (uint32_t)key[x];
    hash ^= val;
    hash *= (uint32_t) FNV_64_PRIME;
  }

  return hash;
}

uint32_t hashkit_fnv1_32(const char *key, size_t key_length, void *context __attribute__((unused)))
{
  uint32_t hash= FNV_32_INIT;

  for (size_t x= 0; x < key_length; x++)
  {
    uint32_t val= (uint32_t)key[x];
    hash *= FNV_32_PRIME;
    hash ^= val;
  }

  return hash;
}

uint32_t hashkit_fnv1a_32(const char *key, size_t key_length, void *context __attribute__((unused)))
{
  uint32_t hash= FNV_32_INIT;

  for (size_t x= 0; x < key_length; x++)
  {
    uint32_t val= (uint32_t)key[x];
    hash ^= val;
    hash *= FNV_32_PRIME;
  }

  return hash;
}
