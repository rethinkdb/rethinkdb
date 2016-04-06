// Copyright 2010-2016 RethinkDB, all rights reserved.
#include "clustering/administration/persist/migrate/migrate_v2_1.hpp"

#include "clustering/administration/metadata.hpp"
#include "clustering/administration/persist/file_keys.hpp"
#include "clustering/administration/persist/migrate/metadata_v2_1.hpp"
#include "clustering/administration/persist/migrate/rewrite.hpp"
#include "clustering/administration/persist/raft_storage_interface.hpp"
#include "clustering/table_manager/table_metadata.hpp"
#include "clustering/administration/metadata.hpp"

// This will migrate all metadata from v2_1 to v2_3 (including auth metadata).
template <cluster_version_t W>
void migrate_metadata_v2_1_to_v2_3(metadata_file_t::write_txn_t *txn,
                                   signal_t *interruptor) {
    // Rewrite all metadata so it's serialized under the latest version
    rewrite_metadata_values<W>(mdkey_cluster_semilattices(), txn, interruptor);
    rewrite_metadata_values<W>(mdkey_heartbeat_semilattices(), txn, interruptor);
    rewrite_metadata_values<W>(mdkey_server_id(), txn, interruptor);
    rewrite_metadata_values<W>(mdkey_server_config(), txn, interruptor);

    rewrite_metadata_values<W>(mdprefix_table_active(), txn, interruptor);
    rewrite_metadata_values<W>(mdprefix_table_inactive(), txn, interruptor);
    rewrite_metadata_values<W>(mdprefix_table_raft_header(), txn, interruptor);
    rewrite_metadata_values<W>(mdprefix_table_raft_snapshot(), txn, interruptor);
    rewrite_metadata_values<W>(mdprefix_table_raft_log(), txn, interruptor);
    rewrite_metadata_values<W>(mdprefix_branch_birth_certificate(), txn, interruptor);

    rewrite_metadata_values<W>(metadata_v2_1::mdkey_auth_semilattices(),
                               txn, interruptor);

    migrate_auth_metadata_v2_1_to_v2_3(txn, interruptor);
}

// This is only exposed because auth metadata used to be in a separate file,
//  and when migrating from that file, we need this function.
void migrate_auth_metadata_v2_1_to_v2_3(metadata_file_t::write_txn_t *txn,
                                        signal_t *interruptor) {
    // Read out the old auth metadata
    metadata_v1_16::auth_semilattice_metadata_t old_metadata;
    std::string old_auth_key;

    // This can fail if we're migrating very old metadata, and it hasn't brought
    // in the auth_metadata file yet.  That will be done later and overwrite the
    // default value we write here.
    if (txn->read_maybe(metadata_v2_1::mdkey_auth_semilattices(),
                        &old_metadata,
                        interruptor)) {
        old_auth_key = old_metadata.auth_key.get_ref().key;
    }

    // Write out the new auth metadata
    auth_semilattice_metadata_t new_metadata(old_auth_key);
    txn->write(mdkey_auth_semilattices(), new_metadata, interruptor);
}

// This will migrate all metadata from v2_1 to v2_3 (including auth metadata).
void migrate_metadata_v2_1_to_v2_3(cluster_version_t serialization_version,
                                   metadata_file_t::write_txn_t *txn,
                                   signal_t *interruptor) {
    switch (serialization_version) {
    case cluster_version_t::v2_1:
        migrate_metadata_v2_1_to_v2_3<cluster_version_t::v2_1>(txn, interruptor);
        break;
    case cluster_version_t::v2_2:
        migrate_metadata_v2_1_to_v2_3<cluster_version_t::v2_2>(txn, interruptor);
        break;
    case cluster_version_t::v2_3:
        // This only really needs to migrate auth data, but this should be fine
        migrate_metadata_v2_1_to_v2_3<cluster_version_t::v2_3>(txn, interruptor);
        break;
    case cluster_version_t::v1_14:
    case cluster_version_t::v1_15:
    case cluster_version_t::v1_16:
    case cluster_version_t::v2_0:
    default:
        unreachable();
    }
}
