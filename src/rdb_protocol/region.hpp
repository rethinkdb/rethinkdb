// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef MEMCACHED_PROTOCOL_JSON_ADAPTER_HPP_
#define MEMCACHED_PROTOCOL_JSON_ADAPTER_HPP_

#include <string>

#include "btree/keys.hpp"
#include "http/json/json_adapter.hpp"

struct cJSON;
struct store_key_t;
template <class> class hash_region_t;
struct key_range_t;

// json adapter concept for store_key_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(store_key_t *);
cJSON *render_as_json(store_key_t *);
void apply_json_to(cJSON *, store_key_t *);
void  on_subfield_change(store_key_t *);
std::string to_string_for_json_key(const store_key_t *);


// json adapter concept for key_range_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(key_range_t *target);

// this is not part of the json adapter concept.
std::string render_region_as_string(key_range_t *target);

cJSON *render_as_json(key_range_t *target);

void apply_json_to(cJSON *change, key_range_t *target);

void on_subfield_change(key_range_t *target);



// json adapter concept for hash_region_t<key_range_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(hash_region_t<key_range_t> *);
cJSON *render_as_json(hash_region_t<key_range_t> *);
void apply_json_to(cJSON *, hash_region_t<key_range_t> *);
void  on_subfield_change(hash_region_t<key_range_t> *);
std::string render_region_as_string(hash_region_t<key_range_t> *target);
std::string to_string_for_json_key(hash_region_t<key_range_t> *target);




/* The protocol API specifies that the following standalone functions must be
defined for `protocol_t::region_t`. Most of them are thin wrappers around
existing methods or functions with slightly different names. */

// RSISAM: Deinline these?
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





#endif /* MEMCACHED_PROTOCOL_JSON_ADAPTER_HPP_ */
