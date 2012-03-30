#ifndef __CLUSTERING_ADMINISTRATION_ISSUES_PINNINGS_SHARDS_MISMTACH_TCC__
#define __CLUSTERING_ADMINISTRATION_ISSUES_PINNINGS_SHARDS_MISMTACH_TCC__

#include "clustering/administration/http/json_adapters.hpp"
#include "clustering/administration/issues/pinnings_shards_mismatch.hpp"
#include "http/json/json_adapter.hpp"
#include "utils.hpp"

template <class protocol_t>
pinnings_shards_mismatch_issue_t<protocol_t>::pinnings_shards_mismatch_issue_t(
        const namespace_id_t &_offending_namespace,
        const std::set<typename protocol_t::region_t> &_shards,
        const region_map_t<protocol_t, std::set<boost::uuids::uuid> > &_pinnings) 
    : offending_namespace(_offending_namespace), shards(_shards), pinnings(_pinnings)
{ }

template <class protocol_t>
std::string pinnings_shards_mismatch_issue_t<protocol_t>::get_description() const {
    //XXX XXX god fuck this. We have to make copies because we don't have
    //constness worked out in json_adapters and just fuck everything.
    std::set<typename protocol_t::region_t> _shards = shards;
    region_map_t<protocol_t, std::set<machine_id_t> > _pinnings = pinnings;
    return strprintf("The namespace: %s has a pinning map which is segmented differently than its sharding scheme.\n"
                      "Sharding scheme:\n %s\n"
                      "Pinnings:\n %s\n", 
                      uuid_to_str(offending_namespace).c_str(),
                      cJSON_print_std_string(scoped_cJSON_t(render_as_json(&_shards, 0)).get()).c_str(),
                      cJSON_print_std_string(scoped_cJSON_t(render_as_json(&_pinnings, 0)).get()).c_str());
}

template <class protocol_t>
cJSON *pinnings_shards_mismatch_issue_t<protocol_t>::get_json_description() {
    issue_json_t json;
    json.critical = false;
    json.description = get_description();
    json.type.issue_type = PINNINGS_SHARDS_MISMATCH;
    json.time = get_ticks();
    cJSON *res = render_as_json(&json, 0);

    cJSON_AddItemToObject(res, "offending_namespace", render_as_json(&offending_namespace, 0));
    cJSON_AddItemToObject(res, "shards", render_as_json(&shards, 0));
    cJSON_AddItemToObject(res, "pinnings", render_as_json(&pinnings, 0));

    return res;
}

template <class protocol_t>
pinnings_shards_mismatch_issue_t<protocol_t> *pinnings_shards_mismatch_issue_t<protocol_t>::clone() const {
    return new pinnings_shards_mismatch_issue_t<protocol_t>(offending_namespace, shards, pinnings);
}

template <class protocol_t>
std::list<clone_ptr_t<global_issue_t> > pinnings_shards_mismatch_issue_tracker_t<protocol_t>::get_issues() {
    debugf("Getting issues\n");
    std::list<clone_ptr_t<global_issue_t> > res;

    typedef std::map<namespace_id_t, deletable_t<namespace_semilattice_metadata_t<protocol_t> > > namespace_map_t;

    namespace_map_t namespaces = semilattice_view->get().namespaces;

    for (typename namespace_map_t::iterator it  = namespaces.begin();
                                            it != namespaces.end();
                                            ++it) {
        if (it->second.is_deleted()) {
            continue;
        }
        std::set<typename protocol_t::region_t> shards = it->second.get().shards.get();
        region_map_t<protocol_t, std::set<machine_id_t> > pinnings = it->second.get().pinnings.get();
        for (typename std::set<typename protocol_t::region_t>::iterator shit  = shards.begin();
                                                                        shit != shards.end();
                                                                        ++shit) {
            region_map_t<protocol_t, std::set<machine_id_t> > masked_pinnings = pinnings.mask(*shit);

            std::set<machine_id_t> expected_val = masked_pinnings.begin()->second;
            for (typename region_map_t<protocol_t, std::set<machine_id_t> >::iterator pit  = masked_pinnings.begin();
                                                                                      pit != masked_pinnings.end();
                                                                                      ++pit) {
                if (pit->second!= expected_val) {
                    res.push_back(clone_ptr_t<global_issue_t>(new pinnings_shards_mismatch_issue_t<protocol_t>(it->first, shards, pinnings)));
                    goto NAMESPACE_HAS_ISSUE;
                }
            }
        }
NAMESPACE_HAS_ISSUE:
        ;
    }

    debugf("Done getting issues\n");
    return res;
}

#endif
