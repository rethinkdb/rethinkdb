#include "unittest/gtest.hpp"

#include "rdb_protocol/geo/s2/util/math/exactfloat/exactfloat.h"

namespace unittest {

TEST(BignumTest, TestGetUint64) {
    std::unique_ptr<BIGNUM, geo::BIGNUM_deleter> bn = geo::make_BN_new();

    uint64_t x = geo::BN_ext_get_uint64(bn.get());
    ASSERT_EQ(0, x);

    uint64_t values[4] = { 1, (1ull << 32) - 1, (1ull << 32), uint64_t(-1) };
    for (int i = 0; i < 4; ++i) {
        geo::BN_ext_set_uint64(bn.get(), values[i]);
        ASSERT_EQ(values[i], geo::BN_ext_get_uint64(bn.get()));
    }
}

TEST(BignumTest, TestCountLowZeroBits) {
    std::unique_ptr<BIGNUM, geo::BIGNUM_deleter> bn = geo::make_BN_new();
    int n = geo::BN_ext_count_low_zero_bits(bn.get());
    ASSERT_EQ(0, n);

    geo::BN_ext_set_uint64(bn.get(), 1);
    n = geo::BN_ext_count_low_zero_bits(bn.get());
    ASSERT_EQ(0, n);

    geo::BN_ext_set_uint64(bn.get(), 2);
    n = geo::BN_ext_count_low_zero_bits(bn.get());
    ASSERT_EQ(1, n);

    geo::BN_ext_set_uint64(bn.get(), 128);
    n = geo::BN_ext_count_low_zero_bits(bn.get());
    ASSERT_EQ(7, n);

    geo::BN_ext_set_uint64(bn.get(), 256);
    n = geo::BN_ext_count_low_zero_bits(bn.get());
    ASSERT_EQ(8, n);

    geo::BN_ext_set_uint64(bn.get(), 257);
    n = geo::BN_ext_count_low_zero_bits(bn.get());
    ASSERT_EQ(0, n);

    for (int i = 0; i < 64; i++) {
        geo::BN_ext_set_uint64(bn.get(), (1ull << i));
        n = geo::BN_ext_count_low_zero_bits(bn.get());
        ASSERT_EQ(i, n);
    }

    geo::BN_ext_set_uint64(bn.get(), (1ull << 63));
    BN_CTX *ctx = BN_CTX_new();
    BN_mul(bn.get(), bn.get(), bn.get(), ctx);
    BN_CTX_free(ctx);
    n = geo::BN_ext_count_low_zero_bits(bn.get());
    ASSERT_EQ(126, n);
}

}  // namespace unittest
