#include "clustering/administration/persistable_blueprint.hpp"

#include <string>

#include "http/json.hpp"
#include "http/json/json_adapter.hpp"

namespace blueprint_details {
json_adapter_if_t::json_adapter_map_t get_json_subfields(role_t *) {
    return json_adapter_if_t::json_adapter_map_t();
}

cJSON *render_as_json(role_t *target) {
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

void apply_json_to(cJSON *change, role_t *target) {
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

void on_subfield_change(role_t *) { }
}  // namespace blueprint_details

template <class protocol_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(persistable_blueprint_t<protocol_t> *target) {
    json_adapter_if_t::json_adapter_map_t res;
    res["peers_roles"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<typename persistable_blueprint_t<protocol_t>::role_map_t>(&target->machines_roles));
    return  res;
}

template <class protocol_t>
cJSON *render_as_json(persistable_blueprint_t<protocol_t> *target) {
    return render_as_directory(target);
}

template <class protocol_t>
void apply_json_to(cJSON *change, persistable_blueprint_t<protocol_t> *target) {
    apply_as_directory(change, target);
}

template <class protocol_t>
void on_subfield_change(persistable_blueprint_t<protocol_t> *) { }

#include "memcached/protocol.hpp"
#include "memcached/protocol_json_adapter.hpp"
template json_adapter_if_t::json_adapter_map_t get_json_subfields<memcached_protocol_t>(persistable_blueprint_t<memcached_protocol_t> *);
template cJSON *render_as_json<memcached_protocol_t>(persistable_blueprint_t<memcached_protocol_t> *);
template void apply_json_to<memcached_protocol_t>(cJSON *, persistable_blueprint_t<memcached_protocol_t> *);
template void on_subfield_change<memcached_protocol_t>(persistable_blueprint_t<memcached_protocol_t> *);

#include "rdb_protocol/protocol.hpp"
template json_adapter_if_t::json_adapter_map_t get_json_subfields<rdb_protocol_t>(persistable_blueprint_t<rdb_protocol_t> *);
template cJSON *render_as_json<rdb_protocol_t>(persistable_blueprint_t<rdb_protocol_t> *);
template void apply_json_to<rdb_protocol_t>(cJSON *, persistable_blueprint_t<rdb_protocol_t> *);
template void on_subfield_change<rdb_protocol_t>(persistable_blueprint_t<rdb_protocol_t> *);

#include "mock/dummy_protocol_json_adapter.hpp"
template json_adapter_if_t::json_adapter_map_t get_json_subfields<mock::dummy_protocol_t>(persistable_blueprint_t<mock::dummy_protocol_t> *);
template cJSON *render_as_json<mock::dummy_protocol_t>(persistable_blueprint_t<mock::dummy_protocol_t> *);
template void apply_json_to<mock::dummy_protocol_t>(cJSON *, persistable_blueprint_t<mock::dummy_protocol_t> *);
template void on_subfield_change<mock::dummy_protocol_t>(persistable_blueprint_t<mock::dummy_protocol_t> *);
