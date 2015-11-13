// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/persist/branch_history_manager.hpp"

#include "clustering/administration/persist/file_keys.hpp"

void real_branch_history_manager_t::erase(
        metadata_file_t::write_txn_t *write_txn,
        const namespace_id_t &table_id) {
    cond_t non_interruptor;
    std::set<branch_id_t> branch_ids;
    write_txn->read_many<branch_birth_certificate_t>(
        mdprefix_branch_birth_certificate().suffix(uuid_to_str(table_id) + "/"),
        [&](const std::string &branch_id_str, const branch_birth_certificate_t &) {
            branch_id_t branch_id = str_to_uuid(branch_id_str);
            branch_ids.insert(branch_id);
        },
        &non_interruptor);
    for (const branch_id_t &b : branch_ids) {
        write_txn->erase(
            mdprefix_branch_birth_certificate().suffix(
                uuid_to_str(table_id) + "/" + uuid_to_str(b)),
            &non_interruptor);
    }
}

real_branch_history_manager_t::real_branch_history_manager_t(
        const namespace_id_t &_table_id,
        metadata_file_t *_metadata_file,
        metadata_file_t::read_txn_t *metadata_read_txn,
        signal_t *interruptor):
    table_id(_table_id), metadata_file(_metadata_file)
{
    metadata_read_txn->read_many<branch_birth_certificate_t>(
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
        const branch_birth_certificate_t &bc)
        THROWS_NOTHING {
    assert_thread();
    {
        cond_t non_interruptor;
        metadata_file_t::write_txn_t write_txn(metadata_file, &non_interruptor);
        write_txn.write(
            mdprefix_branch_birth_certificate().suffix(
                uuid_to_str(table_id) + "/" + uuid_to_str(branch_id)),
            bc,
            &non_interruptor);
    }
    cache.branches.insert(std::make_pair(branch_id, bc));
}

void real_branch_history_manager_t::import_branch_history(
        const branch_history_t &new_records)
        THROWS_NOTHING {
    assert_thread();
    {
        cond_t non_interruptor;
        metadata_file_t::write_txn_t write_txn(metadata_file, &non_interruptor);
        for (const auto &pair : new_records.branches) {
            if (!is_branch_known(pair.first)) {
                write_txn.write(
                    mdprefix_branch_birth_certificate().suffix(
                        uuid_to_str(table_id) + "/" + uuid_to_str(pair.first)),
                    pair.second,
                    &non_interruptor);
                cache.branches.insert(pair);
            }
        }
    }
}

void real_branch_history_manager_t::prepare_gc(
        std::set<branch_id_t> *branches_out)
        THROWS_NOTHING {
    branches_out->clear();
    for (const auto &pair : cache.branches) {
        branches_out->insert(pair.first);
    }
}

void real_branch_history_manager_t::perform_gc(
        const std::set<branch_id_t> &remove_branches)
        THROWS_NOTHING {
    cond_t non_interruptor;
    metadata_file_t::write_txn_t write_txn(metadata_file, &non_interruptor);
    for (const branch_id_t &bid : remove_branches) {
        if (is_branch_known(bid)) {
            write_txn.erase(
                mdprefix_branch_birth_certificate().suffix(
                    uuid_to_str(table_id) + "/" + uuid_to_str(bid)),
                &non_interruptor);
            cache.branches.erase(bid);
        }
    }
}

