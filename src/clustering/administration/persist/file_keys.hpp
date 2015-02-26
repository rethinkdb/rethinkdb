// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_PERSIST_FILE_KEYS_HPP_
#define CLUSTERING_ADMINISTRATION_PERSIST_FILE_KEYS_HPP_

#include "clustering/administration/persist/file.hpp"

/* This file defines the keys that are used to index the `metadata_file_t`. Changing
these keys will break the on-disk format. */

metadata_file_t::key_t<cluster_semilattice_metadata_t>
    mdkey_cluster_semilattices();
metadata_file_t::key_t<auth_semilattice_metadata_t>
    mdkey_auth_semilattices();
metadata_file_t::key_t<server_id_t>
    mdkey_server_id();

#endif /* CLUSTERING_ADMINISTRATION_PERSIST_FILE_KEYS_HPP_ */

