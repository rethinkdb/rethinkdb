#include <algorithm>
#include <vector>

#include "unittest/gtest.hpp"

#include "btree/node.hpp"
#include "btree/leaf_node.hpp"

namespace unittest {

// TODO: This is rather duplicative of fsck::check_value.
void expect_valid_value_shallowly(const btree_value *value) {
    EXPECT_TRUE(!(value->metadata_flags & ~(MEMCACHED_FLAGS | MEMCACHED_CAS | MEMCACHED_EXPTIME | LARGE_VALUE)));

    size_t size = value->value_size();

    if (value->is_large()) {
        EXPECT_GT(size, MAX_IN_NODE_VALUE_SIZE);

        // This isn't fsck, we can't follow large buf pointers.
    } else {
        EXPECT_LE(size, MAX_IN_NODE_VALUE_SIZE);
    }
}

// TODO: This is rather duplicative of fsck::check_subtree_leaf_node.
void verify(block_size_t block_size, const leaf_node_t *buf) {

    ASSERT_LE(offsetof(leaf_node_t, pair_offsets) + buf->npairs * 2, buf->frontmost_offset);
    ASSERT_LE(buf->frontmost_offset, block_size.value());

    std::vector<uint16_t> offsets(buf->pair_offsets, buf->pair_offsets + buf->npairs);
    std::sort(offsets.begin(), offsets.end());

    uint16_t expected = buf->frontmost_offset;
    for (std::vector<uint16_t>::const_iterator p = offsets.begin(), e = offsets.end(); p < e; ++p) {
        ASSERT_LE(expected, block_size.value());
        ASSERT_EQ(expected, *p);
        expected += leaf_node_handler::pair_size(leaf_node_handler::get_pair(buf, *p));
    }
    ASSERT_EQ(block_size.value(), expected);

    const btree_key *last_key = NULL;
    for (const uint16_t *p = buf->pair_offsets, *e = p + buf->npairs; p < e; ++p) {
        const btree_leaf_pair *pair = leaf_node_handler::get_pair(buf, *p);
        if (last_key != NULL) {
            const btree_key *next_key = &pair->key;
            EXPECT_LT(leaf_key_comp::compare(last_key, next_key), 0);
            last_key = next_key;
        }
        expect_valid_value_shallowly(pair->value());
    }
}

TEST(LeafNodeTest, Offsets) {
    // Check leaf_node_t.
    EXPECT_EQ(0, offsetof(leaf_node_t, magic));
    EXPECT_EQ(4, offsetof(leaf_node_t, times));
    EXPECT_EQ(12, offsetof(leaf_node_t, npairs));
    EXPECT_EQ(14, offsetof(leaf_node_t, frontmost_offset));
    EXPECT_EQ(16, offsetof(leaf_node_t, pair_offsets));
    EXPECT_EQ(16, sizeof(leaf_node_t));


    // Check leaf_timestamps_t, since that's specifically part of leaf_node_t.
    EXPECT_EQ(0, offsetof(leaf_timestamps_t, last_modified));
    EXPECT_EQ(4, offsetof(leaf_timestamps_t, earlier));

    // Changing NUM_LEAF_NODE_EARLIER_TIMES changes the disk format.
    EXPECT_EQ(2, NUM_LEAF_NODE_EARLIER_TIMES);

    EXPECT_EQ(8, sizeof(leaf_timestamps_t));


    // Check btree_leaf_pair.
    EXPECT_EQ(0, offsetof(btree_leaf_pair, key));

    btree_leaf_pair p;
    p.key.size = 173;
    EXPECT_EQ(174, reinterpret_cast<byte *>(p.value()) - reinterpret_cast<byte *>(&p));
}

block_size_t make_bs() {
    // TODO: Test weird block sizes.
    return block_size_t::unsafe_make(4096);
}

leaf_node_t *malloc_leaf_node(block_size_t bs) {
    return reinterpret_cast<leaf_node_t *>(malloc(bs.value()));
}

btree_key *malloc_key(const char *s) {
    size_t origlen = strlen(s);
    EXPECT_LE(origlen, MAX_KEY_SIZE);

    size_t len = std::min<size_t>(origlen, MAX_KEY_SIZE);
    btree_key *k = reinterpret_cast<btree_key *>(malloc(sizeof(btree_key) + len));
    k->size = len;
    memcpy(k->contents, s, len);
    return k;
}

btree_value *malloc_value(const char *s) {
    size_t origlen = strlen(s);
    EXPECT_LE(origlen, MAX_IN_NODE_VALUE_SIZE);

    size_t len = std::min<size_t>(origlen, MAX_IN_NODE_VALUE_SIZE);

    btree_value *v = reinterpret_cast<btree_value *>(malloc(sizeof(btree_value) + len));
    v->size = len;
    v->metadata_flags = 0;
    memcpy(v->value(), s, len);
    return v;
}

TEST(LeafNodeTest, Initialization) {
    block_size_t bs = make_bs();
    leaf_node_t *node = malloc_leaf_node(bs);

    leaf_node_handler::init(bs, node, repli_timestamp::invalid);
    verify(bs, node);
    free(node);
}

// Runs an InsertLookupRemove test with key and value.
void InsertLookupRemoveHelper(const char *key, const char *value) {
    block_size_t bs = make_bs();
    leaf_node_t *node = malloc_leaf_node(bs);

    leaf_node_handler::init(bs, node, repli_timestamp::invalid);
    verify(bs, node);

    btree_key *k = malloc_key(key);
    btree_value *v = malloc_value(value);

    ASSERT_TRUE(leaf_node_handler::insert(bs, node, k, v, repli_timestamp::invalid));
    verify(bs, node);
    ASSERT_EQ(1, node->npairs);

    union {
        byte readval_padding[MAX_TOTAL_NODE_CONTENTS_SIZE + sizeof(btree_value)];
        btree_value readval;
    };

    (void)readval_padding;

    ASSERT_TRUE(leaf_node_handler::lookup(node, k, &readval));

    ASSERT_EQ(0, memcmp(v, &readval, v->full_size()));

    leaf_node_handler::remove(bs, node, k);
    verify(bs, node);
    ASSERT_EQ(0, node->npairs);

    free(k);
    free(v);
    free(node);
}

TEST(LeafNodeTest, ElementaryInsertLookupRemove) {
    InsertLookupRemoveHelper("the_key", "the_value");
}
TEST(LeafNodeTest, EmptyValueInsertLookupRemove) {
    InsertLookupRemoveHelper("the_key", "");
}
TEST(LeafNodeTest, EmptyKeyInsertLookupRemove) {
    InsertLookupRemoveHelper("", "the_value");
}
TEST(LeafNodeTest, EmptyKeyValueInsertLookupRemove) {
    InsertLookupRemoveHelper("", "");
}


}  // namespace unittest

