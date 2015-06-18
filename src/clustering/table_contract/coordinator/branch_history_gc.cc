// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_contract/coordinator/branch_history_gc.hpp"

void copy_branch_history_for_branch(
        const branch_id_t &new_branch,
        const branch_history_t &source,
        const table_raft_state_t &old_state,
        branch_history_t *add_branches_out) {
    const branch_history_t &existing = old_state.branch_history;
    std::set<branch_id_t> todo;
    if (existing.branches.count(root) == 0 && add_out->branches.count(root) == 0) {
        todo.insert(root);
    }
    while (!todo.empty()) {
        branch_id_t b = *todo.begin();
        todo.erase(todo.begin());
        const branch_birth_certificate_t &bc = source.branches.at(b);
        add_out->branches.insert(std::make_pair(b, bc));
        bc.origin.visit(bc.origin.get_domain(),
        [&](const region_t &, const version_t &version) {
            if (version != version_t::zero() &&
                    existing.branches.count(version.branch) == 0 &&
                    add_out->branches.count(version.branch) == 0) {
                todo.insert(version.branch);
            }
        });
    }
}


