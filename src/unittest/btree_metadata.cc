// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "arch/io/disk.hpp"
#include "clustering/administration/persist/file.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

TPTEST(BtreeMetadata, MetadataTest) {
    temp_directory_t temp_dir;
    io_backender_t io_backender(file_direct_io_mode_t::buffered_desired);
    cond_t non_interruptor;

    std::string big_string(10000, 'Q');
    metadata_file_t::key_t<std::string> big_string_key("big_string");
    metadata_file_t::key_t<int> int_prefix("int/");

    {
        metadata_file_t file(
            &io_backender,
            temp_dir.path(),
            &get_global_perfmon_collection(),
            [&](metadata_file_t::write_txn_t *txn, signal_t *interruptor) {
                txn->write(big_string_key, std::string("small string"), interruptor);
            },
            &non_interruptor);
        metadata_file_t::write_txn_t txn(&file, &non_interruptor);
        txn.write(big_string_key, big_string, &non_interruptor);
        txn.write(int_prefix.suffix("foo"), 101, &non_interruptor);
        txn.write(int_prefix.suffix("bar"), 102, &non_interruptor);
    }

    {
        metadata_file_t file(
            &io_backender,
            temp_dir.path(),
            &get_global_perfmon_collection(),
            &non_interruptor);
        metadata_file_t::read_txn_t txn(&file, &non_interruptor);
        std::string big_string_2 = txn.read(big_string_key, &non_interruptor);
        EXPECT_EQ(big_string, big_string_2);
        bool found_foo = false, found_bar = false;
        txn.read_many<int>(
            int_prefix,
            [&](const std::string &suffix, int value) {
                if (suffix == "foo") {
                    EXPECT_EQ(101, value);
                    EXPECT_FALSE(found_foo);
                    found_foo = true;
                } else if (suffix == "bar") {
                    EXPECT_EQ(102, value);
                    EXPECT_FALSE(found_bar);
                    found_bar = true;
                } else {
                    ADD_FAILURE() << "unexpected suffix: " << suffix;
                }
            },
            &non_interruptor);
        EXPECT_TRUE(found_foo);
        EXPECT_TRUE(found_bar);
    }
}

TPTEST(BtreeMetadata, ManyKeysBigValues) {
    temp_directory_t temp_dir;
    io_backender_t io_backender(file_direct_io_mode_t::buffered_desired);
    cond_t non_interruptor;

    std::map<std::string, std::string> data;
    for (size_t i = 0; i < 1000; ++i) {
        data.insert(std::make_pair(
            strprintf("%zu-", i) + rand_string(30),
            rand_string(i * 10)));
    }

    metadata_file_t::key_t<std::string> prefix("s");
    {
        metadata_file_t file(
            &io_backender,
            temp_dir.path(),
            &get_global_perfmon_collection(),
            [&](metadata_file_t::write_txn_t *, signal_t *) { },
            &non_interruptor);
        metadata_file_t::write_txn_t txn(&file, &non_interruptor);
        for (const auto &pair : data) {
            txn.write(prefix.suffix(pair.first), pair.second, &non_interruptor);
        }
    }

    {
        metadata_file_t file(
            &io_backender,
            temp_dir.path(),
            &get_global_perfmon_collection(),
            &non_interruptor);
        metadata_file_t::read_txn_t txn(&file, &non_interruptor);
        for (const auto &pair : data) {
            std::string value = txn.read(prefix.suffix(pair.first), &non_interruptor);
            ASSERT_EQ(pair.second, value);
        }
    }
}

} // namespace unittest

