/* HashKit
 * Copyright (C) 2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief HashKit Header
 */

#ifndef HASHKIT_ALGORITHM_H
#define HASHKIT_ALGORITHM_H

#ifdef __cplusplus
extern "C" {
#endif

HASHKIT_API
uint32_t libhashkit_one_at_a_time(const char *key, size_t key_length);

HASHKIT_API
uint32_t libhashkit_fnv1_64(const char *key, size_t key_length);

HASHKIT_API
uint32_t libhashkit_fnv1a_64(const char *key, size_t key_length);

HASHKIT_API
uint32_t libhashkit_fnv1_32(const char *key, size_t key_length);

HASHKIT_API
uint32_t libhashkit_fnv1a_32(const char *key, size_t key_length);

HASHKIT_API
uint32_t libhashkit_crc32(const char *key, size_t key_length);

#ifdef HAVE_HSIEH_HASH
HASHKIT_API
uint32_t libhashkit_hsieh(const char *key, size_t key_length);
#endif

HASHKIT_API
uint32_t libhashkit_murmur(const char *key, size_t key_length);

HASHKIT_API
uint32_t libhashkit_jenkins(const char *key, size_t key_length);

HASHKIT_API
uint32_t libhashkit_md5(const char *key, size_t key_length);

HASHKIT_LOCAL
uint32_t hashkit_one_at_a_time(const char *key, size_t key_length, void *context);

HASHKIT_LOCAL
uint32_t hashkit_fnv1_64(const char *key, size_t key_length, void *context);

HASHKIT_LOCAL
uint32_t hashkit_fnv1a_64(const char *key, size_t key_length, void *context);

HASHKIT_LOCAL
uint32_t hashkit_fnv1_32(const char *key, size_t key_length, void *context);

HASHKIT_LOCAL
uint32_t hashkit_fnv1a_32(const char *key, size_t key_length, void *context);

HASHKIT_LOCAL
uint32_t hashkit_crc32(const char *key, size_t key_length, void *context);

#ifdef HAVE_HSIEH_HASH
HASHKIT_LOCAL
uint32_t hashkit_hsieh(const char *key, size_t key_length, void *context);
#endif

HASHKIT_LOCAL
uint32_t hashkit_murmur(const char *key, size_t key_length, void *context);

HASHKIT_LOCAL
uint32_t hashkit_jenkins(const char *key, size_t key_length, void *context);

HASHKIT_LOCAL
uint32_t hashkit_md5(const char *key, size_t key_length, void *context);

HASHKIT_API
void libhashkit_md5_signature(const unsigned char *key, size_t length, unsigned char *result);

#ifdef __cplusplus
}
#endif

#endif /* HASHKIT_ALGORITHM_H */
