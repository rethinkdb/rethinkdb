// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_contract/branch_history_gc.hpp"

#include "logger.hpp"

void copy_branch_history_for_branch(
        const branch_id_t &root,
        const branch_history_t &source,
        const table_raft_state_t &old_state,
        bool ignore_missing,
        branch_history_t *add_branches_out) {
    const branch_history_t &existing = old_state.branch_history;
    std::set<branch_id_t> todo;
    if (existing.branches.count(root) == 0 &&
            add_branches_out->branches.count(root) == 0) {
        todo.insert(root);
    }
    while (!todo.empty()) {
        branch_id_t b = *todo.begin();
        todo.erase(todo.begin());
        const auto bc_it = source.branches.find(b);
        if (bc_it == source.branches.end()) {
            rassert(ignore_missing);
            if (!ignore_missing) {
                /* In release mode we just log a message if a birth certificate
                is missing, and hope that we can recover from this by ignoring
                the branch. */
                logERR("Ignoring missing branch `%s`. This probably means that "
                       "there is a bug in RethinkDB. Please report this at "
                       "https://github.com/rethinkdb/rethinkdb/issues/",
                       uuid_to_str(b).c_str());
            }
            continue;
        }
        const branch_birth_certificate_t &bc = bc_it->second;
        add_branches_out->branches.insert(std::make_pair(b, bc));
        bc.origin.visit(bc.origin.get_domain(),
        [&](const region_t &, const version_t &version) {
            if (version != version_t::zero() &&
                    existing.branches.count(version.branch) == 0 &&
                    add_branches_out->branches.count(version.branch) == 0) {
                todo.insert(version.branch);
            }
        });
    }
}

void mark_all_ancestors_live(
        const branch_id_t &root,
        const region_t &region,
        const branch_history_reader_t *branch_reader,
        std::set<branch_id_t> *remove_branches_out) {
    guarantee(!root.is_nil());
    std::multimap<branch_id_t, region_t> todo;
    std::set<branch_id_t> done;
    todo.insert(std::make_pair(root, region));
    while (!todo.empty()) {
        std::pair<branch_id_t, region_t> next = *todo.begin();
        todo.erase(todo.begin());
        done.insert(next.first);
        remove_branches_out->erase(next.first);
        branch_reader->get_branch(next.first).origin.visit(next.second,
        [&](const region_t &subregion, const version_t &version) {
            if (version != version_t::zero() &&
                    branch_reader->is_branch_known(version.branch) &&
                    done.count(version.branch) == 0) {
                todo.insert(std::make_pair(version.branch, subregion));
            }
        });
    }
}

void mark_ancestors_since_base_live(
        const branch_id_t &root,
        const region_t &region,
        const branch_history_reader_t *branch_reader,
        const branch_history_reader_t *base,
        std::set<branch_id_t> *remove_branches_out) {
    guarantee(!root.is_nil());
    std::multimap<branch_id_t, region_t> todo;
    std::set<branch_id_t> done;
    todo.insert(std::make_pair(root, region));
    while (!todo.empty()) {
        std::pair<branch_id_t, region_t> next = *todo.begin();
        todo.erase(todo.begin());
        done.insert(next.first);
        remove_branches_out->erase(next.first);
        bool next_in_base = base->is_branch_known(next.first);
        branch_reader->get_branch(next.first).origin.visit(next.second,
        [&](const region_t &subregion, const version_t &version) {
            if (version != version_t::zero() &&
                    branch_reader->is_branch_known(version.branch) &&
                    done.count(version.branch) == 0) {
                bool version_in_base = base->is_branch_known(version.branch);
                if (!(next_in_base && !version_in_base)) {
                    todo.insert(std::make_pair(version.branch, subregion));
                }
            }
        });
    }
}

