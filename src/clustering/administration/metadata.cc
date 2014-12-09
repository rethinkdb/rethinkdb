// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/metadata.hpp"

#include "clustering/administration/servers/server_metadata.hpp"
#include "containers/archive/archive.hpp"
#include "containers/archive/boost_types.hpp"
#include "containers/archive/cow_ptr_type.hpp"
#include "containers/archive/stl_types.hpp"
#include "containers/archive/versioned.hpp"
#include "rdb_protocol/protocol.hpp"
#include "region/region_map_json_adapter.hpp"
#include "stl_utils.hpp"

/* RSI(reql_admin): The ReQL admin API changes involve big changes to the semilattice
metadata. Because many of the underlying concepts have changed, it's not clear if/how to
migrate serialized metadata from pre-ReQL-admin versions into the new format. GitHub
issue #2869 is tracking this discussion. As a temporary measure so that development can
proceed, deserialization of old versions has been disabled. */

RDB_IMPL_SERIALIZABLE_3_SINCE_v1_16(server_semilattice_metadata_t,
                                    name, tags, cache_size_bytes);
RDB_IMPL_SEMILATTICE_JOINABLE_3(server_semilattice_metadata_t,
                                name, tags, cache_size_bytes);
RDB_IMPL_EQUALITY_COMPARABLE_3(server_semilattice_metadata_t,
                               name, tags, cache_size_bytes);

RDB_IMPL_SERIALIZABLE_1_SINCE_v1_16(servers_semilattice_metadata_t, servers);
RDB_IMPL_SEMILATTICE_JOINABLE_1(servers_semilattice_metadata_t, servers);
RDB_IMPL_EQUALITY_COMPARABLE_1(servers_semilattice_metadata_t, servers);

RDB_IMPL_SERIALIZABLE_3(server_config_business_card_t,
                        change_name_addr, change_tags_addr, change_cache_size_addr);
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(server_config_business_card_t);

RDB_IMPL_SERIALIZABLE_3_SINCE_v1_16(
        cluster_semilattice_metadata_t,
        rdb_namespaces, servers, databases);
RDB_IMPL_SEMILATTICE_JOINABLE_3(cluster_semilattice_metadata_t,
                                rdb_namespaces, servers, databases);
RDB_IMPL_EQUALITY_COMPARABLE_3(cluster_semilattice_metadata_t,
                               rdb_namespaces, servers, databases);

RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(auth_semilattice_metadata_t, auth_key);
RDB_IMPL_SEMILATTICE_JOINABLE_1(auth_semilattice_metadata_t, auth_key);
RDB_IMPL_EQUALITY_COMPARABLE_1(auth_semilattice_metadata_t, auth_key);

RDB_IMPL_SERIALIZABLE_18_FOR_CLUSTER(cluster_directory_metadata_t,
     server_id,
     peer_id,
     version,
     time_started,
     pid,
     hostname,
     cluster_port,
     reql_port,
     http_admin_port,
     canonical_addresses,
     argv,
     actual_cache_size_bytes,
     jobs_mailbox,
     get_stats_mailbox_address,
     log_mailbox,
     server_config_business_card,
     local_issues,
     peer_type);

// ctx-less json adapter concept for cluster_directory_metadata_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(cluster_directory_metadata_t *target) {
    json_adapter_if_t::json_adapter_map_t res;
    res["server_id"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<server_id_t>(&target->server_id)); // TODO: should be 'me'?
    res["peer_id"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<peer_id_t>(&target->peer_id));
    res["peer_type"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<cluster_directory_peer_type_t>(&target->peer_type));
    return res;
}

cJSON *render_as_json(cluster_directory_metadata_t *target) {
    return render_as_directory(target);
}

void apply_json_to(cJSON *change, cluster_directory_metadata_t *target) {
    apply_as_directory(change, target);
}

// ctx-less json adapter for cluster_directory_peer_type_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(cluster_directory_peer_type_t *) {
    return std::map<std::string, boost::shared_ptr<json_adapter_if_t> >();
}

cJSON *render_as_json(cluster_directory_peer_type_t *peer_type) {
    switch (*peer_type) {
    case SERVER_PEER:
        return cJSON_CreateString("server");
    case PROXY_PEER:
        return cJSON_CreateString("proxy");
    default:
        break;
    }
    return cJSON_CreateString("unknown");
}

void apply_json_to(cJSON *, cluster_directory_peer_type_t *) { }

bool check_metadata_status(metadata_search_status_t status,
                           const char *entity_type,
                           const std::string &entity_name,
                           bool expect_present,
                           std::string *error_out) {
    switch (status) {
        case METADATA_SUCCESS: {
            if (expect_present) {
                return true;
            } else {
                *error_out = strprintf("%s `%s` already exists.",
                    entity_type, entity_name.c_str());
                return false;
            }
        }
        case METADATA_ERR_MULTIPLE: {
            if (expect_present) {
                *error_out = strprintf("%s `%s` is ambiguous; there are multiple "
                    "entities with that name.", entity_type, entity_name.c_str());
            } else {
                *error_out = strprintf("%s `%s` already exists.",
                    entity_type, entity_name.c_str());
            }
            return false;
        }
        case METADATA_ERR_NONE: {
            if (expect_present) {
                *error_out = strprintf("%s `%s` does not exist.",
                    entity_type, entity_name.c_str());
                return false;
            } else {
                return true;
            }
        default: unreachable();
        }
    }
}
