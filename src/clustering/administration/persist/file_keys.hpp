// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_PERSIST_FILE_KEYS_HPP_
#define CLUSTERING_ADMINISTRATION_PERSIST_FILE_KEYS_HPP_

#include "clustering/administration/persist/file.hpp"
#include "containers/uuid.hpp"

class auth_semilattice_metadata_t;
class branch_birth_certificate_t;
class cluster_semilattice_metadata_t;
class heartbeat_semilattice_metadata_t;
template<class state_t> class raft_log_entry_t;
class server_config_versioned_t;
class table_active_persistent_state_t;
class table_inactive_persistent_state_t;
class table_raft_state_t;
class table_raft_stored_header_t;
class table_raft_stored_snapshot_t;

/* This file defines the keys that are used to index the `metadata_file_t`. Changing
these keys will break the on-disk format. */

metadata_file_t::key_t<cluster_semilattice_metadata_t>
    mdkey_cluster_semilattices();
metadata_file_t::key_t<auth_semilattice_metadata_t>
    mdkey_auth_semilattices();
metadata_file_t::key_t<heartbeat_semilattice_metadata_t>
    mdkey_heartbeat_semilattices();
metadata_file_t::key_t<server_id_t>
    mdkey_server_id();
metadata_file_t::key_t<server_config_versioned_t>
    mdkey_server_config();

/* These prefixes should be followed by the table's UUID as formatted by `uuid_to_str()`.
*/
metadata_file_t::key_t<table_active_persistent_state_t>
    mdprefix_table_active();
metadata_file_t::key_t<table_inactive_persistent_state_t>
    mdprefix_table_inactive();
metadata_file_t::key_t<table_raft_stored_header_t>
    mdprefix_table_raft_header();
metadata_file_t::key_t<table_raft_stored_snapshot_t>
    mdprefix_table_raft_snapshot();

/* This prefix should be followed by a string of the form `TABLE/LOG_INDEX`, where
`TABLE` is a UUID as before, and `LOG_INDEX` is a 16-digit hexadecimal. */
metadata_file_t::key_t<raft_log_entry_t<table_raft_state_t> >
    mdprefix_table_raft_log();

/* This prefix should be followed by a string of the form `TABLE/BRANCH_ID`, where
`TABLE` and `BRANCH_ID` are UUIDs as formatted by `uuid_to_str()`. */
metadata_file_t::key_t<branch_birth_certificate_t>
    mdprefix_branch_birth_certificate();

#endif /* CLUSTERING_ADMINISTRATION_PERSIST_FILE_KEYS_HPP_ */

