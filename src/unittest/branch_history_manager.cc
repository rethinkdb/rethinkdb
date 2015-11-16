// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "unittest/branch_history_manager.hpp"

#include <map>
#include <set>

#include "arch/timing.hpp"
#include "rdb_protocol/protocol.hpp"

namespace unittest {

branch_birth_certificate_t in_memory_branch_history_manager_t::get_branch(
        const branch_id_t &branch) const THROWS_ONLY(missing_branch_exc_t) {
    assert_thread();
    return bh.get_branch(branch);
}

bool in_memory_branch_history_manager_t::is_branch_known(
        const branch_id_t &branch) const THROWS_NOTHING {
    assert_thread();
    return bh.is_branch_known(branch);
}

void in_memory_branch_history_manager_t::create_branch(
        branch_id_t branch_id,
        const branch_birth_certificate_t &bc)
        THROWS_NOTHING {
    assert_thread();
    if (bh.branches.find(branch_id) == bh.branches.end()) {
        nap(10);
        bh.branches[branch_id] = bc;
    }
}

void in_memory_branch_history_manager_t::import_branch_history(
        const branch_history_t &new_records)
        THROWS_NOTHING {
    assert_thread();
    nap(10);
    for (const auto &pair : new_records.branches) {
        bh.branches.insert(std::make_pair(pair.first, pair.second));
    }
}

void in_memory_branch_history_manager_t::prepare_gc(
        std::set<branch_id_t> *branches_out)
        THROWS_NOTHING {
    assert_thread();
    for (const auto &pair : bh.branches) {
        branches_out->insert(pair.first);
    }
}

void in_memory_branch_history_manager_t::perform_gc(
        const std::set<branch_id_t> &remove_branches)
        THROWS_NOTHING {
    assert_thread();
    nap(10);
    for (const branch_id_t &bid : remove_branches) {
        bh.branches.erase(bid);
    }
}

}  // namespace unittest
