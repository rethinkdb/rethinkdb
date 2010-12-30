#include "unittest/gtest.hpp"

#include "containers/bitset.hpp"

namespace unittest {

class BitsetTest : public ::testing::Test {
    public:
        BitsetTest() {
            bitset = new bitset_t(100);
        }
        bitset_t *bitset;
};

TEST_F(BitsetTest, ClearedAtStart) {
    ASSERT_TRUE(100 == bitset->size());
    ASSERT_TRUE(0 == bitset->count());
    for (int i = 0; i < 100; i++)
    {
        ASSERT_FALSE(bitset->test(i));
    }
}

TEST_F(BitsetTest, SetsGets) {
    bitset->set(20, true);
    bitset->set(0, true);
    bitset->set(99, true);

    ASSERT_TRUE(bitset->test(20));
    ASSERT_TRUE(bitset->test(0));
    ASSERT_TRUE(bitset->test(99));
    ASSERT_TRUE((*bitset)[99]);
    ASSERT_FALSE(bitset->test(50));

    ASSERT_TRUE(3 == bitset->count());

    bitset->set();

    ASSERT_TRUE(100 == bitset->count());
    ASSERT_TRUE((*bitset)[80]);
}

TEST_F(BitsetTest, BoundsChecking) {
    ASSERT_DEATH(bitset->test(400), "");
    ASSERT_DEATH(bitset->set(-5), "");
}

}
