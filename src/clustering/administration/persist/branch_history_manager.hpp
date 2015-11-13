// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_PERSIST_BRANCH_HISTORY_MANAGER_HPP_
#define CLUSTERING_ADMINISTRATION_PERSIST_BRANCH_HISTORY_MANAGER_HPP_

#include "clustering/administration/persist/file.hpp"
#include "clustering/immediate_consistency/history.hpp"

class real_branch_history_manager_t : public branch_history_manager_t {
public:
    static void erase(
        metadata_file_t::write_txn_t *write_txn,
        const namespace_id_t &_table_id);

    real_branch_history_manager_t(
        const namespace_id_t &_table_id,
        metadata_file_t *_metadata_file,
        metadata_file_t::read_txn_t *metadata_read_txn,
        signal_t *interruptor);

    branch_birth_certificate_t get_branch(const branch_id_t &branch)
        const THROWS_ONLY(missing_branch_exc_t);
    bool is_branch_known(const branch_id_t &branch) const THROWS_NOTHING;
    void create_branch(
        branch_id_t branch_id,
        const branch_birth_certificate_t &bc)
        THROWS_NOTHING;
    void import_branch_history(
        const branch_history_t &new_records)
        THROWS_NOTHING;
    void prepare_gc(
        std::set<branch_id_t> *branches_out)
        THROWS_NOTHING;
    void perform_gc(
        const std::set<branch_id_t> &remove_branches)
        THROWS_NOTHING;

private:
    namespace_id_t const table_id;
    metadata_file_t * const metadata_file;
    branch_history_t cache;
};

#endif /* CLUSTERING_ADMINISTRATION_PERSIST_BRANCH_HISTORY_MANAGER_HPP_ */

