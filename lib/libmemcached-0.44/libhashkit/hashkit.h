/* HashKit
 * Copyright (C) 2009-2010 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#ifndef HASHKIT_H
#define HASHKIT_H


#if !defined(__cplusplus)
# include <stdbool.h>
#endif
#include <inttypes.h>
#include <sys/types.h>
#include <libhashkit/visibility.h>
#include <libhashkit/configure.h>
#include <libhashkit/types.h>
#include <libhashkit/algorithm.h>
#include <libhashkit/behavior.h>
#include <libhashkit/digest.h>
#include <libhashkit/function.h>
#include <libhashkit/strerror.h>

#ifdef __cplusplus

#include <string>

extern "C" {
#endif

HASHKIT_API
hashkit_st *hashkit_create(hashkit_st *hash);

HASHKIT_API
hashkit_st *hashkit_clone(hashkit_st *destination, const hashkit_st *ptr);

HASHKIT_API
bool hashkit_compare(const hashkit_st *first, const hashkit_st *second);

HASHKIT_API
void hashkit_free(hashkit_st *hash);

#define hashkit_is_allocated(__object) ((__object)->options.is_allocated)
#define hashkit_is_initialized(__object) ((__object)->options.is_initialized)

#ifdef __cplusplus
} // extern "C"
#endif

struct hashkit_st
{
  struct hashkit_function_st {
    hashkit_hash_fn function;
    void *context;
  } base_hash, distribution_hash;

  struct {
    bool is_base_same_distributed:1;
  } flags;

  struct {
    bool is_allocated:1;
  } options;
};

#ifdef __cplusplus
class Hashkit : private hashkit_st {

public:

  Hashkit()
  {
    hashkit_create(this);
  }

  Hashkit(const Hashkit& source)
  {
    hashkit_clone(this, &source);
  }

  Hashkit& operator=(const Hashkit& source)
  {
    hashkit_free(this);
    hashkit_clone(this, &source);

    return *this;
  }

  friend bool operator==(const Hashkit &left, const Hashkit &right)
  {
    return hashkit_compare(&left, &right);
  }

  uint32_t digest(std::string& str)
  {
    return hashkit_digest(this, str.c_str(), str.length());
  }

  uint32_t digest(const char *key, size_t key_length)
  {
    return hashkit_digest(this, key, key_length);
  }

  hashkit_return_t set_function(hashkit_hash_algorithm_t hash_algorithm)
  {
    return hashkit_set_function(this, hash_algorithm);
  }

  hashkit_return_t set_distribution_function(hashkit_hash_algorithm_t hash_algorithm)
  {
    return hashkit_set_function(this, hash_algorithm);
  }

  ~Hashkit()
  {
    hashkit_free(this);
  }
};
#endif


#endif /* HASHKIT_H */
