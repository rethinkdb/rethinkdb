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

RDB_IMPL_SERIALIZABLE_2_SINCE_v1_16(server_semilattice_metadata_t, name, tags);
RDB_IMPL_SEMILATTICE_JOINABLE_2(server_semilattice_metadata_t, name, tags);
RDB_IMPL_EQUALITY_COMPARABLE_2(server_semilattice_metadata_t, name, tags);

RDB_IMPL_SERIALIZABLE_1_SINCE_v1_16(servers_semilattice_metadata_t, servers);
RDB_IMPL_SEMILATTICE_JOINABLE_1(servers_semilattice_metadata_t, servers);
RDB_IMPL_EQUALITY_COMPARABLE_1(servers_semilattice_metadata_t, servers);

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
     cache_size,
     time_started,
     pid,
     hostname,
     cluster_port,
     reql_port,
     http_admin_port,
     canonical_addresses,
     argv,
     jobs_mailbox,
     get_stats_mailbox_address,
     log_mailbox,
     server_name_business_card,
     local_issues,
     peer_type);

// ctx-less json adapter concept for cluster_directory_metadata_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(cluster_directory_metadata_t *target) {
    json_adapter_if_t::json_adapter_map_t res;
    res["server_id"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<server_id_t>(&target->server_id)); // TODO: should be 'me'?
    res["peer_id"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<peer_id_t>(&target->peer_id));
    res["cache_size"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<uint64_t>(&target->cache_size));
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

bool search_db_metadata_by_name(
        const databases_semilattice_metadata_t &metadata,
        const name_string_t &name,
        database_id_t *id_out,
        std::string *error_out) {
    size_t found = 0;
    for (const auto &pair : metadata.databases) {
        if (!pair.second.is_deleted() && pair.second.get_ref().name.get_ref() == name) {
            *id_out = pair.first;
            ++found;
        }
    }
    if (found == 0) {
        *error_out = strprintf("Database `%s` does not exist.", name.c_str());
        return false;
    } else if (found >= 2) {
        *error_out = strprintf("Database `%s` is ambiguous; there are multiple "
            "databases with that name.", name.c_str());
        return false;
    } else {
        return true;
    }
}

bool search_table_metadata_by_name(
        const namespaces_semilattice_metadata_t &metadata,
        const database_id_t &db_id,
        const name_string_t &db_name,
        const name_string_t &name,
        namespace_id_t *id_out,
        std::string *error_out) {
    size_t found = 0;
    for (const auto &pair : metadata.namespaces) {
        if (!pair.second.is_deleted() &&
                pair.second.get_ref().database.get_ref() == db_id &&
                pair.second.get_ref().name.get_ref() == name) {
            *id_out = pair.first;
            ++found;
        }
    }
    if (found == 0) {
        *error_out = strprintf("Table `%s.%s` does not exist.",
            db_name.c_str(), name.c_str());
        return false;
    } else if (found >= 2) {
        *error_out = strprintf("Table `%s.%s` is ambiguous; there are multiple "
            "databases with that name.", db_name.c_str(), name.c_str());
        return false;
    } else {
        return true;
    }
}

