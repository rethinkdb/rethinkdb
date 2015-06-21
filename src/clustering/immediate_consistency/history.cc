// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/history.hpp"

#include <stack>

#include "containers/binary_blob.hpp"

RDB_IMPL_SERIALIZABLE_2_SINCE_v1_13(version_t, branch, timestamp);

region_map_t<version_t> to_version_map(const region_map_t<binary_blob_t> &blob_map) {
    return blob_map.map(blob_map.get_domain(),
        [&](const binary_blob_t &b) -> version_t {
            return binary_blob_t::get<version_t>(b);
        });
}

region_map_t<binary_blob_t> from_version_map(const region_map_t<version_t> &vers_map) {
    return vers_map.map(vers_map.get_domain(), &binary_blob_t::make<version_t>);
}

RDB_IMPL_SERIALIZABLE_2_SINCE_v2_1(branch_birth_certificate_t,
    initial_timestamp, origin);
RDB_IMPL_EQUALITY_COMPARABLE_2(branch_birth_certificate_t,
    initial_timestamp, origin);

void branch_history_reader_t::export_branch_history(
        const branch_id_t &branch, branch_history_t *out) const THROWS_NOTHING {
    if (out->branches.count(branch) == 1) {
        return;
    }
    std::set<branch_id_t> to_process {branch};
    while (!to_process.empty()) {
        branch_id_t next = *to_process.begin();
        to_process.erase(to_process.begin());
        auto res = out->branches.insert(std::make_pair(next, get_branch(next)));
        guarantee(res.second);
        res.first->second.origin.visit(res.first->second.origin.get_domain(),
        [&](const region_t &, const version_t &vers) {
            if (!vers.branch.is_nil() && out->branches.count(vers.branch) == 0) {
                to_process.insert(vers.branch);
            }
        });
    }
}

void branch_history_reader_t::export_branch_history(
        const region_map_t<version_t> &region_map, branch_history_t *out)
        const THROWS_NOTHING {
    region_map.visit(region_map.get_domain(),
    [&](const region_t &, const version_t &vers) {
        if (!vers.branch.is_nil()) {
            export_branch_history(vers.branch, out);
        }
    });
}

branch_birth_certificate_t branch_history_t::get_branch(const branch_id_t &branch) const
        THROWS_ONLY(missing_branch_exc_t) {
    auto it = branches.find(branch);
    if (it == branches.end()) {
        throw missing_branch_exc_t();
    } else {
        return it->second;
    }
}

bool branch_history_t::is_branch_known(const branch_id_t &branch) const THROWS_NOTHING {
    return branches.count(branch) != 0;
}

RDB_IMPL_SERIALIZABLE_1_SINCE_v2_1(branch_history_t, branches);
RDB_IMPL_EQUALITY_COMPARABLE_1(branch_history_t, branches);

bool version_is_ancestor(
        const branch_history_reader_t *bh,
        const version_t &ancestor,
        const version_t &descendent,
        const region_t &relevant_region)
        THROWS_ONLY(missing_branch_exc_t) {
    std::stack<std::pair<region_t, version_t> > stack;
    stack.push(std::make_pair(relevant_region, descendent));
    bool missing = false;
    while (!stack.empty()) {
        region_t reg = stack.top().first;
        version_t vers = stack.top().second;
        stack.pop();
        if (vers.branch == ancestor.branch && vers.timestamp >= ancestor.timestamp) {
            /* OK, this part matches; we don't need to do any further checking here */
        } else if (vers.timestamp < ancestor.timestamp) {
            /* This part definitely doesn't match */
            return false;
        } else {
            branch_birth_certificate_t birth_certificate;
            try {
                birth_certificate = bh->get_branch(vers.branch);
                birth_certificate.origin.visit(reg,
                    [&](const region_t &subreg, const version_t &subvers) {
                        stack.push(std::make_pair(subreg, subvers));
                    });
            } catch (const missing_branch_exc_t &) {
                /* We don't want to throw `missing_branch_exc_t` yet because we might
                later encounter some part that definitely isn't a descendent of
                `ancestor`. In other words, there might still be a possibility to return
                `false`. */
                missing = true;
            }
        }
    }
    if (missing) {
        throw missing_branch_exc_t();
    } else {
        return true;
    }
}

/* Arbitrary comparison operator for `version_t`, so we can make sets of them. */
class version_set_less_t {
public:
    bool operator()(const version_t &a, const version_t &b) const {
        return std::tie(a.branch, a.timestamp) < std::tie(b.branch, b.timestamp);
    }
};

region_map_t<version_t> version_find_common(
        const branch_history_reader_t *bh,
        const version_t &initial_v1,
        const version_t &initial_v2,
        const region_t &initial_region)
        THROWS_ONLY(missing_branch_exc_t) {
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
        std::set<version_t, version_set_less_t> v1_equiv, v2_equiv;
    };
    std::stack<fragment_t> stack;
    std::vector<region_t> result_regions;
    std::vector<version_t> result_versions;
    fragment_t initial_fragment;
    initial_fragment.r = initial_region;
    initial_fragment.v1 = initial_v1;
    initial_fragment.v2 = initial_v2;
    stack.push(initial_fragment);
    while (!stack.empty()) {
        fragment_t x = stack.top();
        stack.pop();
        if (x.v1.branch == x.v2.branch) {
            /* This is the base case; we got to two versions which are on the same
            branch, so one of them is the common ancestor of both. */
            version_t common(x.v1.branch, std::min(x.v1.timestamp, x.v2.timestamp));
            result_regions.push_back(x.r);
            result_versions.push_back(common);
        } else if (x.v1_equiv.count(x.v2) == 1) {
            /* Alternative base case: `v1` and `v2` are equivalent. */
            result_regions.push_back(x.r);
            result_versions.push_back(x.v2);
        } else if (x.v2_equiv.count(x.v1) == 1) {
            result_regions.push_back(x.r);
            result_versions.push_back(x.v1);
        } else if (x.v1 == version_t::zero() || x.v2 == version_t::zero()) {
            /* Conceptually, this case is a subset of the next case. We handle it
            separately because `bh->get_branch()` crashes on a nil branch ID, so we'd
            have to make the next case implementation more complicated. */
            result_regions.push_back(x.r);
            result_versions.push_back(version_t::zero());
        } else {
            /* The versions are on two separate branches. */
            branch_birth_certificate_t b1 = bh->get_branch(x.v1.branch);
            branch_birth_certificate_t b2 = bh->get_branch(x.v2.branch);
            /* To simplify the algorithm, make sure that `b1` has a later start point or
            the two branches have equal start points */
            if (b1.initial_timestamp < b2.initial_timestamp) {
                std::swap(x.v1, x.v2);
                std::swap(x.v1_equiv, x.v2_equiv);
                std::swap(b1, b2);
            }
            /* One of two things are true (we don't know which):
            1. `b2` is not descended from `b1`. In this case, recursing into `b1`'s
                parents will get us closer to the common ancestor without taking us past
                it.
            2. The common ancestor is `b1`'s start point. (Or, if `v1` is `b1`'s start
                point, the common ancestor might be in `v1_equiv`.) In this case,
                recursing from `b1` to its parents will take `v1` past the common
                ancestor. But this is OK, because the common ancestor will still be
                recorded in `v1_equiv`. */

            /* Prepare the new value of `v1_equiv`. We know that `b1`'s start point
            belongs in `v1_equiv`. And if `x.v1` is equal to `b1`'s start point, then we
            should also include the previous values of `v1_equiv`. */
            std::set<version_t, version_set_less_t> v1_equiv;
            v1_equiv.insert(version_t(x.v1.branch, b1.initial_timestamp));
            if (x.v1.timestamp == b1.initial_timestamp) {
                v1_equiv.insert(x.v1_equiv.begin(), x.v1_equiv.end());
            }

            /* OK, now recurse to `b1`'s parents. */
            guarantee(region_is_superset(b1.origin.get_domain(), x.r));
            b1.origin.visit(x.r,
            [&](const region_t &reg, const version_t &vers) {
                stack.push({reg, vers, x.v2, v1_equiv, x.v2_equiv});
            });
        }
    }
    return region_map_t<version_t>::from_unordered_fragments(
        std::move(result_regions), std::move(result_versions));
}

region_map_t<version_t> version_find_branch_common(
        const branch_history_reader_t *bh,
        const version_t &version,
        const branch_id_t &branch,
        const region_t &relevant_region)
        THROWS_ONLY(missing_branch_exc_t) {
    return version_find_common(
        bh, version, version_t(branch, state_timestamp_t::max()), relevant_region);
}

branch_birth_certificate_t branch_history_combiner_t::get_branch(
        const branch_id_t &branch) const THROWS_ONLY(missing_branch_exc_t) {
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

