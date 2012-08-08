#ifndef CLUSTERING_ADMINISTRATION_PERSISTABLE_BLUEPRINT_TCC_
#define CLUSTERING_ADMINISTRATION_PERSISTABLE_BLUEPRINT_TCC_

#include "clustering/administration/persistable_blueprint.hpp"

#include <string>

#include "http/json.hpp"
#include "http/json/json_adapter.hpp"

namespace blueprint_details {
// TODO: deinline these?
inline json_adapter_if_t::json_adapter_map_t get_json_subfields(role_t *) {
    return json_adapter_if_t::json_adapter_map_t();
}

inline cJSON *render_as_json(role_t *target) {
    switch (*target) {
    case role_primary:
        return cJSON_CreateString("role_primary");
        break;
    case role_secondary:
        return cJSON_CreateString("role_secondary");
        break;
    case role_nothing:
        return cJSON_CreateString("role_nothing");
        break;
    default:
        unreachable();
        break;
    };
}

inline void apply_json_to(cJSON *change, role_t *target) {
    std::string val = get_string(change);

    if (val == "role_primary" || val == "P") {
        *target = role_primary;
    } else if (val == "role_secondary" || val == "S") {
        *target = role_secondary;
    } else if (val == "role_nothing" || val == "N") {
        *target = role_nothing;
    } else {
        throw schema_mismatch_exc_t("Cannot set a role_t object using %s."
                "Acceptable values are: \"role_primary\", \"role_secondary\","
                "\"role_nothing\".\n");
    }
}

inline void on_subfield_change(role_t *) { }
} //namespace blueprint_details

template <class protocol_t, class ctx_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(persistable_blueprint_t<protocol_t> *target, UNUSED const ctx_t &ctx) {
    json_adapter_if_t::json_adapter_map_t res;
    res["peers_roles"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<typename persistable_blueprint_t<protocol_t>::role_map_t>(&target->machines_roles));
    return  res;
}

template <class protocol_t, class ctx_t>
cJSON *render_as_json(persistable_blueprint_t<protocol_t> *target, const ctx_t &ctx) {
    return render_as_directory(target, ctx);
}

template <class protocol_t, class ctx_t>
void apply_json_to(cJSON *change, persistable_blueprint_t<protocol_t> *target, const ctx_t &ctx) {
    apply_as_directory(change, target, ctx);
}

template <class protocol_t, class ctx_t>
void on_subfield_change(persistable_blueprint_t<protocol_t> *, const ctx_t &) { }

#endif /* CLUSTERING_ADMINISTRATION_PERSISTABLE_BLUEPRINT_TCC_ */
