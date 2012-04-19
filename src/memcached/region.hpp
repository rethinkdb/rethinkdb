#ifndef MEMCACHED_REGION_HPP_
#define MEMCACHED_REGION_HPP_

#include "btree/keys.hpp"
#include "protocol_api.hpp"

std::string key_range_as_string(const key_range_t &);

bool region_is_superset(const key_range_t &potential_superset, const key_range_t &potential_subset) THROWS_NOTHING;
key_range_t region_intersection(const key_range_t &r1, const key_range_t &r2) THROWS_NOTHING;
key_range_t region_join(const std::vector<key_range_t> &vec) THROWS_ONLY(bad_join_exc_t, bad_region_exc_t);
bool region_overlaps(const key_range_t &r1, const key_range_t &r2) THROWS_NOTHING;
std::vector<key_range_t> region_subtract_many(key_range_t a, const std::vector<key_range_t>& b);

bool operator==(key_range_t, key_range_t) THROWS_NOTHING;
bool operator!=(key_range_t, key_range_t) THROWS_NOTHING;
bool operator<(const key_range_t &, const key_range_t &) THROWS_NOTHING;






#endif  // MEMCACHED_REGION_HPP_
