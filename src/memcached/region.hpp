#ifndef MEMCACHED_REGION_HPP_
#define MEMCACHED_REGION_HPP_

#include <string>
#include <vector>

#include "btree/keys.hpp"
#include "utils.hpp"

/* The protocol API specifies that the following standalone functions must be
defined for `protocol_t::region_t`. Most of them are thin wrappers around
existing methods or functions with slightly different names. */

inline bool region_is_empty(const key_range_t &r) {
    return r.is_empty();
}

inline bool region_is_superset(const key_range_t &potential_superset, const key_range_t &potential_subset) THROWS_NOTHING {
    return potential_superset.is_superset(potential_subset);
}

inline key_range_t region_intersection(const key_range_t &r1, const key_range_t &r2) THROWS_NOTHING {
    return r1.intersection(r2);
}

inline bool region_overlaps(const key_range_t &r1, const key_range_t &r2) THROWS_NOTHING {
    return r1.overlaps(r2);
}

MUST_USE region_join_result_t region_join(const std::vector<key_range_t> &vec, key_range_t *out) THROWS_NOTHING;

std::vector<key_range_t> region_subtract_many(key_range_t a, const std::vector<key_range_t>& b);

inline std::string region_to_debug_str(const key_range_t &r) {
    return key_range_to_debug_str(r);
}

#endif  // MEMCACHED_REGION_HPP_
