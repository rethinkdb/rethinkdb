/* HashKit
 * Copyright (C) 2010 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#ifndef HASHKIT_DIGEST_H
#define HASHKIT_DIGEST_H

#ifdef __cplusplus
extern "C" {
#endif

HASHKIT_API
uint32_t hashkit_digest(const hashkit_st *self, const char *key, size_t key_length);

/**
  This is a utilitly function provided so that you can directly access hashes with a hashkit_st.
*/

HASHKIT_API
uint32_t libhashkit_digest(const char *key, size_t key_length, hashkit_hash_algorithm_t hash_algorithm);

#ifdef __cplusplus
}
#endif

#endif /* HASHKIT_DIGEST_H */
