// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "unittest/branch_history_manager.hpp"

#include <map>
#include <set>

#include "arch/timing.hpp"
#include "rdb_protocol/protocol.hpp"

namespace unittest {

branch_birth_certificate_t in_memory_branch_history_manager_t::get_branch(
        const branch_id_t &branch) const THROWS_NOTHING {
    std::map<branch_id_t, branch_birth_certificate_t>::const_iterator it = bh.branches.find(branch);
    rassert(it != bh.branches.end(), "no such branch");
    return it->second;
}

bool in_memory_branch_history_manager_t::is_branch_known(
        const branch_id_t &branch) const THROWS_NOTHING {
    return bh.branches.count(branch) > 0;
}

void in_memory_branch_history_manager_t::create_branch(
        branch_id_t branch_id,
        const branch_birth_certificate_t &bc,
        signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    rassert(bh.branches.find(branch_id) == bh.branches.end());
    nap(10, interruptor);
    bh.branches[branch_id] = bc;
}

void in_memory_branch_history_manager_t::import_branch_history(
        const branch_history_t &new_records, signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    nap(10, interruptor);
    for (const auto &pair : new_records.branches) {
        bh.branches.insert(std::make_pair(pair.first, pair.second));
    }
}

}  // namespace unittest
