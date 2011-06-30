#include "unittest/gtest.hpp"

#include "btree/general_node.hpp"
#include "buffer_cache/types.hpp"

namespace unittest {

class small_value_sizer_t : public btree::value_sizer_t {
public:
    int size(const char *value) {
        return *reinterpret_cast<const uint8_t *>(value);
    }

    bool fits(const char *value, int length_available) {
        return length_available > 0 && length_available >= 1 + size(value);
    }

    virtual block_magic_t btree_leaf_magic() const {
        block_magic_t ret = { { 'u', 'n', 'i', 't' } };
        return ret;
    }
};



TEST(GeneralNodeTest, HelloWorld) {
    EXPECT_EQ(0, 0);
}


}
