// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/metadata.hpp"

#include "clustering/administration/servers/server_metadata.hpp"
#include "containers/archive/archive.hpp"
#include "containers/archive/boost_types.hpp"
#include "containers/archive/cow_ptr_type.hpp"
#include "containers/archive/stl_types.hpp"
#include "containers/archive/versioned.hpp"
#include "rdb_protocol/protocol.hpp"
#include "stl_utils.hpp"

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
            "tables with that name in that database.", db_name.c_str(), name.c_str());
        return false;
    } else {
        return true;
    }
}

