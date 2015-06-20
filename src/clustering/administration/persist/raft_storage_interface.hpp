// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_PERSIST_RAFT_STORAGE_INTERFACE_HPP_
#define CLUSTERING_ADMINISTRATION_PERSIST_RAFT_STORAGE_INTERFACE_HPP_

#include "clustering/administration/persist/file.hpp"
#include "clustering/generic/raft_core.hpp"
#include "clustering/table_contract/contract_metadata.hpp"

class metadata_file_t;

class table_raft_storage_interface_t :
    public raft_storage_interface_t<table_raft_state_t> {
public:
    /* This constructor loads an existing Raft state from the metadata file. */
    table_raft_storage_interface_t(
        metadata_file_t *file,
        metadata_file_t::read_txn_t *txn,
        const namespace_id_t &table_id,
        signal_t *interruptor);

    /* This constructor writes a new Raft state to the metadata file. */
    table_raft_storage_interface_t(
        metadata_file_t *file,
        metadata_file_t::write_txn_t *txn,
        const namespace_id_t &table_id,
        const raft_persistent_state_t<table_raft_state_t> &state,
        signal_t *interruptor);

    /* This method erases an existing Raft state in the metadata file. */
    static void erase(
        metadata_file_t::write_txn_t *txn,
        const namespace_id_t &table_id,
        signal_t *interruptor);

    /* `raft_storage_interface_t` methods */
    const raft_persistent_state_t<table_raft_state_t> *get();
    void write_current_term_and_voted_for(
        raft_term_t current_term,
        raft_member_id_t voted_for);
    void write_commit_index(
        raft_log_index_t commit_index);
    void write_log_replace_tail(
        const raft_log_t<table_raft_state_t> &source,
        raft_log_index_t first_replaced);
    void write_log_append_one(
        const raft_log_entry_t<table_raft_state_t> &entry);
    void write_snapshot(
        const table_raft_state_t &snapshot_state,
        const raft_complex_config_t &snapshot_config,
        bool clear_log,
        raft_log_index_t log_prev_index,
        raft_term_t log_prev_term,
        raft_log_index_t commit_index);

private:
    metadata_file_t *const file;
    namespace_id_t const table_id;
    raft_persistent_state_t<table_raft_state_t> state;
};

#endif /* CLUSTERING_ADMINISTRATION_PERSIST_RAFT_STORAGE_INTERFACE_HPP_ */

