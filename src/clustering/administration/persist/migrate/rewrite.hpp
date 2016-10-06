// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_PERSIST_MIGRATE_REWRITE_HPP_
#define CLUSTERING_ADMINISTRATION_PERSIST_MIGRATE_REWRITE_HPP_

#include <map>
#include <string>

#include "clustering/administration/persist/file.hpp"

template <cluster_version_t W, typename T>
void rewrite_metadata_values(const metadata_file_t::key_t<T> &prefix,
                             metadata_file_t::write_txn_t *txn,
                             signal_t *interruptor) {
    std::map<std::string, T> values;
    txn->read_many<T, W>(prefix,
        [&](const std::string &key_suffix, const T &value) {
            values.insert(std::make_pair(key_suffix, value));
        }, interruptor);

    // Perform the writes separately to avoid recursive locking
    for (const auto &pair : values) {
        txn->write(prefix.suffix(pair.first), pair.second, interruptor);
    }
}

#endif /* CLUSTERING_ADMINISTRATION_PERSIST_MIGRATE_REWRITE_HPP_ */
