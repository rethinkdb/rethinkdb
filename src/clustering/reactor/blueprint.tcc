#ifndef __CLUSTERING_REACTOR_BLUEPRINT_TCC__
#define __CLUSTERING_REACTOR_BLUEPRINT_TCC__

#include "http/json.hpp"
#include "http/json/json_adapter.hpp"

template <class ctx_t, class protocol_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(blueprint_t<protocol_t> *target, const ctx_t &ctx) {
    typename json_adapter_if_t<ctx_t>::json_adapter_map_t res;
    res["peers_roles"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(json_adapter_t<typename blueprint_t<protocol_t>::role_map_t, ctx_t>(&target->peers_roles));
    return  res;
}

template <class ctx_t, class protocol_t>
scoped_cJSON_t render_as_json(blueprint_t<protocol_t> *target, const ctx_t &ctx) {
    return render_as_directory(target, ctx);
}

template <class ctx_t, class protocol_t>
void apply_json_to(const scoped_cJSON_t &change, blueprint_t<protocol_t> *target, const ctx_t &ctx) {
    apply_as_directory(change, target, ctx);
}

template <class ctx_t, class protocol_t>
void on_subfield_change(blueprint_t<protocol_t> *, const ctx_t &) { }

#endif
