// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "arch/io/disk.hpp"
#include "btree/btree_store.hpp"
#include "btree/operations.hpp"
#include "btree/slice.hpp"  // RSI: for SLICE_ALT
#if SLICE_ALT
#include "buffer_cache/alt/alt.hpp"
#endif
#include "buffer_cache/blob.hpp"
#include "unittest/unittest_utils.hpp"
#include "rdb_protocol/btree.hpp"
#include "rdb_protocol/protocol.hpp"
#include "serializer/config.hpp"

#if SLICE_ALT
using namespace alt;  // RSI
#endif

namespace unittest {

void run_sindex_low_level_operations_test() {
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
    mirrored_cache_config_t cache_dynamic_config;
    cache_t cache(&serializer, cache_dynamic_config, &get_global_perfmon_collection());
#endif

    //Passing in blank metainfo. We don't need metainfo for this unittest.
    btree_slice_t::create(&cache, std::vector<char>(), std::vector<char>());

    btree_slice_t btree(&cache, &get_global_perfmon_collection(), "unittest");

    order_source_t order_source;

    std::map<std::string, secondary_index_t> mirror;

    {
        order_token_t otok = order_source.check_in("sindex unittest");
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
        alt_buf_lock_t sindex_block(superblock->expose_buf(),
                                    superblock->get_sindex_block_id(),
                                    alt_access_t::write);
#else
        buf_lock_t sindex_block(txn.get(), superblock->get_sindex_block_id(), rwi_write);
#endif

#if SLICE_ALT
        initialize_secondary_indexes(&sindex_block);
#else
        initialize_secondary_indexes(txn.get(), &sindex_block);
#endif
    }

    for (int i = 0; i < 100; ++i) {
        std::string id = uuid_to_str(generate_uuid());

        secondary_index_t s;
        s.superblock = randint(1000);

        std::string opaque_blob = rand_string(1000);
        s.opaque_definition.assign(opaque_blob.begin(), opaque_blob.end());

        mirror[id] = s;

        order_token_t otok = order_source.check_in("sindex unittest");
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
        alt_buf_lock_t sindex_block(superblock->expose_buf(),
                                    superblock->get_sindex_block_id(),
                                    alt_access_t::write);
#else
        buf_lock_t sindex_block(txn.get(), superblock->get_sindex_block_id(), rwi_write);
#endif

#if SLICE_ALT
        set_secondary_index(&sindex_block, id, s);
#else
        set_secondary_index(txn.get(), &sindex_block, id, s);
#endif
    }

    {
        order_token_t otok = order_source.check_in("sindex unittest");
#if SLICE_ALT
        scoped_ptr_t<alt_txn_t> txn;
#else
        scoped_ptr_t<transaction_t> txn;
#endif
        scoped_ptr_t<real_superblock_t> superblock;
#if SLICE_ALT
        get_btree_superblock_and_txn(&btree, alt_access_t::write, 1,
                                     repli_timestamp_t::invalid,
                                     otok, WRITE_DURABILITY_SOFT,
                                     &superblock, &txn);
#else
        get_btree_superblock_and_txn(&btree, rwi_write, rwi_write, 1, repli_timestamp_t::invalid, otok, WRITE_DURABILITY_SOFT, &superblock, &txn);
#endif
#if SLICE_ALT
        alt_buf_lock_t sindex_block(superblock->expose_buf(),
                                    superblock->get_sindex_block_id(),
                                    alt_access_t::write);
#else
        buf_lock_t sindex_block(txn.get(), superblock->get_sindex_block_id(), rwi_write);
#endif

        std::map<std::string, secondary_index_t> sindexes;
#if SLICE_ALT
        get_secondary_indexes(&sindex_block, &sindexes);
#else
        get_secondary_indexes(txn.get(), &sindex_block, &sindexes);
#endif

        ASSERT_TRUE(sindexes == mirror);
    }
}

TEST(BTreeSindex, LowLevelOps) {
    run_in_thread_pool(&run_sindex_low_level_operations_test);
}

void run_sindex_btree_store_api_test() {
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

    rdb_protocol_t::store_t store(
            &serializer,
            "unit_test_store",
            GIGABYTE,
            true,
            &get_global_perfmon_collection(),
            NULL,
            &io_backender,
            base_path_t("."));

    cond_t dummy_interuptor;

    std::set<std::string> created_sindexs;

    for (int i = 0; i < 50; ++i) {
        std::string id = uuid_to_str(generate_uuid());
        created_sindexs.insert(id);
        {
            write_token_pair_t token_pair;
            store.new_write_token_pair(&token_pair);

#if SLICE_ALT
            scoped_ptr_t<alt_txn_t> txn;
#else
            scoped_ptr_t<transaction_t> txn;
#endif
            scoped_ptr_t<real_superblock_t> super_block;

            store.acquire_superblock_for_write(repli_timestamp_t::invalid,
                                               1, WRITE_DURABILITY_SOFT, &token_pair,
                                               &txn, &super_block, &dummy_interuptor);

            UNUSED bool b = store.add_sindex(
                &token_pair,
                id,
                std::vector<char>(),
#if !SLICE_ALT
                txn.get(),
#endif
                super_block.get(),
                &dummy_interuptor);
        }

        {
            write_token_pair_t token_pair;
            store.new_write_token_pair(&token_pair);

#if SLICE_ALT
            scoped_ptr_t<alt_txn_t> txn;
#else
            scoped_ptr_t<transaction_t> txn;
#endif
            scoped_ptr_t<real_superblock_t> super_block;

            store.acquire_superblock_for_write(repli_timestamp_t::invalid,
                                               1, WRITE_DURABILITY_SOFT, &token_pair,
                                               &txn, &super_block, &dummy_interuptor);

#if SLICE_ALT
            scoped_ptr_t<alt_buf_lock_t> sindex_block;
            store.acquire_sindex_block_for_write(
                    &token_pair, super_block->expose_buf(), &sindex_block,
                    super_block->get_sindex_block_id(), &dummy_interuptor);
#else
            scoped_ptr_t<buf_lock_t> sindex_block;
            store.acquire_sindex_block_for_write(
                    &token_pair, txn.get(), &sindex_block,
                    super_block->get_sindex_block_id(), &dummy_interuptor);
#endif

#if SLICE_ALT
            store.mark_index_up_to_date(id, sindex_block.get());
#else
            store.mark_index_up_to_date(id, txn.get(), sindex_block.get());
#endif
        }

        {
            //Insert a piece of data in to the btree.
            write_token_pair_t token_pair;
            store.new_write_token_pair(&token_pair);

#if SLICE_ALT
            scoped_ptr_t<alt_txn_t> txn;
#else
            scoped_ptr_t<transaction_t> txn;
#endif
            scoped_ptr_t<real_superblock_t> super_block;

            store.acquire_superblock_for_write(
                    repli_timestamp_t::invalid, 1, WRITE_DURABILITY_SOFT,
                    &token_pair, &txn, &super_block,
                    &dummy_interuptor);

            scoped_ptr_t<real_superblock_t> sindex_super_block;

#if SLICE_ALT
            bool sindex_exists = store.acquire_sindex_superblock_for_write(id,
                    super_block->get_sindex_block_id(), &token_pair,
                    super_block->expose_buf(),
                    &sindex_super_block, &dummy_interuptor);
#else
            bool sindex_exists = store.acquire_sindex_superblock_for_write(id,
                    super_block->get_sindex_block_id(), &token_pair, txn.get(),
                    &sindex_super_block, &dummy_interuptor);
#endif
            ASSERT_TRUE(sindex_exists);

            counted_t<const ql::datum_t> data = make_counted<ql::datum_t>(1.0);

            rdb_protocol_t::point_write_response_t response;
            rdb_modification_info_t mod_info;

            store_key_t key("foo");
#if SLICE_ALT
            rdb_set(key, data, true, store.get_sindex_slice(id),
                    repli_timestamp_t::invalid,
                    sindex_super_block.get(), &response,
                    &mod_info, static_cast<profile::trace_t *>(NULL));
#else
            rdb_set(key, data, true, store.get_sindex_slice(id),
                    repli_timestamp_t::invalid, txn.get(),
                    sindex_super_block.get(), &response,
                    &mod_info, static_cast<profile::trace_t *>(NULL));
#endif
        }

        {
            //Read that data
            read_token_pair_t token_pair;
            store.new_read_token_pair(&token_pair);

#if SLICE_ALT
            scoped_ptr_t<alt_txn_t> txn;
#else
            scoped_ptr_t<transaction_t> txn;
#endif
            scoped_ptr_t<real_superblock_t> main_sb;
            scoped_ptr_t<real_superblock_t> sindex_super_block;

#if SLICE_ALT
            store.acquire_superblock_for_read(
                    &token_pair.main_read_token, &txn, &main_sb,
                    &dummy_interuptor, true);
#else
            store.acquire_superblock_for_read(rwi_read,
                    &token_pair.main_read_token, &txn, &main_sb,
                    &dummy_interuptor, true);
#endif

            store_key_t key("foo");

#if SLICE_ALT
            bool sindex_exists = store.acquire_sindex_superblock_for_read(id,
                    main_sb->get_sindex_block_id(), &token_pair,
                    main_sb->expose_buf(), &sindex_super_block,
                    static_cast<std::vector<char>*>(NULL), &dummy_interuptor);
#else
            bool sindex_exists = store.acquire_sindex_superblock_for_read(id,
                    main_sb->get_sindex_block_id(), &token_pair,
                    txn.get(), &sindex_super_block,
                    static_cast<std::vector<char>*>(NULL), &dummy_interuptor);
#endif
            ASSERT_TRUE(sindex_exists);

            point_read_response_t response;

#if SLICE_ALT
            rdb_get(key, store.get_sindex_slice(id),
                    sindex_super_block.get(), &response, NULL);
#else
            rdb_get(key, store.get_sindex_slice(id), txn.get(),
                    sindex_super_block.get(), &response, NULL);
#endif

            ASSERT_EQ(ql::datum_t(1.0), *response.data);
        }
    }

    for (auto it  = created_sindexs.begin(); it != created_sindexs.end(); ++it) {
        /* Drop the sindex */
        write_token_pair_t token_pair;
        store.new_write_token_pair(&token_pair);

#if SLICE_ALT
        scoped_ptr_t<alt_txn_t> txn;
#else
        scoped_ptr_t<transaction_t> txn;
#endif
        scoped_ptr_t<real_superblock_t> super_block;

        store.acquire_superblock_for_write(repli_timestamp_t::invalid,
                                           1, WRITE_DURABILITY_SOFT, &token_pair,
                                           &txn, &super_block, &dummy_interuptor);

        value_sizer_t<rdb_value_t> sizer(store.cache->get_block_size());

        rdb_value_deleter_t deleter;

#if SLICE_ALT
        store.drop_sindex( &token_pair, *it,
                super_block.get(), &sizer,
                &deleter, &dummy_interuptor);
#else
        store.drop_sindex( &token_pair, *it,
                txn.get(), super_block.get(), &sizer,
                &deleter, &dummy_interuptor);
#endif
    }
}

TEST(BTreeSindex, BtreeStoreAPI) {
    run_in_thread_pool(&run_sindex_btree_store_api_test);
}

} // namespace unittest
