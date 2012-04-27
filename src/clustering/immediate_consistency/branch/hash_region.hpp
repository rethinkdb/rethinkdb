#ifndef HASH_REGION_HPP_
#define HASH_REGION_HPP_

#include <stdint.h>

#include "utils.hpp"

// Returns a value in [0, UINT64_MAX / 2].
uint64_t hash_region_hasher(const uint8_t *s, size_t len);



#endif  // HASH_REGION_HPP_
