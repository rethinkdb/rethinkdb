// Copyright 2010-2015 RethinkDB, all rights reserved.

#include "arch/io/disk.hpp"
#include "arch/types.hpp"
#include "btree/reql_specific.hpp"
#include "buffer_cache/cache_balancer.hpp"
#include "rdb_protocol/btree.hpp"
#include "repli_timestamp.hpp"
#include "serializer/log/log_serializer.hpp"
#include "serializer/merger.hpp"
#include "unittest/btree_utils.hpp"
#include "unittest/gtest.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

class map_filler_callback_t : public depth_first_traversal_callback_t {
public:
    map_filler_callback_t(std::map<store_key_t, std::string> *m_out) : m_out_(m_out) { }

    continue_bool_t handle_pair(scoped_key_value_t &&keyvalue, UNUSED signal_t *interruptor) {
        store_key_t store_key(keyvalue.key());

        if (last_key.has()) {
            EXPECT_TRUE(store_key > *last_key);
        }
        last_key = make_scoped<store_key_t>(store_key);

        const short_value_buffer_t *value_buf = static_cast<const short_value_buffer_t *>(keyvalue.value());
        (*m_out_)[store_key] = value_buf->as_str();
        return continue_bool_t::CONTINUE;
    }

private:
    std::map<store_key_t, std::string> *m_out_;
    scoped_ptr_t<store_key_t> last_key;
};

class BTreeTestContext {
public:
    BTreeTestContext()
        : io_backender(file_direct_io_mode_t::buffered_desired),
          file_opener(temp_file.name(), &io_backender),
          balancer(GIGABYTE) {

        log_serializer_t::create(&file_opener, log_serializer_t::static_config_t());

        auto inner_serializer = make_scoped<log_serializer_t>(
            log_serializer_t::dynamic_config_t(),
            &file_opener,
            &get_global_perfmon_collection());

        serializer = make_scoped<merger_serializer_t>(
                std::move(inner_serializer),
                MERGER_SERIALIZER_MAX_ACTIVE_WRITES);

        cache = make_scoped<cache_t>(serializer.get(), &balancer, &get_global_perfmon_collection());
        cache_conn = make_scoped<cache_conn_t>(cache.get());
        sizer = make_scoped<short_value_sizer_t>(cache.get()->max_block_size());

        {
            txn_t txn(cache_conn.get(), write_durability_t::SOFT, 1);
            buf_lock_t sb_lock(&txn, SUPERBLOCK_ID, alt_create_t::create);
            real_superblock_t superblock(std::move(sb_lock));
            btree_slice_t::init_real_superblock(&superblock, std::vector<char>(), binary_blob_t());
        }
    }

    void run_txn_fn(bool readwrite,
            const std::function<void(scoped_ptr_t<real_superblock_t> &&)> &fn) {

        scoped_ptr_t<txn_t> txn;
        scoped_ptr_t<real_superblock_t> superblock;

        if (readwrite) {
            get_btree_superblock_and_txn_for_writing(
                    cache_conn.get(),
                    nullptr,
                    write_access_t::write,
                    1,
                    write_durability_t::SOFT,
                    &superblock,
                    &txn);
        } else {
            get_btree_superblock_and_txn_for_reading(
                    cache_conn.get(),
                    CACHE_SNAPSHOTTED_NO,
                    &superblock,
                    &txn);
        }

        fn(std::move(superblock));
    }

    std::string get(const store_key_t &key) {
        std::string bt_result;

        run_txn_fn(false, [&](scoped_ptr_t<real_superblock_t> &&superblock){
            profile::trace_t trace;
            btree_stats_t stats(&get_global_perfmon_collection(), "test-get");

            keyvalue_location_t kv_location;
            find_keyvalue_location_for_read(
                sizer.get(),
                superblock.get(),
                key.btree_key(),
                &kv_location,
                &stats,
                &trace);

            if (kv_location.value.has()) {
                short_value_buffer_t *v = kv_location.value_as<short_value_buffer_t>();
                bt_result = v->as_str();
            }
        });

        std::string kv_result;
        auto kv_pair = kv.find(key);
        if (kv_pair != kv.end()) {
            kv_result = kv_pair->second;
        }

        EXPECT_EQ(kv_result, bt_result);

        return bt_result;
    }

    void set(const store_key_t &key, const std::string &value, repli_timestamp_t timestamp) {
        run_txn_fn(true, [&](scoped_ptr_t<real_superblock_t> &&superblock){
            profile::trace_t trace;
            noop_value_deleter_t deleter;
            rdb_live_deletion_context_t deletion_context;
            null_key_modification_callback_t null_cb;

            keyvalue_location_t kv_location;
            find_keyvalue_location_for_write(
                sizer.get(),
                superblock.get(),
                key.btree_key(),
                timestamp,
                &deleter,
                &kv_location,
                &trace);

            short_value_buffer_t buf(value);
            char *data = reinterpret_cast<char *>(buf.data());
            scoped_malloc_t<void> value_data(data, data+buf.size());

            kv_location.value = std::move(value_data);

            apply_keyvalue_change(
                sizer.get(),
                &kv_location,
                key.btree_key(),
                timestamp,
                &deleter,
                &null_cb,
                delete_mode_t::REGULAR_QUERY);
        });

        kv[key] = value;
    }

    void set(const store_key_t &key, const std::string &value) {
        set(key, value, repli_timestamp_t::distant_past);
    }

    void remove(const store_key_t &key, repli_timestamp_t timestamp) {
        EXPECT_TRUE(should_have(key));

        run_txn_fn(true, [&](scoped_ptr_t<real_superblock_t> &&superblock){
            profile::trace_t trace;
            noop_value_deleter_t deleter;
            rdb_live_deletion_context_t deletion_context;
            null_key_modification_callback_t null_cb;

            keyvalue_location_t kv_location;
            find_keyvalue_location_for_write(
                sizer.get(),
                superblock.get(),
                key.btree_key(),
                timestamp,
                &deleter,
                &kv_location,
                &trace);

            EXPECT_TRUE(kv_location.value.has());

            kv_location.value.reset();

            apply_keyvalue_change(
                sizer.get(),
                &kv_location,
                key.btree_key(),
                timestamp,
                &deleter,
                &null_cb,
                delete_mode_t::REGULAR_QUERY);
        });

        kv.erase(key);
    }

    void remove(const store_key_t &key) {
        remove(key, repli_timestamp_t::distant_past);
    }

    void range(const key_range_t &range) {
        std::map<store_key_t, std::string> bt_map;

        run_txn_fn(false, [&](scoped_ptr_t<real_superblock_t> &&superblock){
            cond_t interruptor;

            map_filler_callback_t filler_cb(&bt_map);

            btree_depth_first_traversal(
                superblock.get(),
                range,
                &filler_cb,
                access_t::read,
                direction_t::FORWARD,
                release_superblock_t::RELEASE,
                &interruptor);
        });

        std::map<store_key_t, std::string> kv_map;

        for (auto it = kv.begin(), last = kv.end(); it != last; ++it) {
            if (range.contains_key(it->first)) {
                kv_map[it->first] = it->second;
            }
        }

        expect_maps_equal(bt_map, kv_map);
    }

    bool should_have(const store_key_t &key) {
        return kv.find(key) != kv.end();
    }

    void printmap(const std::map<store_key_t, std::string> &m) {
        for (auto it = m.begin(), last = m.end(); it != last; ++it) {
            printf("%s: %s, ", key_to_debug_str(it->first).c_str(), it->second.c_str());
        }
    }

    void expect_maps_equal(const std::map<store_key_t, std::string> &m1,
                           const std::map<store_key_t, std::string> &m2) {
        if (m1 != m2) {
            printf("m1: ");
            printmap(m1);
            printf("\nm2: ");
            printmap(m2);
            printf("\n");
        }
        EXPECT_TRUE(m1 == m2);
    }

    void verify() {
        std::map<store_key_t, std::string> bt_map;

        run_txn_fn(false, [&](scoped_ptr_t<real_superblock_t> &&superblock){
            cond_t interruptor;

            map_filler_callback_t filler_cb(&bt_map);

            btree_depth_first_traversal(
                superblock.get(),
                key_range_t::universe(),
                &filler_cb,
                access_t::read,
                direction_t::FORWARD,
                release_superblock_t::RELEASE,
                &interruptor);
        });

        expect_maps_equal(bt_map, kv);
    }

    store_key_t pick_random_key(rng_t *rng) {
        if (is_empty()) {
            return store_key_t();
        }

        int index = rng->randint(kv.size());
        auto it = kv.begin();
        while (index > 0) {
            index--;
            it++;
        }
        return it->first;
    }

    store_key_t lowest_key() {
        if (is_empty()) {
            return store_key_t();
        }
        return kv.begin()->first;
    }

    bool is_empty() {
        return kv.size() == 0;
    }

private:
    temp_file_t temp_file;
    io_backender_t io_backender;
    filepath_file_opener_t file_opener;
    dummy_cache_balancer_t balancer;

    scoped_ptr_t<merger_serializer_t> serializer;
    scoped_ptr_t<cache_t> cache;
    scoped_ptr_t<cache_conn_t> cache_conn;
    scoped_ptr_t<short_value_sizer_t> sizer;

    std::map<store_key_t, std::string> kv;
};

TPTEST(BTree, Basics) {
    BTreeTestContext ctx;

    ctx.set(store_key_t("a"), "b");
    ctx.get(store_key_t("a"));
    ctx.set(store_key_t("a"), "c");
    ctx.get(store_key_t("a"));
    ctx.set(store_key_t("b"), "d");
    ctx.get(store_key_t("a"));
    ctx.get(store_key_t("b"));
    ctx.range(key_range_t(key_range_t::bound_t::none,   store_key_t(),
                          key_range_t::bound_t::closed, store_key_t("a")));
    ctx.remove(store_key_t("a"));
    ctx.get(store_key_t("a"));
    ctx.get(store_key_t("b"));
    ctx.verify();
}

key_range_t random_key_range(rng_t *rng) {
    key_range_t::bound_t lm = key_range_t::bound_t::none;
    if (rng->randint(8) != 0) {
        lm = rng->randint(2) == 0 ? key_range_t::bound_t::open : key_range_t::bound_t::closed;
    }

    key_range_t::bound_t rm = key_range_t::bound_t::none;
    if (rng->randint(8) != 0) {
        rm = rng->randint(2) == 0 ? key_range_t::bound_t::open : key_range_t::bound_t::closed;
    }

    store_key_t lk, rk;
    do {
        lk = store_key_t(random_letter_string(rng, 1, 250));
        rk = store_key_t(random_letter_string(rng, 1, 250));
    } while (lk > rk);

    return key_range_t(lm, lk, rm, rk);
}

enum class btree_fuzz_op_t {
    get,
    set,
    remove,
    range,
    verify,
};

void btree_fuzz_test(bool stay_small, bool random_timestamps, int iterations) {
    BTreeTestContext ctx;
    rng_t rng;

    std::vector<btree_fuzz_op_t> op_table {
         btree_fuzz_op_t::set,
         btree_fuzz_op_t::set,
         btree_fuzz_op_t::set,

         // more removes than sets if stay_small is true,
         // more sets than removes if stay_small is false.
         stay_small ? btree_fuzz_op_t::remove : btree_fuzz_op_t::set,

         btree_fuzz_op_t::remove,
         btree_fuzz_op_t::remove,
         btree_fuzz_op_t::remove,

         btree_fuzz_op_t::get,
         btree_fuzz_op_t::range,
         btree_fuzz_op_t::verify,
    };

    for (int i = 0; i < iterations; i++) {
        switch (op_table[rng.randint(op_table.size())]) {
        case btree_fuzz_op_t::set: {
            repli_timestamp_t ts(repli_timestamp_t::distant_past);
            if (random_timestamps) {
                ts.longtime = rng.randuint64(1000);
            }

            store_key_t key(random_letter_string(&rng, 1, 250));
            if (rng.randint(10) == 0) {
                key = ctx.pick_random_key(&rng);
            }

            ctx.set(key, random_letter_string(&rng, 0, 250), ts);
            break;
        }

        case btree_fuzz_op_t::remove: {
            repli_timestamp_t ts(repli_timestamp_t::distant_past);
            if (random_timestamps) {
                ts.longtime = rng.randuint64(1000);
            }

            store_key_t key(ctx.pick_random_key(&rng));

            if (ctx.should_have(key)) {
                ctx.remove(key, ts);
            }
            break;
        }

        case btree_fuzz_op_t::get:
            if (rng.randint(10) == 0) {
                ctx.get(store_key_t(random_letter_string(&rng, 1, 250)));
            } else {
                ctx.get(ctx.pick_random_key(&rng));
            }
            break;

        case btree_fuzz_op_t::range: {
            ctx.range(random_key_range(&rng));
            break;
        }

        case btree_fuzz_op_t::verify:
            ctx.verify();
            break;

        default:
            unreachable();
        }
    }
}

TPTEST(BTree, WholeSmallFuzz) {
    btree_fuzz_test(true, false, 300);
}

TPTEST(BTree, WholeSmallFuzzRandomTS) {
    btree_fuzz_test(true, true, 300);
}

TPTEST(BTree, WholeLargeFuzz) {
    btree_fuzz_test(false, false, 1000);
}

TPTEST(BTree, WholeLargeFuzzRandomTS) {
    btree_fuzz_test(false, true, 1000);
}

TPTEST(BTree, RemoveInOrder) {
    BTreeTestContext ctx;
    rng_t rng;

    for (int i = 0; i < 1000; i++) {
        ctx.set(store_key_t(random_letter_string(&rng, 1, 250)),
                random_letter_string(&rng, 0, 250));
    }

    ctx.verify();

    while (!ctx.is_empty()) {
        ctx.remove(ctx.lowest_key());

        if (rng.randint(50) == 0) {
            ctx.verify();
        }
    }

    ctx.verify();
}

TPTEST(BTree, RemoveRandomOrder) {
    BTreeTestContext ctx;
    rng_t rng;

    for (int i = 0; i < 1000; i++) {
        ctx.set(store_key_t(random_letter_string(&rng, 1, 250)),
                random_letter_string(&rng, 0, 250));
    }

    ctx.verify();

    while (!ctx.is_empty()) {
        ctx.remove(ctx.pick_random_key(&rng));

        if (rng.randint(50) == 0) {
            ctx.verify();
        }
    }

    ctx.verify();
}

}
