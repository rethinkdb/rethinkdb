// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/persist/file_keys.hpp"

#include "rpc/connectivity/server_id.hpp"

metadata_file_t::key_t<cluster_semilattice_metadata_t>
        mdkey_cluster_semilattices() {
    return metadata_file_t::key_t<cluster_semilattice_metadata_t>("cluster_semilattice");
}

metadata_file_t::key_t<auth_semilattice_metadata_t>
        mdkey_auth_semilattices() {
    return metadata_file_t::key_t<auth_semilattice_metadata_t>("auth_semilattice");
}

metadata_file_t::key_t<heartbeat_semilattice_metadata_t>
        mdkey_heartbeat_semilattices() {
    return metadata_file_t::key_t<heartbeat_semilattice_metadata_t>(
        "heartbeat_semilattice");
}

metadata_file_t::key_t<server_id_t>
        mdkey_server_id() {
    return metadata_file_t::key_t<server_id_t>("server_id");
}

metadata_file_t::key_t<server_config_versioned_t>
        mdkey_server_config() {
    return metadata_file_t::key_t<server_config_versioned_t>("server_config");
}

metadata_file_t::key_t<table_active_persistent_state_t>
        mdprefix_table_active() {
    return metadata_file_t::key_t<table_active_persistent_state_t>("table.active/");
}

metadata_file_t::key_t<table_inactive_persistent_state_t>
        mdprefix_table_inactive() {
    return metadata_file_t::key_t<table_inactive_persistent_state_t>("table.inactive/");
}

metadata_file_t::key_t<table_raft_stored_header_t>
        mdprefix_table_raft_header() {
    return metadata_file_t::key_t<table_raft_stored_header_t>("table.header/");
}

metadata_file_t::key_t<table_raft_stored_snapshot_t>
        mdprefix_table_raft_snapshot() {
    return metadata_file_t::key_t<table_raft_stored_snapshot_t>("table.snapshot/");
}

metadata_file_t::key_t<raft_log_entry_t<table_raft_state_t> >
        mdprefix_table_raft_log() {
    return metadata_file_t::key_t<raft_log_entry_t<table_raft_state_t> >("table.log/");
}

metadata_file_t::key_t<branch_birth_certificate_t>
        mdprefix_branch_birth_certificate() {
    return metadata_file_t::key_t<branch_birth_certificate_t>("branch/");
}
