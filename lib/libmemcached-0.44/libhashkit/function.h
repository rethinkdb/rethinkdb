/* HashKit
 * Copyright (C) 2010 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#ifndef HASHKIT_FUNCTION_H
#define HASHKIT_FUNCTION_H

#ifdef __cplusplus
extern "C" {
#endif

/**
  This sets/gets the default function we will be using.
*/
HASHKIT_API
hashkit_return_t hashkit_set_function(hashkit_st *hash, hashkit_hash_algorithm_t hash_algorithm);

HASHKIT_API
hashkit_return_t hashkit_set_custom_function(hashkit_st *hash, hashkit_hash_fn function, void *context);

HASHKIT_API
hashkit_hash_algorithm_t hashkit_get_function(const hashkit_st *hash);

/**
  This sets/gets the function we use for distribution.
*/
HASHKIT_API
hashkit_return_t hashkit_set_distribution_function(hashkit_st *hash, hashkit_hash_algorithm_t hash_algorithm);

HASHKIT_API
hashkit_return_t hashkit_set_custom_distribution_function(hashkit_st *self, hashkit_hash_fn function, void *context);

HASHKIT_API
hashkit_hash_algorithm_t hashkit_get_distribution_function(const hashkit_st *self);

#ifdef __cplusplus
}
#endif

#endif /* HASHKIT_FUNCTION_H */
