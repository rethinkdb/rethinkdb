#include "clustering/generic/nonoverlapping_regions.hpp"

#include "protocol_api.hpp"

// TODO: Ugh, this is O(n)!  Oh well.
template <class protocol_t>
bool nonoverlapping_regions_t<protocol_t>::add_region(const typename protocol_t::region_t &region) {
    for (typename std::set<typename protocol_t::region_t>::iterator it = regions_.begin();
         it != regions_.end();
         ++it) {
        if (region_overlaps(region, *it)) {
            return false;
        }
    }

    regions_.insert(region);
    return true;
}


// TODO: See if any json adapter for std::set is being used.

// ctx-less json adapter concept for nonoverlapping_regions_t.
template <class protocol_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(UNUSED nonoverlapping_regions_t<protocol_t> *target) {
    return json_adapter_if_t::json_adapter_map_t();
}

template <class protocol_t>
cJSON *render_as_json(nonoverlapping_regions_t<protocol_t> *target) {
    cJSON *res = cJSON_CreateArray();
    for (typename nonoverlapping_regions_t<protocol_t>::iterator it = target->begin(); it != target->end(); ++it) {
        // TODO: An extra copy.
        typename protocol_t::region_t tmp = *it;
        cJSON_AddItemToArray(res, render_as_json(&tmp));
    }
    return res;
}

template <class protocol_t>
void apply_json_to(cJSON *change, nonoverlapping_regions_t<protocol_t> *target) THROWS_ONLY(schema_mismatch_exc_t) {
    nonoverlapping_regions_t<protocol_t> res;
    json_array_iterator_t it = get_array_it(change);
    for (cJSON *val; (val = it.next()); ) {
        typename protocol_t::region_t region;
        apply_json_to(val, &region);
        if (!res.add_region(region)) {
            throw schema_mismatch_exc_t("Sharding spec has overlapping regions.");
        }
    }

    if (res.size() == 0) {
        throw schema_mismatch_exc_t("Sharding spec has no regions.");
    }

    *target = res;
}

template <class protocol_t>
void on_subfield_change(UNUSED nonoverlapping_regions_t<protocol_t> *target) { }



#include "mock/dummy_protocol.hpp"
#include "mock/dummy_protocol_json_adapter.hpp"
template class nonoverlapping_regions_t<mock::dummy_protocol_t>;
template json_adapter_if_t::json_adapter_map_t get_json_subfields<mock::dummy_protocol_t>(nonoverlapping_regions_t<mock::dummy_protocol_t> *target);
template cJSON *render_as_json<mock::dummy_protocol_t>(nonoverlapping_regions_t<mock::dummy_protocol_t> *target);
template void apply_json_to<mock::dummy_protocol_t>(cJSON *change, nonoverlapping_regions_t<mock::dummy_protocol_t> *target);
template void on_subfield_change<mock::dummy_protocol_t>(nonoverlapping_regions_t<mock::dummy_protocol_t> *target);

#include "memcached/protocol.hpp"
#include "memcached/protocol_json_adapter.hpp"
template class nonoverlapping_regions_t<memcached_protocol_t>;
template json_adapter_if_t::json_adapter_map_t get_json_subfields<memcached_protocol_t>(nonoverlapping_regions_t<memcached_protocol_t> *target);
template cJSON *render_as_json<memcached_protocol_t>(nonoverlapping_regions_t<memcached_protocol_t> *target);
template void apply_json_to<memcached_protocol_t>(cJSON *change, nonoverlapping_regions_t<memcached_protocol_t> *target);
template void on_subfield_change<memcached_protocol_t>(nonoverlapping_regions_t<memcached_protocol_t> *target);

#include "rdb_protocol/protocol.hpp"
template class nonoverlapping_regions_t<rdb_protocol_t>;
template json_adapter_if_t::json_adapter_map_t get_json_subfields<rdb_protocol_t>(nonoverlapping_regions_t<rdb_protocol_t> *target);
template cJSON *render_as_json<rdb_protocol_t>(nonoverlapping_regions_t<rdb_protocol_t> *target);
template void apply_json_to<rdb_protocol_t>(cJSON *change, nonoverlapping_regions_t<rdb_protocol_t> *target);
template void on_subfield_change<rdb_protocol_t>(nonoverlapping_regions_t<rdb_protocol_t> *target);

