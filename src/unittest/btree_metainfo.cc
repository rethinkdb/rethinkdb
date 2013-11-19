// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "arch/io/disk.hpp"
#include "btree/operations.hpp"
#include "unittest/unittest_utils.hpp"
#include "serializer/config.hpp"

using namespace alt;  // RSI

namespace unittest {

static const bool print_log_messages = false;
#ifdef __clang__
#pragma clang diagnostic ignored "-Wunreachable-code"
#endif

std::string random_existing_key(const std::map<std::string, std::string> &map) {
    int i = random() % map.size();
    std::map<std::string, std::string>::const_iterator it = map.begin();
    while (i-- > 0) it++;
    return it->first;
}

std::string random_string() {
    std::string s;
    int length = random() % 10;
    if (random() % 100 == 0) {
        length *= 5000;
    }
    char c = 'a' + random() % ('z' - 'a' + 1);
    return std::string(length, c);
}

std::vector<char> string_to_vector(const std::string &s) {
    return std::vector<char>(s.data(), s.data() + s.size());
}

std::string vector_to_string(const std::vector<char> &v) {
    return std::string(v.data(), v.size());
}

void run_metainfo_test() {
    temp_file_t temp_file;

    io_backender_t io_backender(file_direct_io_mode_t::buffered_desired);

    filepath_file_opener_t file_opener(temp_file.name(), &io_backender);
    standard_serializer_t::create(
        &file_opener,
        standard_serializer_t::static_config_t());

    standard_serializer_t serializer(
        standard_serializer_t::dynamic_config_t(),
        &file_opener,
        &get_global_perfmon_collection());

    cache_t::create(&serializer);

#if SLICE_ALT
    alt_cache_t cache(&serializer);
#else
    const mirrored_cache_config_t cache_dynamic_config;
    cache_t cache(&serializer, cache_dynamic_config, &get_global_perfmon_collection());
#endif

    btree_slice_t::create(&cache, std::vector<char>(), std::vector<char>());
    std::map<std::string, std::string> mirror;
    mirror[""] = "";

    btree_slice_t btree(&cache, &get_global_perfmon_collection(), "unittest");

    order_source_t order_source;


    for (int i = 0; i < 1000; i++) {

        order_token_t otok = order_source.check_in("metainfo unittest");

#if SLICE_ALT
        scoped_ptr_t<alt_txn_t> txn;
#else
        scoped_ptr_t<transaction_t> txn;
#endif
        scoped_ptr_t<real_superblock_t> superblock;
#if SLICE_ALT
        get_btree_superblock_and_txn(&btree, alt_access_t::write, 1,
                                     repli_timestamp_t::invalid, otok,
                                     WRITE_DURABILITY_SOFT,
                                     &superblock, &txn);
#else
        get_btree_superblock_and_txn(&btree, rwi_write, rwi_write, 1, repli_timestamp_t::invalid, otok, WRITE_DURABILITY_SOFT, &superblock, &txn);
#endif
#if SLICE_ALT
        alt_buf_lock_t *sb_buf = superblock->get();
#else
        buf_lock_t *sb_buf = superblock->get();
#endif

        int op = random() % 100;
        if (op == 0) {
#if SLICE_ALT
            clear_superblock_metainfo(sb_buf);
#else
            clear_superblock_metainfo(txn.get(), sb_buf);
#endif
            mirror.clear();
            if (print_log_messages) {
                puts("clear");
            }
        } else if (op <= 30) {
            if (mirror.empty()) {
                continue;
            }
            std::string key = random_existing_key(mirror);
            std::vector<char> value_out;
#if SLICE_ALT
            bool found = get_superblock_metainfo(sb_buf, string_to_vector(key),
                                                 &value_out);
#else
            bool found = get_superblock_metainfo(txn.get(), sb_buf, string_to_vector(key), &value_out);
#endif
            EXPECT_TRUE(found);
            if (found) {
                EXPECT_EQ(mirror[key], vector_to_string(value_out));
            }
            if (print_log_messages) {
                std::string repr = found ? "'" + vector_to_string(value_out) + "'" : "<not found>";
                printf("get '%s' -> %s (expected '%s')\n", key.c_str(), repr.c_str(), mirror[key].c_str());
            }
        } else if (op <= 40) {
            std::string key = random_string();
            if (mirror.count(key) > 0) {
                continue;
            }
            std::vector<char> value_out;
#if SLICE_ALT
            const bool found = get_superblock_metainfo(sb_buf, string_to_vector(key),
                                                       &value_out);
#else
            const bool found = get_superblock_metainfo(txn.get(), sb_buf, string_to_vector(key), &value_out);
#endif
            EXPECT_FALSE(found);
            if (found) {
                EXPECT_EQ(mirror[key], vector_to_string(value_out));
            }
            if (print_log_messages) {
                std::string repr = found ? "'" + vector_to_string(value_out) + "'" : "<not found>";
                printf("get '%s' -> %s (Expected <not found>)\n", key.c_str(), repr.c_str());
            }
        } else if (op <= 60) {
            if (mirror.empty()) {
                continue;
            }
            std::string key = random_existing_key(mirror);
            std::string value = random_string();
#if SLICE_ALT
            set_superblock_metainfo(sb_buf, string_to_vector(key),
                                    string_to_vector(value));
#else
            set_superblock_metainfo(txn.get(), sb_buf, string_to_vector(key), string_to_vector(value));
#endif
            mirror[key] = value;
            if (print_log_messages) {
                printf("update '%s' = '%s'\n", key.c_str(), value.c_str());
            }
        } else if (op <= 80) {
            std::string key = random_string();
            if (mirror.count(key) > 0) {
                continue;
            }
            std::string value = random_string();
#if SLICE_ALT
            set_superblock_metainfo(sb_buf, string_to_vector(key),
                                    string_to_vector(value));
#else
            set_superblock_metainfo(txn.get(), sb_buf, string_to_vector(key), string_to_vector(value));
#endif
            mirror[key] = value;
            if (print_log_messages) {
                printf("insert '%s' = '%s'\n", key.c_str(), value.c_str());
            }
        } else if (op <= 90) {
            if (mirror.empty()) {
                continue;
            }
            std::string key = random_existing_key(mirror);
#if SLICE_ALT
            delete_superblock_metainfo(sb_buf, string_to_vector(key));
#else
            delete_superblock_metainfo(txn.get(), sb_buf, string_to_vector(key));
#endif
            mirror.erase(key);
            if (print_log_messages) {
                printf("delete '%s'\n", key.c_str());
            }
        } else {
            std::vector<std::pair<std::vector<char>, std::vector<char> > > pairs;
#if SLICE_ALT
            get_superblock_metainfo(sb_buf, &pairs);
#else
            get_superblock_metainfo(txn.get(), sb_buf, &pairs);
#endif
            std::map<std::string, std::string> mirror_copy = mirror;
            if (print_log_messages) {
                puts("scan...");
            }
            for (size_t j = 0; j < pairs.size(); ++j) {
                std::map<std::string, std::string>::iterator it = mirror_copy.find(vector_to_string(pairs[j].first));
                if (it == mirror_copy.end()) {
                    if (print_log_messages) {
                        printf("    '%s' = '%s' (expected <not found>)\n", vector_to_string(pairs[j].first).c_str(), vector_to_string(pairs[j].second).c_str());
                    }
                    ADD_FAILURE() << "extra key";
                } else {
                    if (print_log_messages) {
                        printf("    '%s' = '%s' (expected '%s')\n", vector_to_string(pairs[j].first).c_str(), vector_to_string(pairs[j].second).c_str(), it->second.c_str());
                    }
                    EXPECT_EQ(it->second, vector_to_string(pairs[j].second));
                    mirror_copy.erase(it);
                }
            }
            if (print_log_messages) {
                for (std::map<std::string, std::string>::iterator it = mirror_copy.begin(); it != mirror_copy.end(); it++) {
                    printf("    '%s' = <not found> (expected '%s')\n", it->first.c_str(), it->second.c_str());
                }
            }
            EXPECT_EQ(0u, mirror_copy.size()) << "missing key(s)";
        }
    }
}

TEST(BtreeMetainfo, MetainfoTest) {
    unittest::run_in_thread_pool(&run_metainfo_test);
}

}   /* namespace unittest */
