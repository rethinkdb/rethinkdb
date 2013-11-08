// Copyright 2010-2013 RethinkDB, all rights reserved.
#include <algorithm>
#include <vector>

#include "unittest/gtest.hpp"

#include "btree/internal_node.hpp"
#include "btree/node.hpp"

namespace unittest {

void verify(block_size_t block_size, const internal_node_t *buf) {
    EXPECT_TRUE(buf->magic == internal_node_t::expected_magic);

    // Internal nodes must have at least one pair.
    ASSERT_LE(1, buf->npairs);

    // Thus buf->npairs - 1 >= 0.
    const uint16_t last_pair_offset = buf->pair_offsets[buf->npairs - 1];

    ASSERT_LE(buf->npairs, block_size.value());  // sanity checking to prevent overflow
    ASSERT_LE(offsetof(internal_node_t, pair_offsets) + sizeof(*buf->pair_offsets) * buf->npairs, buf->frontmost_offset);
    ASSERT_LE(buf->frontmost_offset, block_size.value());

    std::vector<uint16_t> offsets(buf->pair_offsets, buf->pair_offsets + buf->npairs);
    std::sort(offsets.begin(), offsets.end());

    uint16_t expected = buf->frontmost_offset;
    for (std::vector<uint16_t>::const_iterator p = offsets.begin(), e = offsets.end(); p < e; ++p) {
        ASSERT_LE(expected, block_size.value());
        ASSERT_EQ(expected, *p);
        expected += internal_node::pair_size(internal_node::get_pair(buf, *p));
    }
    ASSERT_EQ(block_size.value(), expected);

    const btree_key_t *last_key = NULL;
    for (const uint16_t *p = buf->pair_offsets, *e = p + buf->npairs - 1; p < e; ++p) {
        const btree_internal_pair *pair = internal_node::get_pair(buf, *p);
        const btree_key_t *next_key = &pair->key;

        if (last_key != NULL) {
            EXPECT_LT(internal_key_comp::compare(last_key, next_key), 0);
        }

        last_key = next_key;
    }

    EXPECT_EQ(0, internal_node::get_pair(buf, last_pair_offset)->key.size);
}

TEST(InternalNodeTest, Offsets) {
    EXPECT_EQ(0u, offsetof(internal_node_t, magic));
    EXPECT_EQ(4u, offsetof(internal_node_t, npairs));
    EXPECT_EQ(6u, offsetof(internal_node_t, frontmost_offset));
    EXPECT_EQ(8u, offsetof(internal_node_t, pair_offsets));
    EXPECT_EQ(8u, sizeof(internal_node_t));

    EXPECT_EQ(0u, offsetof(btree_internal_pair, lnode));
    // 8u depends on sizeof(block_id_t).
    EXPECT_EQ(8u, offsetof(btree_internal_pair, key));
    EXPECT_EQ(9u, sizeof(btree_internal_pair));
}


}  // namespace unittest

