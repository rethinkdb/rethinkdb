// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef MOCK_BRANCH_HISTORY_MANAGER_TCC_
#define MOCK_BRANCH_HISTORY_MANAGER_TCC_

#include "mock/branch_history_manager.hpp"

#include <map>
#include <set>

#include "arch/timing.hpp"

namespace mock {

template <class protocol_t>
branch_birth_certificate_t<protocol_t> in_memory_branch_history_manager_t<protocol_t>::get_branch(branch_id_t branch) THROWS_NOTHING {
    typename std::map<branch_id_t, branch_birth_certificate_t<protocol_t> >::const_iterator it = bh.branches.find(branch);
    rassert(it != bh.branches.end(), "no such branch");
    return it->second;
}

template <class protocol_t>
std::set<branch_id_t> in_memory_branch_history_manager_t<protocol_t>::known_branches() THROWS_NOTHING {
    std::set<branch_id_t> res;

    for (typename std::map<branch_id_t, branch_birth_certificate_t<protocol_t> >::iterator it  = bh.branches.begin();
                                                                                           it != bh.branches.end();
                                                                                           ++it) {
        res.insert(it->first);
    }

    return res;
}

template <class protocol_t>
void in_memory_branch_history_manager_t<protocol_t>::create_branch(branch_id_t branch_id, const branch_birth_certificate_t<protocol_t> &bc, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    rassert(bh.branches.find(branch_id) == bh.branches.end());
    nap(10, interruptor);
    bh.branches[branch_id] = bc;
}

template <class protocol_t>
void in_memory_branch_history_manager_t<protocol_t>::export_branch_history(branch_id_t branch, branch_history_t<protocol_t> *out) THROWS_NOTHING {
    rassert(!branch.is_nil());
    std::set<branch_id_t> to_process;
    if (out->branches.count(branch) == 0) {
        to_process.insert(branch);
    }
    while (!to_process.empty()) {
        branch_id_t next = *to_process.begin();
        to_process.erase(next);
        branch_birth_certificate_t<protocol_t> bc = get_branch(next);
        rassert(out->branches.count(next) == 0);
        out->branches[next] = bc;
        for (typename region_map_t<protocol_t, version_range_t>::const_iterator it = bc.origin.begin(); it != bc.origin.end(); it++) {
            if (!it->second.latest.branch.is_nil() && out->branches.count(it->second.latest.branch) == 0) {
                to_process.insert(it->second.latest.branch);
            }
        }
    }
}

template <class protocol_t>
void in_memory_branch_history_manager_t<protocol_t>::import_branch_history(const branch_history_t<protocol_t> &new_records, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    nap(10, interruptor);
    for (typename std::map<branch_id_t, branch_birth_certificate_t<protocol_t> >::const_iterator it = new_records.branches.begin(); it != new_records.branches.end(); it++) {
        bh.branches.insert(std::make_pair(it->first, it->second));
    }
}

}   /* namespace mock */

#endif /* MOCK_BRANCH_HISTORY_MANAGER_TCC_ */
