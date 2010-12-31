#include "unittest/gtest.hpp"

#include "containers/bitset.hpp"

namespace unittest {

TEST(BitsetTest, ClearedAtStart) {
    bitset_t b(100);
    ASSERT_TRUE(100 == b.size());
    ASSERT_TRUE(0 == b.count());
    for (int i = 0; i < 100; i++) {
        ASSERT_FALSE(b.test(i));
    }
}

TEST(BitsetTest, SetsGets) {
    bitset_t b(100);

    b.set(20, true);
    b.set(0, true);
    b.set(99, true);

    ASSERT_TRUE(b.test(20));
    ASSERT_TRUE(b.test(0));
    ASSERT_TRUE(b.test(99));
    ASSERT_TRUE(b[99]);
    ASSERT_FALSE(b.test(50));

    ASSERT_TRUE(3 == b.count());

    b.set();

    ASSERT_TRUE(100 == b.count());
    ASSERT_TRUE(b[80]);
}

TEST(BitsetTest, BoundsChecking) {
    bitset_t b(100);
    ASSERT_DEATH(b.test(400), "");
    ASSERT_DEATH(b.set(-5), "");
    ASSERT_DEATH(b.test(100), "");
    ASSERT_DEATH(b.test(-1), "");
    b.set(99, true);
    ASSERT_TRUE(b.test(99));
}

}
