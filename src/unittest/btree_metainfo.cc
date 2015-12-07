// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "arch/io/disk.hpp"
#include "btree/operations.hpp"
#include "btree/reql_specific.hpp"
#include "buffer_cache/cache_balancer.hpp"
#include "containers/binary_blob.hpp"
#include "unittest/unittest_utils.hpp"
#include "serializer/log/log_serializer.hpp"

namespace unittest {

#ifdef _MSC_VER
int (&random)() = rand;
#endif

#ifdef __clang__
#pragma clang diagnostic ignored "-Wunreachable-code"
#endif

int (&random)() = rand; // ATN TODO

std::string random_string() {
    std::string s;
    int length = random() % 10;
    if (random() % 100 == 0) {
        length *= 5000;
    }
    char c = 'a' + random() % ('z' - 'a' + 1);
    return std::string(length, c);
}

binary_blob_t string_to_blob(const std::string &s) {
    return binary_blob_t(s.data(), s.data() + s.size());
}

std::vector<char> string_to_vector(const std::string &s) {
    return std::vector<char>(s.data(), s.data() + s.size());
}

std::string vector_to_string(const std::vector<char> &v) {
    return std::string(v.data(), v.size());
}

TPTEST(BtreeMetainfo, MetainfoTest) {
    temp_file_t temp_file;

    io_backender_t io_backender(file_direct_io_mode_t::buffered_desired);

    filepath_file_opener_t file_opener(temp_file.name(), &io_backender);
    log_serializer_t::create(
        &file_opener,
        log_serializer_t::static_config_t());

    log_serializer_t serializer(
        log_serializer_t::dynamic_config_t(),
        &file_opener,
        &get_global_perfmon_collection());

    dummy_cache_balancer_t balancer(GIGABYTE);
    cache_t cache(&serializer, &balancer, &get_global_perfmon_collection());
    cache_conn_t cache_conn(&cache);

    {
        txn_t txn(&cache_conn, write_durability_t::HARD, 1);
        buf_lock_t sb_lock(&txn, SUPERBLOCK_ID, alt_create_t::create);
        real_superblock_t superblock(std::move(sb_lock));
        btree_slice_t::init_real_superblock(&superblock,
                                            std::vector<char>(), binary_blob_t());
    }

    for (int i = 0; i < 3; ++i) {
        std::map<std::string, std::string> metainfo;
        for (int j = 0; j < 100; ++j) {
            metainfo[random_string()] = random_string();
        }
        {
            scoped_ptr_t<txn_t> txn;
            scoped_ptr_t<real_superblock_t> superblock;
            get_btree_superblock_and_txn_for_writing(
                &cache_conn, nullptr, write_access_t::write, 1,
                write_durability_t::SOFT, &superblock, &txn);
            std::vector<std::vector<char> > keys;
            std::vector<binary_blob_t> values;
            for (const auto &pair : metainfo) {
                keys.push_back(string_to_vector(pair.first));
                values.push_back(string_to_blob(pair.second));
            }
            set_superblock_metainfo(superblock.get(), keys, values,
                                    cluster_version_t::v2_1);
        }
        {
            scoped_ptr_t<txn_t> txn;
            scoped_ptr_t<real_superblock_t> superblock;
            get_btree_superblock_and_txn_for_reading(
                &cache_conn, CACHE_SNAPSHOTTED_NO, &superblock, &txn);
            cluster_version_t version;
            std::vector<std::pair<std::vector<char>, std::vector<char> > > read_back;
            get_superblock_metainfo(superblock.get(), &read_back, &version);
            std::set<std::string> seen;
            for (const auto &pair : read_back) {
                std::string key = vector_to_string(pair.first);
                std::string value = vector_to_string(pair.second);
                ASSERT_EQ(0, seen.count(key));
                seen.insert(key);
                ASSERT_EQ(metainfo[key], value);
            }
            ASSERT_EQ(version, cluster_version_t::v2_1);
            ASSERT_EQ(metainfo.size(), seen.size());
        }
    }
}

}   /* namespace unittest */
