// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/persistable_blueprint.hpp"

#include <string>

#include "debug.hpp"
#include "http/json.hpp"
#include "http/json/json_adapter.hpp"
#include "rdb_protocol/protocol.hpp"
#include "region/region_json_adapter.hpp"
#include "stl_utils.hpp"

RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(persistable_blueprint_t, machines_roles);

void debug_print(printf_buffer_t *buf, const persistable_blueprint_t &x) {
    buf->appendf("persistable_blueprint{machines_roles=");
    debug_print(buf, x.machines_roles);
    buf->appendf("}");
}


json_adapter_if_t::json_adapter_map_t get_json_subfields(blueprint_role_t *) {
    return json_adapter_if_t::json_adapter_map_t();
}

cJSON *render_as_json(blueprint_role_t *target) {
    switch (*target) {
    case blueprint_role_primary:
        return cJSON_CreateString("role_primary");
        break;
    case blueprint_role_secondary:
        return cJSON_CreateString("role_secondary");
        break;
    case blueprint_role_nothing:
        return cJSON_CreateString("role_nothing");
        break;
    default:
        unreachable();
        break;
    };
}

void apply_json_to(cJSON *change, blueprint_role_t *target) {
    std::string val = get_string(change);

    if (val == "role_primary" || val == "P") {
        *target = blueprint_role_primary;
    } else if (val == "role_secondary" || val == "S") {
        *target = blueprint_role_secondary;
    } else if (val == "role_nothing" || val == "N") {
        *target = blueprint_role_nothing;
    } else {
        throw schema_mismatch_exc_t("Cannot set a blueprint_role_t object using %s."
                "Acceptable values are: \"role_primary\", \"role_secondary\","
                "\"role_nothing\".\n");
    }
}

json_adapter_if_t::json_adapter_map_t get_json_subfields(persistable_blueprint_t *target) {
    json_adapter_if_t::json_adapter_map_t res;
    res["peers_roles"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<persistable_blueprint_t::role_map_t>(&target->machines_roles));
    return  res;
}

cJSON *render_as_json(persistable_blueprint_t *target) {
    return render_as_directory(target);
}

void apply_json_to(cJSON *change, persistable_blueprint_t *target) {
    apply_as_directory(change, target);
}

