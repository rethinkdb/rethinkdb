// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_PERSIST_FILE_KEYS_HPP_
#define CLUSTERING_ADMINISTRATION_PERSIST_FILE_KEYS_HPP_

#include "clustering/administration/persist/file.hpp"
#include "containers/uuid.hpp"

class auth_semilattice_metadata_t;
class branch_birth_certificate_t;
class cluster_semilattice_metadata_t;
class server_config_t;
class table_persistent_state_t;

/* This file defines the keys that are used to index the `metadata_file_t`. Changing
these keys will break the on-disk format. */

metadata_file_t::key_t<cluster_semilattice_metadata_t>
    mdkey_cluster_semilattices();
metadata_file_t::key_t<auth_semilattice_metadata_t>
    mdkey_auth_semilattices();
metadata_file_t::key_t<server_id_t>
    mdkey_server_id();
metadata_file_t::key_t<server_config_t>
    mdkey_server_config();

/* This prefix should be followed by the table's UUID as formatted by `uuid_to_str()`. */
metadata_file_t::key_t<table_persistent_state_t>
    mdprefix_table_persistent_state();

/* This prefix should be followed by a string of the form `TABLE/BRANCH_ID`, where
`TABLE` and `BRANCH_ID` are UUIDs as formatted by `uuid_to_str()`. */
metadata_file_t::key_t<branch_birth_certificate_t>
    mdprefix_branch_birth_certificate();

#endif /* CLUSTERING_ADMINISTRATION_PERSIST_FILE_KEYS_HPP_ */

