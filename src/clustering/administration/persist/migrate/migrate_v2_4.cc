// Copyright 2010-2026 RethinkDB, all rights reserved.
#include "clustering/administration/persist/migrate/migrate_v2_4.hpp"

#include "clustering/administration/metadata.hpp"
#include "clustering/administration/persist/file_keys.hpp"
#include "clustering/administration/persist/migrate/rewrite.hpp"
#include "clustering/administration/persist/raft_storage_interface.hpp"
#include "clustering/table_manager/table_metadata.hpp"

// This will migrate all metadata from v2_4 to v2_5
template <cluster_version_t W>
void migrate_metadata_v2_4_to_v2_5(metadata_file_t::write_txn_t *txn,
                                   signal_t *interruptor) {
    // Because we don't need to be "clever," we update all table
    // metadata, everything that the v2_3_to_v2_4 migration updated.
    // This is for the `table_config_t` change to add a `user_data`
    // field.
    rewrite_metadata_values<W>(mdprefix_table_active(), txn, interruptor);
    rewrite_metadata_values<W>(mdprefix_table_inactive(), txn, interruptor);
    rewrite_metadata_values<W>(mdprefix_table_raft_header(), txn, interruptor);
    rewrite_metadata_values<W>(mdprefix_table_raft_snapshot(), txn, interruptor);
    rewrite_metadata_values<W>(mdprefix_table_raft_log(), txn, interruptor);
}



// This will migrate all metadata from v2_4 to v2_5
void migrate_metadata_v2_4_to_v2_5(cluster_version_t serialization_version,
                                   metadata_file_t::write_txn_t *txn,
                                   signal_t *interruptor) {
    switch (serialization_version) {
    case cluster_version_t::v2_4:
        migrate_metadata_v2_4_to_v2_5<cluster_version_t::v2_4>(txn, interruptor);
        break;
    case cluster_version_t::v2_5_is_latest:
        break;
    case cluster_version_t::v1_14:
    case cluster_version_t::v1_15:
    case cluster_version_t::v1_16:
    case cluster_version_t::v2_0:
    case cluster_version_t::v2_1:
    case cluster_version_t::v2_2:
    case cluster_version_t::v2_3:
    default:
        unreachable();
    }
}
