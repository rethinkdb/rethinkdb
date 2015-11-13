// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef UNITTEST_BRANCH_HISTORY_MANAGER_HPP_
#define UNITTEST_BRANCH_HISTORY_MANAGER_HPP_

#include <set>

#include "clustering/immediate_consistency/history.hpp"


namespace unittest {

class in_memory_branch_history_manager_t : public branch_history_manager_t {
public:
    in_memory_branch_history_manager_t() { }
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
    branch_history_t bh;

    DISABLE_COPYING(in_memory_branch_history_manager_t);
};

}  // namespace unittest

#endif  // UNITTEST_BRANCH_HISTORY_MANAGER_HPP_
