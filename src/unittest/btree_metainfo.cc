// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "arch/io/disk.hpp"
#include "btree/operations.hpp"
#include "btree/reql_specific.hpp"
#include "buffer_cache/cache_balancer.hpp"
#include "containers/binary_blob.hpp"
#include "unittest/unittest_utils.hpp"
#include "serializer/config.hpp"

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
    standard_serializer_t::create(
        &file_opener,
        standard_serializer_t::static_config_t());

    standard_serializer_t serializer(
        standard_serializer_t::dynamic_config_t(),
        &file_opener,
        &get_global_perfmon_collection());

    dummy_cache_balancer_t balancer(GIGABYTE);
    cache_t cache(&serializer, &balancer, &get_global_perfmon_collection());
    cache_conn_t cache_conn(&cache);

    {
        txn_t txn(&cache_conn, write_durability_t::HARD,
                  repli_timestamp_t::distant_past, 1);
        buf_lock_t superblock(&txn, SUPERBLOCK_ID, alt_create_t::create);
        buf_write_t sb_write(&superblock);
        btree_slice_t::init_superblock(&superblock,
                                       std::vector<char>(), binary_blob_t());
    }

    std::map<std::string, std::string> mirror;
    mirror[""] = "";

    order_source_t order_source;


    for (int i = 0; i < 1000; i++) {
        scoped_ptr_t<txn_t> txn;
        scoped_ptr_t<real_superblock_t> superblock;
        get_btree_superblock_and_txn(&cache_conn, write_access_t::write, 1,
                                     repli_timestamp_t::distant_past,
                                     write_durability_t::SOFT,
                                     &superblock, &txn);
        buf_lock_t *sb_buf = superblock->get();

        int op = random() % 100;
        if (op == 0) {
            clear_superblock_metainfo(sb_buf);
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
            bool found = get_superblock_metainfo(sb_buf, string_to_vector(key),
                                                 &value_out);
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
            const bool found = get_superblock_metainfo(sb_buf, string_to_vector(key),
                                                       &value_out);
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
            set_superblock_metainfo(sb_buf, string_to_vector(key),
                                    string_to_blob(value));
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
            set_superblock_metainfo(sb_buf, string_to_vector(key),
                                    string_to_blob(value));
            mirror[key] = value;
            if (print_log_messages) {
                printf("insert '%s' = '%s'\n", key.c_str(), value.c_str());
            }
        } else if (op <= 90) {
            if (mirror.empty()) {
                continue;
            }
            std::string key = random_existing_key(mirror);
            delete_superblock_metainfo(sb_buf, string_to_vector(key));
            mirror.erase(key);
            if (print_log_messages) {
                printf("delete '%s'\n", key.c_str());
            }
        } else {
            std::vector<std::pair<std::vector<char>, std::vector<char> > > pairs;
            get_superblock_metainfo(sb_buf, &pairs);
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

}   /* namespace unittest */
