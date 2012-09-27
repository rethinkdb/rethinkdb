#include "clustering/administration/issues/pinnings_shards_mismatch.hpp"

#include "clustering/administration/http/json_adapters.hpp"
#include "http/json/json_adapter.hpp"
#include "utils.hpp"

template <class protocol_t>
pinnings_shards_mismatch_issue_t<protocol_t>::pinnings_shards_mismatch_issue_t(
        const namespace_id_t &_offending_namespace,
        const nonoverlapping_regions_t<protocol_t> &_shards,
        const region_map_t<protocol_t, uuid_t> &_primary_pinnings,
        const region_map_t<protocol_t, std::set<uuid_t> > &_secondary_pinnings)
    : offending_namespace(_offending_namespace), shards(_shards),
      primary_pinnings(_primary_pinnings), secondary_pinnings(_secondary_pinnings)
{ }

template <class protocol_t>
std::string pinnings_shards_mismatch_issue_t<protocol_t>::get_description() const {
    //XXX XXX god fuck this. We have to make copies because we don't have
    //constness worked out in json_adapters and just fuck everything.
    nonoverlapping_regions_t<protocol_t> _shards = shards;
    region_map_t<protocol_t, machine_id_t> _primary_pinnings = primary_pinnings;
    region_map_t<protocol_t, std::set<machine_id_t> > _secondary_pinnings = secondary_pinnings;
    return strprintf("The namespace: %s has a pinning map which is segmented differently than its sharding scheme.\n"
                      "Sharding scheme:\n %s\n"
                      "Primary pinnings:\n %s\n"
                      "Secondary pinnings:\n %s\n",
                      uuid_to_str(offending_namespace).c_str(),
                      scoped_cJSON_t(render_as_json(&_shards)).PrintUnformatted().c_str(),
                      scoped_cJSON_t(render_as_json(&_primary_pinnings)).PrintUnformatted().c_str(),
                      scoped_cJSON_t(render_as_json(&_secondary_pinnings)).PrintUnformatted().c_str());
}

template <class protocol_t>
cJSON *pinnings_shards_mismatch_issue_t<protocol_t>::get_json_description() {
    issue_json_t json;
    json.critical = false;
    json.description = get_description();
    json.type = "PINNINGS_SHARDS_MISMATCH";
    json.time = get_secs();
    cJSON *res = render_as_json(&json);

    cJSON_AddItemToObject(res, "offending_namespace", render_as_json(&offending_namespace));
    cJSON_AddItemToObject(res, "shards", render_as_json(&shards));
    cJSON_AddItemToObject(res, "primary_pinnings", render_as_json(&primary_pinnings));
    cJSON_AddItemToObject(res, "secondary_pinnings", render_as_json(&secondary_pinnings));

    return res;
}

template <class protocol_t>
pinnings_shards_mismatch_issue_t<protocol_t> *pinnings_shards_mismatch_issue_t<protocol_t>::clone() const {
    return new pinnings_shards_mismatch_issue_t<protocol_t>(offending_namespace, shards, primary_pinnings, secondary_pinnings);
}

template <class protocol_t>
std::list<clone_ptr_t<global_issue_t> > pinnings_shards_mismatch_issue_tracker_t<protocol_t>::get_issues() {
    std::list<clone_ptr_t<global_issue_t> > res;

    cow_ptr_t<namespaces_semilattice_metadata_t<protocol_t> > namespaces = semilattice_view->get();

    for (typename namespaces_semilattice_metadata_t<protocol_t>::namespace_map_t::const_iterator it  = namespaces->namespaces.begin();
                                                                                                 it != namespaces->namespaces.end();
                                                                                                 ++it) {
        if (it->second.is_deleted()) {
            continue;
        }
        nonoverlapping_regions_t<protocol_t> shards = it->second.get().shards.get();
        region_map_t<protocol_t, machine_id_t> primary_pinnings = it->second.get().primary_pinnings.get();
        region_map_t<protocol_t, std::set<machine_id_t> > secondary_pinnings = it->second.get().secondary_pinnings.get();
        for (typename std::set<typename protocol_t::region_t>::iterator shit = shards.begin();
             shit != shards.end(); ++shit) {
            /* Check primary pinnings for problem. */
            region_map_t<protocol_t, machine_id_t> primary_masked_pinnings = primary_pinnings.mask(*shit);

            machine_id_t primary_expected_val = primary_masked_pinnings.begin()->second;
            for (typename region_map_t<protocol_t, machine_id_t>::iterator pit = primary_masked_pinnings.begin();
                 pit != primary_masked_pinnings.end(); ++pit) {
                if (pit->second != primary_expected_val) {
                    res.push_back(clone_ptr_t<global_issue_t>(new pinnings_shards_mismatch_issue_t<protocol_t>(it->first, shards, primary_pinnings, secondary_pinnings)));
                    goto NAMESPACE_HAS_ISSUE;
                }
            }

            /* Check secondary pinnings for problem. */
            region_map_t<protocol_t, std::set<machine_id_t> > secondary_masked_pinnings = secondary_pinnings.mask(*shit);

            std::set<machine_id_t> secondary_expected_val = secondary_masked_pinnings.begin()->second;
            for (typename region_map_t<protocol_t, std::set<machine_id_t> >::iterator pit  = secondary_masked_pinnings.begin();
                                                                                      pit != secondary_masked_pinnings.end();
                                                                                      ++pit) {
                if (pit->second!= secondary_expected_val) {
                    res.push_back(clone_ptr_t<global_issue_t>(new pinnings_shards_mismatch_issue_t<protocol_t>(it->first, shards, primary_pinnings, secondary_pinnings)));
                    goto NAMESPACE_HAS_ISSUE;
                }
            }
        }
    NAMESPACE_HAS_ISSUE:
        (void)0;
        // do nothing, continue around loop.
    }

    return res;
}


#include "mock/dummy_protocol.hpp"
#include "memcached/protocol.hpp"

template class pinnings_shards_mismatch_issue_t<mock::dummy_protocol_t>;
template class pinnings_shards_mismatch_issue_tracker_t<mock::dummy_protocol_t>;

template class pinnings_shards_mismatch_issue_t<memcached_protocol_t>;
template class pinnings_shards_mismatch_issue_tracker_t<memcached_protocol_t>;
