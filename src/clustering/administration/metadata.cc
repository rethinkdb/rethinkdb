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

// RSI(raft): Some of these should be `SINCE_v1_N`, where `N` is the version number at
// which Raft is released.

RDB_IMPL_SERIALIZABLE_3_SINCE_v1_16(server_config_t, name, tags, cache_size_bytes);
RDB_IMPL_EQUALITY_COMPARABLE_3(server_config_t, name, tags, cache_size_bytes);

RDB_IMPL_SERIALIZABLE_2_SINCE_v1_16(server_config_versioned_t, config, version);
RDB_IMPL_EQUALITY_COMPARABLE_2(server_config_versioned_t, config, version);

RDB_IMPL_SERIALIZABLE_1_SINCE_v1_16(server_name_map_t, names);
RDB_IMPL_EQUALITY_COMPARABLE_1(server_name_map_t, names);

RDB_IMPL_SERIALIZABLE_1(server_config_business_card_t, set_config_addr);
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(server_config_business_card_t);

RDB_IMPL_SERIALIZABLE_1_SINCE_v1_16(cluster_semilattice_metadata_t, databases);
RDB_IMPL_SEMILATTICE_JOINABLE_1(cluster_semilattice_metadata_t, databases);
RDB_IMPL_EQUALITY_COMPARABLE_1(cluster_semilattice_metadata_t, databases);

RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(auth_semilattice_metadata_t, auth_key);
RDB_IMPL_SEMILATTICE_JOINABLE_1(auth_semilattice_metadata_t, auth_key);
RDB_IMPL_EQUALITY_COMPARABLE_1(auth_semilattice_metadata_t, auth_key);

RDB_IMPL_SERIALIZABLE_9_FOR_CLUSTER(proc_directory_metadata_t,
    version,
    time_started,
    pid,
    hostname,
    cluster_port,
    reql_port,
    http_admin_port,
    canonical_addresses,
    argv);

RDB_IMPL_SERIALIZABLE_12_FOR_CLUSTER(cluster_directory_metadata_t,
     server_id,
     peer_id,
     proc,
     actual_cache_size_bytes,
     multi_table_manager_bcard,
     jobs_mailbox,
     get_stats_mailbox_address,
     log_mailbox,
     server_config,
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


