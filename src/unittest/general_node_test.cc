#include "unittest/gtest.hpp"

#include "btree/general_node.hpp"

namespace unittest {

class small_value_sizer_t : public btree::value_sizer_t {
public:
    int size(const char *value) {
        return *reinterpret_cast<const uint8_t *>(value);
    }

    bool fits(const char *value, int length_available) {
        return length_available > 0 && length_available >= 1 + size(value);
    }

    int max_possible_size() const {
        return 251;
    }

    block_magic_t btree_leaf_magic() const {
        block_magic_t ret = { { 'u', 'n', 'i', 't' } };
        return ret;
    }
};

TEST(GeneralNodeTest, Offsets) {
    EXPECT_EQ(0, offsetof(btree::general_node_t, magic));
    EXPECT_EQ(4, offsetof(btree::general_node_t, times));
    EXPECT_EQ(12, offsetof(btree::general_node_t, npairs));
    EXPECT_EQ(14, offsetof(btree::general_node_t, frontmost_offset));
    EXPECT_EQ(16, offsetof(btree::general_node_t, pair_offsets));
    EXPECT_EQ(16, sizeof(btree::general_node_t));

    EXPECT_EQ(0, offsetof(btree::general_timestamps_t, last_modified));
    EXPECT_EQ(4, offsetof(btree::general_timestamps_t, earlier));
    EXPECT_EQ(8, sizeof(btree::general_timestamps_t));

}


}
