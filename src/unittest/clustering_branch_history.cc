// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "clustering/immediate_consistency/history.hpp"
#include "containers/uuid.hpp"
#include "rdb_protocol/protocol.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

struct quick_version_map_args_t {
public:
    const char *quick_range_spec;
    const branch_id_t &branch;
    int timestamp;
};
region_map_t<version_t> quick_version_map(
        std::initializer_list<quick_version_map_args_t> qvms) {
    std::vector<region_t> region_vector;
    std::vector<version_t> version_vector;
    for (const quick_version_map_args_t &qvm : qvms) {
        region_vector.push_back(region_t(quick_range(qvm.quick_range_spec)));
        version_vector.push_back(
            version_t(qvm.branch, state_timestamp_t::from_num(qvm.timestamp)));
    }
    return region_map_t<version_t>::from_unordered_fragments(
        std::move(region_vector), std::move(version_vector));
}

version_t quick_vers(const branch_id_t &branch, int timestamp) {
    return version_t(branch, state_timestamp_t::from_num(timestamp));
}

branch_id_t quick_branch(
        branch_history_t *bhist,
        std::initializer_list<quick_version_map_args_t> origin) {
    branch_birth_certificate_t bc;
    bc.origin = quick_version_map(origin);
    bc.region = bc.origin.get_domain();
    bc.initial_timestamp = state_timestamp_t::zero();
    bc.origin.visit(bc.region, [&](const region_t &, const version_t &version) {
        bc.initial_timestamp = std::max(bc.initial_timestamp, version.timestamp);
    });
    branch_id_t bid = generate_uuid();
    bhist->branches.insert(std::make_pair(bid, bc));
    return bid;
}

TPTEST(ClusteringBranchHistory, IsAncestor) {
    branch_history_t bh;
    branch_id_t b1 = quick_branch(&bh, { {"A-Z", nil_uuid(), 0} });
    branch_id_t b2 = quick_branch(&bh, { {"A-M", b1, 123} });
    branch_id_t b3 = quick_branch(&bh, { {"N-Z", b1, 456} });
    branch_id_t b4 = quick_branch(&bh, { {"A-M", b2, 1000}, {"N-Z", b3, 1000} });

    /* Simplest case: two versions on the same branch */
    EXPECT_TRUE(version_is_ancestor(&bh,
        quick_vers(b1, 10), quick_vers(b1, 11), quick_region("A-Z")));
    EXPECT_FALSE(version_is_ancestor(&bh,
        quick_vers(b1, 11), quick_vers(b1, 10), quick_region("A-Z")));

    /* Versions related through several branches */
    EXPECT_TRUE(version_is_ancestor(&bh,
        quick_vers(b1, 34), quick_vers(b4, 1002),  quick_region("A-Z")));
    EXPECT_TRUE(version_is_ancestor(&bh,
        version_t::zero(), quick_vers(b4, 1002), quick_region("A-Z")));
    EXPECT_TRUE(version_is_ancestor(&bh,
        quick_vers(b1, 10), quick_vers(b1, 10), quick_region("A-Z")));

    /* Ancestor-ness might be different for different regions */
    EXPECT_FALSE(version_is_ancestor(&bh,
        quick_vers(b1, 124), quick_vers(b4, 1002), quick_region("A-Z")));
    EXPECT_FALSE(version_is_ancestor(&bh,
        quick_vers(b1, 124), quick_vers(b4, 1002), quick_region("A-M")));
    EXPECT_TRUE(version_is_ancestor(&bh,
        quick_vers(b1, 124), quick_vers(b4, 1002), quick_region("N-Z")));

    /* The first version of a new branch is the descendent of the origin, but not vis
    versa. */
    EXPECT_TRUE(version_is_ancestor(&bh,
        quick_vers(b1, 123), quick_vers(b2, 123), quick_region("A-M")));
    EXPECT_FALSE(version_is_ancestor(&bh,
        quick_vers(b2, 123), quick_vers(b1, 123), quick_region("A-M")));
}

TPTEST(ClusteringBranchHistory, CommonAncestor) {
    branch_history_t bh;
    branch_id_t b1 = quick_branch(&bh, { {"A-Z", nil_uuid(), 0} });
    branch_id_t b2 = quick_branch(&bh, { {"A-Z", b1, 123} });
    branch_id_t b3 = quick_branch(&bh, { {"A-Z", b1, 456} });

    /* Simplest case: two versions on the same branch */
    EXPECT_EQ(quick_version_map({ {"A-Z", b1, 123} }),
        version_find_common(&bh,
            quick_vers(b1, 123), quick_vers(b1, 456), quick_region("A-Z")));
    EXPECT_EQ(quick_version_map({ {"A-Z", b1, 123} }),
        version_find_common(&bh,
            quick_vers(b1, 456), quick_vers(b1, 123), quick_region("A-Z")));

    /* Versions on different branches */
    EXPECT_EQ(quick_version_map({ {"A-Z", b1, 123} }),
        version_find_common(&bh,
            quick_vers(b1, 456), quick_vers(b2, 456), quick_region("A-Z")));
    EXPECT_EQ(quick_version_map({ {"A-Z", b1, 123} }),
        version_find_common(&bh,
            quick_vers(b2, 456), quick_vers(b1, 456), quick_region("A-Z")));
    EXPECT_EQ(quick_version_map({ {"A-Z", b1, 123} }),
        version_find_common(&bh,
            quick_vers(b2, 456), quick_vers(b3, 789), quick_region("A-Z")));
    EXPECT_EQ(quick_version_map({ {"A-Z", b1, 123} }),
        version_find_common(&bh,
            quick_vers(b3, 789), quick_vers(b2, 456), quick_region("A-Z")));

    /* The `version_find_branch_common()` variant */
    EXPECT_EQ(quick_version_map({ {"A-Z", b1, 123} }),
        version_find_branch_common(&bh,
            quick_vers(b2, 456), b1, quick_region("A-Z")));
    EXPECT_EQ(quick_version_map({ {"A-Z", b1, 89} }),
        version_find_branch_common(&bh,
            quick_vers(b1, 89), b1, quick_region("A-Z")));
    EXPECT_EQ(quick_version_map({ {"A-Z", b1, 123} }),
        version_find_branch_common(&bh,
            quick_vers(b1, 456), b2, quick_region("A-Z")));
}

TPTEST(ClusteringBranchHistory, CommonAncestorSplit) {
    branch_history_t bh;
    branch_id_t b1l = quick_branch(&bh, { {"A-M", nil_uuid(), 0} });
    branch_id_t b1r = quick_branch(&bh, { {"N-Z", nil_uuid(), 0} });
    branch_id_t b2 = quick_branch(&bh, { {"A-M", b1l, 123}, {"N-Z", b1r, 456} });
    branch_id_t b3 = quick_branch(&bh, { {"A-M", b1l, 456}, {"N-Z", b1r, 123} });

    EXPECT_EQ(quick_version_map({ {"A-M", b1l, 123}, {"N-Z", b1r, 123} }),
        version_find_common(&bh,
            quick_vers(b2, 789), quick_vers(b3, 789), quick_region("A-Z")));
}

TPTEST(ClusteringBranchHistory, CommonAncestorSameOrigin) {
    /* Visualization:
    nil
    +-b1
      +-b11
      | +-b111
      | | +-b1111
      | | +-b1112
      +-b12
    All of the branches descending from `b1` have exactly the same starting timestamp.
    This hits a weird corner case in `version_find_common()`. */
    branch_history_t bh;
    branch_id_t b1 = quick_branch(&bh, { {"A-Z", nil_uuid(), 0} });
    branch_id_t b11 = quick_branch(&bh, { {"A-Z", b1, 123} });
    branch_id_t b12 = quick_branch(&bh, { {"A-Z", b1, 123} });
    branch_id_t b111 = quick_branch(&bh, { {"A-Z", b11, 123} });
    branch_id_t b1111 = quick_branch(&bh, { {"A-Z", b111, 123} });
    branch_id_t b1112 = quick_branch(&bh, { {"A-Z", b111, 123} });

    EXPECT_EQ(quick_version_map({ {"A-Z", b1, 123} }),
        version_find_common(&bh,
            quick_vers(b11, 456), quick_vers(b12, 456), quick_region("A-Z")));
    EXPECT_EQ(quick_version_map({ {"A-Z", b111, 123} }),
        version_find_common(&bh,
            quick_vers(b1111, 456), quick_vers(b1112, 456), quick_region("A-Z")));
    EXPECT_EQ(quick_version_map({ {"A-Z", b111, 123} }),
        version_find_common(&bh,
            quick_vers(b1111, 456), quick_vers(b1112, 456), quick_region("A-Z")));
    EXPECT_EQ(quick_version_map({ {"A-Z", b11, 123} }),
        version_find_common(&bh,
            quick_vers(b11, 456), quick_vers(b1112, 456), quick_region("A-Z")));
}

}   /* namespace unittest */
