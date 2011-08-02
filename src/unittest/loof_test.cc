#include "unittest/gtest.hpp"

#include "btree/loof_node.hpp"

struct short_value_t;

template <>
class value_sizer_t<short_value_t> {
    value_sizer_t<short_value_t>(block_size_t bs) : block_size_(bs) { }

    int size(const short_value_t *value) const {
        int x = *reinterpret_cast<const uint8_t *>(value);
        return 1 + x;
    }

    bool fits(const short_value_t *value, int length_available) const {
        return length_available > 0 && size(value) <= length_available;
    }

    int max_possible_size() const {
        return 256;
    }

    static block_magic_t btree_leaf_magic() {
        block_magic_t magic = { { 's', 'h', 'L', 'F' } };
        return magic;
    }

    block_size_t block_size() const { return block_size_; }

private:
    block_size_t block_size_;

    DISABLE_COPYING(value_sizer_t<short_value_t>);
};

using loof::loof_t;

namespace unittest {


TEST(LoofTest, Offsets) {
    ASSERT_EQ(0, offsetof(loof_t, magic));
    ASSERT_EQ(4, offsetof(loof_t, num_pairs));
    ASSERT_EQ(6, offsetof(loof_t, live_size));
    ASSERT_EQ(8, offsetof(loof_t, frontmost));
    ASSERT_EQ(10, offsetof(loof_t, tstamp_cutpoint));
    ASSERT_EQ(12, offsetof(loof_t, pair_offsets));
    ASSERT_EQ(12, sizeof(loof_t));
}


}  // namespace unittest
