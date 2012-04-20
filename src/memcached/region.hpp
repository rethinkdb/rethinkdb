#ifndef MEMCACHED_REGION_HPP_
#define MEMCACHED_REGION_HPP_

#include "btree/keys.hpp"
#include "utils.hpp"

std::string key_range_as_string(const key_range_t &);

bool region_is_empty(const key_range_t &r);
bool region_is_superset(const key_range_t &potential_superset, const key_range_t &potential_subset) THROWS_NOTHING;
key_range_t region_intersection(const key_range_t &r1, const key_range_t &r2) THROWS_NOTHING;

MUST_USE region_join_result_t region_join(const std::vector<key_range_t> &vec, key_range_t *out) THROWS_NOTHING;
bool region_overlaps(const key_range_t &r1, const key_range_t &r2) THROWS_NOTHING;
std::vector<key_range_t> region_subtract_many(key_range_t a, const std::vector<key_range_t>& b);

bool operator==(key_range_t, key_range_t) THROWS_NOTHING;
bool operator!=(key_range_t, key_range_t) THROWS_NOTHING;
bool operator<(const key_range_t &, const key_range_t &) THROWS_NOTHING;






#endif  // MEMCACHED_REGION_HPP_
