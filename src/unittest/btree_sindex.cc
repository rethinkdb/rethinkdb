// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "arch/io/disk.hpp"
#include "btree/btree_store.hpp"
#include "btree/operations.hpp"
#include "buffer_cache/blob.hpp"
#include "mock/unittest_utils.hpp"
#include "rdb_protocol/btree.hpp"
#include "rdb_protocol/protocol.hpp"
#include "serializer/config.hpp"

namespace unittest {

void run_sindex_low_level_operations_test() {
    mock::temp_file_t temp_file("/tmp/rdb_unittest.XXXXXX");

    scoped_ptr_t<io_backender_t> io_backender;
    make_io_backender(aio_default, &io_backender);

    filepath_file_opener_t file_opener(temp_file.name(), io_backender.get());
    standard_serializer_t::create(
        &file_opener,
        standard_serializer_t::static_config_t());

    standard_serializer_t serializer(
        standard_serializer_t::dynamic_config_t(),
        &file_opener,
        &get_global_perfmon_collection());

    mirrored_cache_static_config_t cache_static_config;
    cache_t::create(&serializer, &cache_static_config);

    mirrored_cache_config_t cache_dynamic_config;
    cache_t cache(&serializer, &cache_dynamic_config, &get_global_perfmon_collection());

    btree_slice_t::create(&cache);

    btree_slice_t btree(&cache, &get_global_perfmon_collection());

    order_source_t order_source;

    std::map<uuid_t, secondary_index_t> mirror;

    {
        order_token_t otok = order_source.check_in("sindex unittest");
        scoped_ptr_t<transaction_t> txn;
        scoped_ptr_t<real_superblock_t> superblock;
        get_btree_superblock_and_txn(&btree, rwi_write, 1, repli_timestamp_t::invalid, otok, &superblock, &txn);
        buf_lock_t *sb_buf = superblock->get();

        initialize_secondary_indexes(txn.get(), sb_buf);
    }

    for (int i = 0; i < 100; ++i) {
        uuid_t uuid = generate_uuid();

        secondary_index_t s;
        s.superblock = randint(1000);

        std::string opaque_blob = rand_string(1000);
        s.opaque_definition.assign(opaque_blob.begin(), opaque_blob.end());

        mirror[uuid] = s;

        order_token_t otok = order_source.check_in("sindex unittest");
        scoped_ptr_t<transaction_t> txn;
        scoped_ptr_t<real_superblock_t> superblock;
        get_btree_superblock_and_txn(&btree, rwi_write, 1, repli_timestamp_t::invalid, otok, &superblock, &txn);
        buf_lock_t *sb_buf = superblock->get();

        add_secondary_index(txn.get(), sb_buf, uuid, s);
    }

    {
        order_token_t otok = order_source.check_in("sindex unittest");
        scoped_ptr_t<transaction_t> txn;
        scoped_ptr_t<real_superblock_t> superblock;
        get_btree_superblock_and_txn(&btree, rwi_write, 1, repli_timestamp_t::invalid, otok, &superblock, &txn);
        buf_lock_t *sb_buf = superblock->get();

        std::map<uuid_t, secondary_index_t> sindexes;
        get_secondary_indexes(txn.get(), sb_buf, &sindexes);

        ASSERT_TRUE(sindexes == mirror);
    }
}

TEST(BTreeSindex, LowLevelOps) {
    mock::run_in_thread_pool(&run_sindex_low_level_operations_test);
}

void run_sindex_btree_store_api_test() {
    mock::temp_file_t temp_file("/tmp/rdb_unittest.XXXXXX");

    scoped_ptr_t<io_backender_t> io_backender;
    make_io_backender(aio_default, &io_backender);

    filepath_file_opener_t file_opener(temp_file.name(), io_backender.get());
    standard_serializer_t::create(
        &file_opener,
        standard_serializer_t::static_config_t());

    standard_serializer_t serializer(
        standard_serializer_t::dynamic_config_t(),
        &file_opener,
        &get_global_perfmon_collection());

    rdb_protocol_t::store_t store(
            &serializer,
            "unit_test_store",
            GIGABYTE,
            true,
            &get_global_perfmon_collection(),
            NULL);

    cond_t dummy_interuptor;
    uuid_t id = generate_uuid();
    {
        object_buffer_t<fifo_enforcer_sink_t::exit_write_t> token;
        store.new_write_token(&token);

        scoped_ptr_t<transaction_t> txn;
        scoped_ptr_t<real_superblock_t> super_block;

        store.acquire_superblock_for_write(rwi_write, repli_timestamp_t::invalid,
                                           1, &token, &txn, &super_block, &dummy_interuptor);

        store.add_secondary_index(
                id,
                std::vector<unsigned char>(),
                txn.get(),
                super_block->get(),
                &dummy_interuptor);
    }

    {
        object_buffer_t<fifo_enforcer_sink_t::exit_write_t> token;
        store.new_write_token(&token);

        scoped_ptr_t<transaction_t> txn;
        scoped_ptr_t<real_superblock_t> super_block;

        store.acquire_superblock_for_write(rwi_write, repli_timestamp_t::invalid,
                                           1, &token, &txn, &super_block, &dummy_interuptor);

        value_sizer_t<rdb_value_t> sizer(store.cache->get_block_size());

        rdb_value_deleter_t deleter;

        store.drop_secondary_index(
                id,
                txn.get(),
                super_block->get(),
                &sizer,
                &deleter,
                &dummy_interuptor);
    }
}

TEST(BTreeSindex, BtreeStoreAPI) {
    mock::run_in_thread_pool(&run_sindex_btree_store_api_test);
}

} // namespace unittest
