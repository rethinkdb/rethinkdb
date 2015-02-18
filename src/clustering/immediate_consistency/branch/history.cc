// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/branch/history.hpp"

#include <stack>

#include "containers/binary_blob.hpp"

RDB_IMPL_SERIALIZABLE_2_SINCE_v1_13(version_t, branch, timestamp);

region_map_t<version_t> to_version_map(const region_map_t<binary_blob_t> &blob_map) {
    return region_map_transform<binary_blob_t, version_t>(
        blob_map, &binary_blob_t::get<version_t>);
}

/* RSI(raft): This should be SINCE_N, where N is the version when Raft is released */
RDB_IMPL_SERIALIZABLE_3_SINCE_v1_16(branch_birth_certificate_t,
                        region, initial_timestamp, origin);
RDB_IMPL_EQUALITY_COMPARABLE_3(branch_birth_certificate_t,
                               region, initial_timestamp, origin);

void branch_history_reader_t::export_branch_history(
        const branch_id_t &branch, branch_history_t *out) const THROWS_NOTHING {
    if (out->branches.count(branch) == 1) {
        return;
    }
    out->branches[branch] = get_branch(branch);
    export_branch_history(out->branches[branch].origin, out);
}

void branch_history_reader_t::export_branch_history(
        const region_map_t<version_t> &region_map, branch_history_t *out)
        const THROWS_NOTHING {
    for (const auto &pair : region_map) {
        if (!pair.second.branch.is_nil()) {
            export_branch_history(pair.second.branch, out);
        }
    }
}

branch_birth_certificate_t branch_history_t::get_branch(const branch_id_t &branch)
        const THROWS_NOTHING {
    auto it = branches.find(branch);
    guarantee(it != branches.end(), "Branch is missing from branch history");
    return it->second;
}

bool branch_history_t::is_branch_known(const branch_id_t &branch) const THROWS_NOTHING {
    return branches.count(branch) != 0;
}

RDB_IMPL_SERIALIZABLE_1_SINCE_v1_16(branch_history_t, branches);
RDB_IMPL_EQUALITY_COMPARABLE_1(branch_history_t, branches);

bool version_is_ancestor(
        const branch_history_reader_t *bh,
        const version_t ancestor,
        version_t descendent,
        region_t relevant_region) {
    typedef region_map_t<version_t> version_map_t;
    // A stack of version maps and iterators pointing an the next element in the map to
    // traverse.
    std::stack<std::pair<version_map_t *, version_map_t::const_iterator> > origin_stack;

    // We break from this for loop when the version is shown not to be an ancestor.
    for (;;) {
        if (region_is_empty(relevant_region)) {
            // do nothing, continue to next part of tree.
        } else if (ancestor.branch.is_nil()) {
            /* Everything is descended from the root pseudobranch. */
            // do nothing, continue to next part of tree.
        } else if (descendent.branch.is_nil()) {
            /* The root psuedobranch is descended from nothing but itself. */
            // fail!
            break;
        } else if (ancestor.branch == descendent.branch) {
            if (ancestor.timestamp <= descendent.timestamp) {
                // do nothing, continue to next part of tree.
            } else {
                // fail!
                break;
            }
        } else {
            branch_birth_certificate_t descendent_branch_bc =
                bh->get_branch(descendent.branch);

            rassert(region_is_superset(descendent_branch_bc.region, relevant_region));
            guarantee(descendent.timestamp >= descendent_branch_bc.initial_timestamp);

            version_map_t *relevant_origin =
                new version_map_t(descendent_branch_bc.origin.mask(relevant_region));
            guarantee(relevant_origin->begin() != relevant_origin->end());

            origin_stack.push(std::make_pair(relevant_origin, relevant_origin->begin()));
        }

        if (origin_stack.empty()) {
            // We've navigated the entire tree and succeed in seeing version_is_ancestor
            // for all children.  Yay.
            return true;
        }

        version_map_t::const_iterator it = origin_stack.top().second;
        descendent = it->second;
        relevant_region = it->first;

        ++it;
        if (it == origin_stack.top().first->end()) {
            delete origin_stack.top().first;
            origin_stack.pop();
        } else {
            origin_stack.top().second = it;
        }
    }

    // We failed.  We need to clean up some memory.
    while (!origin_stack.empty()) {
        delete origin_stack.top().first;
        origin_stack.pop();
    }

    return false;
}

region_map_t<version_t> version_find_common(
        const branch_history_reader_t *bh,
        const version_t &initial_v1,
        const version_t &initial_v2,
        const region_t &initial_region) {
    /* In order to limit recursion depth in pathological cases, we use a heap-stack
    instead of the real stack. `stack` stores the sub-regions left to process and
    `result` stores the sub-regions that we've already solved. We repeatedly pop
    fragments from `stack`; for each fragment, we either add one or more new
    sub-fragments to `stack`, or add an entry to `result`. After each iteration of the
    loop, the union of the regions in `stack` and the regions in `result` is
    `initial_region`. */
    class fragment_t {
    public:
        /* The region that this fragment applies to */
        region_t r;
        /* The versions we're trying to find the common ancestor of in the region */
        version_t v1, v2;
        /* Versions equivalent to `v1` and `v2` in the region, which the initial values
        of `v1` or `v2` are descended from. We need to track these to deal with an
        awkward corner case; see below. */
        std::set<version_t> v1_equiv, v2_equiv;
    };
    std::stack<fragment_t> stack;
    std::vector<std::pair<region_t, state_timestamp_t> > result;
    stack.push_back({initial_region, initial_v1, initial_v2, {}, {}});
    while (!stack.empty()) {
        fragment_t x = stack.top();
        stack.pop();
        if (x.v1.branch == x.v2.branch) {
            /* This is the base case; we got to two versions which are on the same
            branch, so one of them is the common ancestor of both. */
            version_t common(x.v1.branch, std::min(x.v1.timestamp, x.v2.timestamp));
            result.push_back({x.r, common});
        } else if (x.v1_equivs.count(x.v2) == 1) {
            /* Alternative base case: `v1` and `v2` are equivalent. */
            result.push_back({x.r, x.v2});
        } else if (x.v2_equivs.count(x.v1) == 1) {
            result.push_back({x.r, x.v1});
        } else if (x.v1 == version_t::zero() || x.v2 == version_t::zero()) {
            /* Conceptually, this case is a subset of the next case. We handle it
            separately because `bh->get_branch()` crashes on a nil branch ID, so we'd
            have to make the third case implementation more complicated. */
            result.push_back({x.r, version_t::zero()});
        } else {
            /* The versions are on two separate branches. */
            branch_birth_certificate_t b1 = bh->get_branch(x.v1.branch);
            branch_birth_certificate_t b2 = bh->get_branch(x.v2.branch);
            /* To simplify the algorithm, make sure that `b1` has a later start point or
            the two branches have equal start points */
            if (b1.initial_timestamp < b2.initial_timestamp) {
                std::swap(x.v1, x.v2);
                std::swap(x.v1_equivs, x.v2_equivs);
                std::swap(b1, b2);
            }
            /* One of two things are true (we don't know which):
            1. `b2` is not descended from `b1`. In this case, recursing into `b1`'s
                parents will get us closer to the common ancestor without taking us past
                it.
            2. The common ancestor is `b1`'s start point. (Or, if `v1` is `b1`'s start
                point, the common ancestor might be in `v1_equivs`.) In this case,
                recursing from `b1` to its parents will take `v1` past the common
                ancestor. But this is OK, because the common ancestor will still be
                recorded in `v1_equivs`. */

            /* Prepare the new value of `v1_equivs`. We know that `b1`'s start point
            belongs in `v1_equivs`. And if `x.v1` is equal to `b1`'s start point, then we
            should also include the previous values of `v1_equivs`. */
            std::set<version_t> v1_equivs;
            v1_equivs.insert(version_t(x.v1.branch, b1.initial_timestamp));
            if (x.v1.timestamp == b1.initial_timestamp) {
                v1_equivs.insert(x.v1_equivs.begin(), x.v1_equivs.end());
            }

            /* OK, now recurse to `b1`'s parents. */
            for (const auto &pair : b1.origin.mask(x.r)) {
                stack.push({pair.first, pair.second, x.v2, v1_equivs, x.v2_equivs});
            }
        }
    }
    return region_map_t<version_t>(std::move(result));
}

region_map_t<version_t> version_find_branch_common(
        const branch_history_reader_t *bh,
        const version_t &version,
        const branch_id_t &branch,
        const region_t &relevant_region) {
    return version_find_common(
        bh, version, version_t(branch, state_timestamp_t::max()), relevant_region);
}

branch_birth_certificate_t branch_history_combiner_t::get_branch(
        const branch_id_t &branch) const THROWS_NOTHING {
    if (r1->is_branch_known(branch)) {
        return r1->get_branch(branch);
    } else {
        return r2->get_branch(branch);
    }
}

bool branch_history_combiner_t::is_branch_known(
        const branch_id_t &branch) const THROWS_NOTHING {
    return r1->is_branch_known(branch) || r2->is_branch_known(branch);
}

