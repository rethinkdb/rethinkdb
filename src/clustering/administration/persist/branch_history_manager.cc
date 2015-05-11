// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/persist/branch_history_manager.hpp"

#include "clustering/administration/persist/file_keys.hpp"

void real_branch_history_manager_t::erase(
        const namespace_id_t &table_id,
        metadata_file_t *metadata_file,
        signal_t *interruptor) {
    metadata_file_t::write_txn_t write_txn(metadata_file, interruptor);
    std::set<branch_id_t> branch_ids;
    write_txn.read_many<branch_birth_certificate_t>(
        mdprefix_branch_birth_certificate().suffix(uuid_to_str(table_id) + "/"),
        [&](const std::string &branch_id_str, const branch_birth_certificate_t &) {
            branch_id_t branch_id = str_to_uuid(branch_id_str);
            branch_ids.insert(branch_id);
        },
        interruptor);
    for (const branch_id_t &b : branch_ids) {
        write_txn.erase(
            mdprefix_branch_birth_certificate().suffix(
                uuid_to_str(table_id) + "/" + uuid_to_str(b)),
            interruptor);
    }
}

real_branch_history_manager_t::real_branch_history_manager_t(
        const namespace_id_t &_table_id,
        metadata_file_t *_metadata_file,
        signal_t *interruptor):
    table_id(_table_id), metadata_file(_metadata_file)
{
    metadata_file_t::read_txn_t read_txn(metadata_file, interruptor);
    read_txn.read_many<branch_birth_certificate_t>(
        mdprefix_branch_birth_certificate().suffix(uuid_to_str(table_id) + "/"),
        [&](const std::string &branch_id_str, const branch_birth_certificate_t &bc) {
            branch_id_t branch_id = str_to_uuid(branch_id_str);
            cache.branches.insert(std::make_pair(branch_id, bc));
        },
        interruptor);
}

branch_birth_certificate_t 
real_branch_history_manager_t::get_branch(const branch_id_t &branch)
        const THROWS_ONLY(missing_branch_exc_t) {
    assert_thread();
    return cache.get_branch(branch);
}

bool real_branch_history_manager_t::is_branch_known(const branch_id_t &branch)
        const THROWS_NOTHING {
    assert_thread();
    return cache.is_branch_known(branch);
}

void real_branch_history_manager_t::create_branch(
        branch_id_t branch_id,
        const branch_birth_certificate_t &bc,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    assert_thread();
    {
        metadata_file_t::write_txn_t write_txn(metadata_file, interruptor);
        write_txn.write(
            mdprefix_branch_birth_certificate().suffix(
                uuid_to_str(table_id) + "/" + uuid_to_str(branch_id)),
            bc,
            interruptor);
    }
    cache.branches.insert(std::make_pair(branch_id, bc));
}

void real_branch_history_manager_t::import_branch_history(
        const branch_history_t &new_records,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    assert_thread();
    {
        metadata_file_t::write_txn_t write_txn(metadata_file, interruptor);
        for (const auto &pair : new_records.branches) {
            if (!is_branch_known(pair.first)) {
                write_txn.write(
                    mdprefix_branch_birth_certificate().suffix(
                        uuid_to_str(table_id) + "/" + uuid_to_str(pair.first)),
                    pair.second,
                    interruptor);
            }
        }
    }
    cache.branches.insert(new_records.branches.begin(), new_records.branches.end());
}

