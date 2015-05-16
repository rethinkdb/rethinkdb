// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/persist/file_keys.hpp"

metadata_file_t::key_t<cluster_semilattice_metadata_t>
        mdkey_cluster_semilattices() {
    return metadata_file_t::key_t<cluster_semilattice_metadata_t>("cluster_semilattice");
}

metadata_file_t::key_t<auth_semilattice_metadata_t>
        mdkey_auth_semilattices() {
    return metadata_file_t::key_t<auth_semilattice_metadata_t>("auth_semilattice");
}

metadata_file_t::key_t<server_id_t>
        mdkey_server_id() {
    return metadata_file_t::key_t<server_id_t>("server_id");
}

metadata_file_t::key_t<server_config_t>
        mdkey_server_config() {
    return metadata_file_t::key_t<server_id_t>("server_config");
}

metadata_file_t::key_t<table_persistent_state_t>
        mdprefix_table_persistent_state() {
    return metadata_file_t::key_t<table_persistent_state_t>("table/");
}

metadata_file_t::key_t<branch_birth_certificate_t>
        mdprefix_branch_birth_certificate() {
    return metadata_file_t::key_t<branch_birth_certificate_t>("branch/");
}
