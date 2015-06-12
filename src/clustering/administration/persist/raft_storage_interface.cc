// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/persist/raft_storage_interface.hpp"

#include "clustering/administration/persist/file_keys.hpp"

/* The raft state is stored as follows:
- There is a single `table_raft_stored_header_t` which stores `current_term` and
    `voted_for`.
- There is a single `table_raft_stored_snapshot_t` which stores `snapshot_state`,
    `snapshot_config`, `log.prev_index`, and `log.prev_term`.
- There are zero or more `raft_log_entry_t`s, which use the log index as part of the
    B-tree key. */

class table_raft_stored_header_t {
public:
    raft_term_t current_term;
    raft_member_id_t voted_for;
};

RDB_IMPL_SERIALIZABLE_2_SINCE_v2_1(table_raft_stored_header_t,
    current_term, voted_for);

class table_raft_stored_snapshot_t {
public:
    table_raft_state_t snapshot_state;
    raft_complex_config_t snapshot_config;
    raft_log_index_t log_prev_index;
    raft_term_t log_prev_term;
};

RDB_IMPL_SERIALIZABLE_4_SINCE_v2_1(table_raft_stored_snapshot_t,
    snapshot_state, snapshot_config, log_prev_index, log_prev_term);

raft_log_index_t str_to_log_index(const std::string &str) {
    guarantee(str.size() == 16);
    raft_log_index_t index = 0;
    for (size_t i = 0; i < 16; ++i) {
        int val;
        if (str[i] >= '0' && str[i] <= '9') {
            val = str[i] - '0';
        } else if (str[i] >= 'a' && str[i] <= 'f') {
            val = 10 + (str[i] - 'a');
        } else {
            crash("bad character in str_to_log_index()");
        }
        index += val << ((15 - i) * 4);
    }
    return index;
}

std::string log_index_to_str(raft_log_index_t log_index) {
    std::string str;
    str.reserve(16);
    for (size_t i = 0; i < 16; ++i) {
        int val = log_index >> ((15 - i) * 4);
        str.push_back("0123456789abcdef"[val]);
    }
    return str;
}

table_raft_storage_interface_t::table_raft_storage_interface_t(
        metadata_file_t *_file,
        metadata_file_t::read_txn_t *txn,
        const namespace_id_t &_table_id,
        signal_t *interruptor) :
        file(_file), table_id(_table_id) {
    table_raft_stored_header_t header = txn->read(
        mdprefix_table_raft_header().suffix(uuid_to_str(table_id)), interruptor);
    state.current_term = header.current_term;
    state.voted_for = header.voted_for;
    table_raft_stored_snapshot_t snapshot = txn->read(
        mdprefix_table_raft_snapshot().suffix(uuid_to_str(table_id)), interruptor);
    state.snapshot_state = std::move(snapshot.snapshot_state);
    state.snapshot_config = std::move(snapshot.snapshot_config);
    state.log.prev_index = snapshot.log_prev_index;
    state.log.prev_term = snapshot.log_prev_term;
    txn->read_many<raft_log_entry_t<table_raft_state_t> >(
        mdprefix_table_raft_log().suffix(uuid_to_str(table_id) + "/"),
        [&](const std::string &index_str,
                const raft_log_entry_t<table_raft_state_t> &entry) {
            guarantee(str_to_log_index(index_str) == state.log.get_latest_index() + 1);
            state.log.append(entry);
        },
        interruptor);
}

table_raft_storage_interface_t::table_raft_storage_interface_t(
        metadata_file_t *_file,
        metadata_file_t::write_txn_t *txn,
        const namespace_id_t &_table_id,
        const raft_persistent_state_t<table_raft_state_t> &_state,
        signal_t *interruptor) :
        file(_file), table_id(_table_id), state(_state) {
    table_raft_stored_header_t header;
    header.current_term = state.current_term;
    header.voted_for = state.voted_for;
    txn->write(
        mdprefix_table_raft_header().suffix(uuid_to_str(table_id)),
        header,
        interruptor);

    /* To avoid expensive copies of `state`, we move `state` into the snapshot and then
    back out after we're done */
    table_raft_stored_snapshot_t snapshot;
    snapshot.snapshot_state = std::move(state.snapshot_state);
    snapshot.snapshot_config = std::move(state.snapshot_config);
    snapshot.log_prev_index = state.log.prev_index;
    snapshot.log_prev_term = state.log.prev_term;
    txn->write(
        mdprefix_table_raft_snapshot().suffix(uuid_to_str(table_id)),
        snapshot,
        interruptor);
    state.snapshot_state = std::move(snapshot.snapshot_state);
    state.snapshot_config = std::move(snapshot.snapshot_config);

    for (raft_log_index_t i = state.log.prev_index + 1;
            i <= state.log.get_latest_index(); ++i) {
        txn->write(
            mdprefix_table_raft_log().suffix(
                uuid_to_str(table_id) + "/" + log_index_to_str(i)),
            state.log.get_entry_ref(i),
            interruptor);
    }
}

void table_raft_storage_interface_t::erase(
        metadata_file_t::write_txn_t *txn,
        const namespace_id_t &table_id,
        signal_t *interruptor) {
    txn->erase(
        mdprefix_table_raft_header().suffix(uuid_to_str(table_id)),
        interruptor);
    txn->erase(
        mdprefix_table_raft_snapshot().suffix(uuid_to_str(table_id)),
        interruptor);
    std::vector<std::string> log_keys;
    txn->read_many<raft_log_entry_t<table_raft_state_t> >(
        mdprefix_table_raft_log().suffix(uuid_to_str(table_id) + "/"),
        [&](const std::string &index_str, const raft_log_entry_t<table_raft_state_t> &) {
            log_keys.push_back(index_str);
        },
        interruptor);
    for (const std::string &key : log_keys) {
        txn->erase(
            mdprefix_table_raft_log().suffix(uuid_to_str(table_id) + "/" + key),
            interruptor);
    }
}

const raft_persistent_state_t<table_raft_state_t> *
table_raft_storage_interface_t::get() {
    return &state;
}

void table_raft_storage_interface_t::write_current_term_and_voted_for(
        raft_term_t current_term,
        raft_member_id_t voted_for,
        signal_t *interruptor) {
    metadata_file_t::write_txn_t txn(file, interruptor);
    table_raft_stored_header_t header;
    header.current_term = state.current_term = current_term;
    header.voted_for = state.voted_for = voted_for;
    txn.write(
        mdprefix_table_raft_header().suffix(uuid_to_str(table_id)),
        header,
        interruptor);
}

void table_raft_storage_interface_t::write_log_replace_tail(
        const raft_log_t<table_raft_state_t> &source,
        raft_log_index_t first_replaced,
        signal_t *interruptor) {
    metadata_file_t::write_txn_t txn(file, interruptor);
    guarantee(first_replaced > state.log.prev_index);
    guarantee(first_replaced <= state.log.get_latest_index() + 1);
    for (raft_log_index_t i = first_replaced;
            i < std::max(state.log.get_latest_index(), source.get_latest_index()); ++i) {
        metadata_file_t::key_t<raft_log_entry_t<table_raft_state_t> > key =
            mdprefix_table_raft_log().suffix(
                uuid_to_str(table_id) + "/" + log_index_to_str(i));
        if (i <= source.get_latest_index()) {
            txn.write(key, source.get_entry_ref(i), interruptor);
        } else {
            txn.erase(key, interruptor);
        }
    }
    if (first_replaced != state.log.get_latest_index() + 1) {
        state.log.delete_entries_from(first_replaced);
    }
    for (raft_log_index_t i = first_replaced; i <= source.get_latest_index(); ++i) {
        state.log.append(source.get_entry_ref(i));
    }
}

void table_raft_storage_interface_t::write_log_append_one(
        const raft_log_entry_t<table_raft_state_t> &entry,
        signal_t *interruptor) {
    metadata_file_t::write_txn_t txn(file, interruptor);
    raft_log_index_t index = state.log.get_latest_index() + 1;
    txn.write(
        mdprefix_table_raft_log().suffix(
            uuid_to_str(table_id) + "/" + log_index_to_str(index)),
        entry,
        interruptor);
    state.log.append(entry);
}

void table_raft_storage_interface_t::write_snapshot(
        const table_raft_state_t &snapshot_state,
        const raft_complex_config_t &snapshot_config,
        raft_log_index_t log_prev_index,
        raft_term_t log_prev_term,
        signal_t *interruptor) {
    metadata_file_t::write_txn_t txn(file, interruptor);
    table_raft_stored_snapshot_t snapshot;
    snapshot.snapshot_state = snapshot_state;
    snapshot.snapshot_config = snapshot_config;
    snapshot.log_prev_index = log_prev_index;
    snapshot.log_prev_term = log_prev_term;
    txn.write(
        mdprefix_table_raft_snapshot().suffix(uuid_to_str(table_id)),
        snapshot,
        interruptor);
    for (raft_log_index_t i = state.log.prev_index + 1; i <= log_prev_index; ++i) {
        txn.erase(
            mdprefix_table_raft_log().suffix(
                uuid_to_str(table_id) + "/" + log_index_to_str(i)),
            interruptor);
    }
    state.snapshot_state = std::move(snapshot.snapshot_state);
    state.snapshot_config = std::move(snapshot.snapshot_config);
    state.log.delete_entries_to(log_prev_index, log_prev_term);
}

