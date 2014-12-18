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
