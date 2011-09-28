#ifndef __UNITTEST_CLUSTERING_DUMMIES_HPP__
#define __UNITTEST_CLUSTERING_DUMMIES_HPP__

#include "clustering/version.hpp"
#include "unittest/dummy_protocol.hpp"

namespace unittest {

/* `dummy_branch_history_tracker_t` is a very simple `branch_history_database_t`
that has a single fixed branch. */

class dummy_branch_history_database_t : public branch_history_database_t<dummy_protocol_t> {
public:
    dummy_branch_history_database_t() {
        for (char c = 'a'; c <= 'z'; c++) region.keys.insert(std::string(1, c));
        branch_id = generate_uuid();
    }

    bool version_is_ancestor(version_t anc, version_t desc, dummy_protocol_t::region_t r) {
        rassert(region_is_superset(region, r));
        rassert(anc.branch == branch_id || anc.branch.is_nil());
        rassert((anc.timestamp == state_timestamp_t::zero()) == anc.branch.is_nil());
        rassert(desc.branch == branch_id || desc.branch.is_nil());
        rassert((desc.timestamp == state_timestamp_t::zero()) == desc.branch.is_nil());
        return desc.timestamp >= anc.timestamp;
    }

    dummy_protocol_t::region_t region;
    branch_id_t branch_id;
};

}   /* namespace unittest */

#endif /* __UNITTEST_CLUSTERING_DUMMIES_HPP__ */
