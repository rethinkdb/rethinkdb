// Copyright 2010-2015 RethinkDB, all rights reserved.
#include <string>

#include "unittest/gtest.hpp"

#include "clustering/administration/tables/split_points.hpp"
#include "clustering/administration/tables/table_metadata.hpp"
#include "btree/keys.hpp"

namespace unittest {

table_shard_scheme_t do_rebalance(const std::map<store_key_t, int64_t> &distribution,
                                  size_t num_shards) {
    table_shard_scheme_t split_points_out;
    std::string error_out;
    bool res = calculate_split_points_with_distribution(distribution, num_shards,
                                                        &split_points_out, &error_out);

    EXPECT_TRUE(res) << ("rebalance failed: " + error_out);
    return split_points_out;
}

TEST(Rebalance, Regression3020) {
    static const uint8_t key0[0] = { };
    static const uint8_t key1[1] = { 'N' };
    static const uint8_t key2[5] = { 'N', 'Z', 'a', 'b', 'c' };
    static const uint8_t key3[6] = { 'N', 'Z', 'a', 'b', 'c', '\x00' };
    static const uint8_t key4[2] = { 'N', 'c' };
    static const uint8_t key5[2] = { 'S', '1' };
    static const uint8_t key6[2] = { 'S', '2' };
    static const uint8_t key7[2] = { 'S', '3' };
    static const uint8_t key8[2] = { 'S', '4' };

    std::map<store_key_t, int64_t> distribution;
    distribution[store_key_t(sizeof(key0), key0)] = 1;
    distribution[store_key_t(sizeof(key1), key1)] = 1;
    distribution[store_key_t(sizeof(key2), key2)] = 6;
    distribution[store_key_t(sizeof(key3), key3)] = 6;
    distribution[store_key_t(sizeof(key4), key4)] = 6;
    distribution[store_key_t(sizeof(key5), key5)] = 6;
    distribution[store_key_t(sizeof(key6), key6)] = 6;
    distribution[store_key_t(sizeof(key7), key7)] = 6;
    distribution[store_key_t(sizeof(key8), key8)] = 1;

    do_rebalance(distribution, 9);
}

TEST(Rebalance, NoBalance) {
    static const uint8_t key0[0] = { };
    static const uint8_t key1[1] = { 'D' };
    static const uint8_t key2[1] = { 'F' };
    static const uint8_t key3[1] = { 'O' };
    static const uint8_t key4[1] = { 'U' };

    std::map<store_key_t, int64_t> distribution;
    distribution[store_key_t(sizeof(key0), key0)] = 5;
    distribution[store_key_t(sizeof(key1), key1)] = 5;
    distribution[store_key_t(sizeof(key2), key2)] = 5;
    distribution[store_key_t(sizeof(key3), key3)] = 5;
    distribution[store_key_t(sizeof(key4), key4)] = 5;

    do_rebalance(distribution, 5);
}

TEST(Rebalance, MaxKeyLength) {
    static const std::string key0("");
    static const std::string key1(MAX_KEY_SIZE - 1, 'A');
    static const std::string key2(MAX_KEY_SIZE, 'A');
    static const std::string key3(std::string(MAX_KEY_SIZE - 1, 'A') + 'B');

    std::map<store_key_t, int64_t> distribution;
    distribution[store_key_t(key0)] = 8;
    distribution[store_key_t(key1)] = 8;
    distribution[store_key_t(key2)] = 8;
    distribution[store_key_t(key3)] = 8;

    do_rebalance(distribution, 3);
}

}  // namespace unittest
