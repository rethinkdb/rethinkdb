// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "arch/io/disk.hpp"
#include "btree/btree_store.hpp"
#include "btree/operations.hpp"
#include "buffer_cache/blob.hpp"
#include "unittest/unittest_utils.hpp"
#include "rdb_protocol/btree.hpp"
#include "rdb_protocol/protocol.hpp"
#include "serializer/config.hpp"

namespace unittest {

void run_sindex_low_level_operations_test() {
    temp_file_t temp_file;

    io_backender_t io_backender;

    filepath_file_opener_t file_opener(temp_file.name(), &io_backender);
    standard_serializer_t::create(
        &file_opener,
        standard_serializer_t::static_config_t());

    standard_serializer_t serializer(
        standard_serializer_t::dynamic_config_t(),
        &file_opener,
        &get_global_perfmon_collection());

    cache_t::create(&serializer);

    mirrored_cache_config_t cache_dynamic_config;
    cache_t cache(&serializer, cache_dynamic_config, &get_global_perfmon_collection());

    //Passing in blank metainfo. We don't need metainfo for this unittest.
    btree_slice_t::create(&cache, std::vector<char>(), std::vector<char>());

    btree_slice_t btree(&cache, &get_global_perfmon_collection(), "unittest");

    order_source_t order_source;

    std::map<std::string, secondary_index_t> mirror;

    {
        order_token_t otok = order_source.check_in("sindex unittest");
        scoped_ptr_t<transaction_t> txn;
        scoped_ptr_t<real_superblock_t> superblock;
        get_btree_superblock_and_txn(&btree, rwi_write, 1, repli_timestamp_t::invalid, otok, WRITE_DURABILITY_SOFT, &superblock, &txn);

        buf_lock_t sindex_block(txn.get(), superblock->get_sindex_block_id(), rwi_write);

        initialize_secondary_indexes(txn.get(), &sindex_block);
    }

    for (int i = 0; i < 100; ++i) {
        std::string id = uuid_to_str(generate_uuid());

        secondary_index_t s;
        s.superblock = randint(1000);

        std::string opaque_blob = rand_string(1000);
        s.opaque_definition.assign(opaque_blob.begin(), opaque_blob.end());

        mirror[id] = s;

        order_token_t otok = order_source.check_in("sindex unittest");
        scoped_ptr_t<transaction_t> txn;
        scoped_ptr_t<real_superblock_t> superblock;
        get_btree_superblock_and_txn(&btree, rwi_write, 1, repli_timestamp_t::invalid, otok, WRITE_DURABILITY_SOFT, &superblock, &txn);
        buf_lock_t sindex_block(txn.get(), superblock->get_sindex_block_id(), rwi_write);

        set_secondary_index(txn.get(), &sindex_block, id, s);
    }

    {
        order_token_t otok = order_source.check_in("sindex unittest");
        scoped_ptr_t<transaction_t> txn;
        scoped_ptr_t<real_superblock_t> superblock;
        get_btree_superblock_and_txn(&btree, rwi_write, 1, repli_timestamp_t::invalid, otok, WRITE_DURABILITY_SOFT, &superblock, &txn);
        buf_lock_t sindex_block(txn.get(), superblock->get_sindex_block_id(), rwi_write);

        std::map<std::string, secondary_index_t> sindexes;
        get_secondary_indexes(txn.get(), &sindex_block, &sindexes);

        ASSERT_TRUE(sindexes == mirror);
    }
}

TEST(BTreeSindex, LowLevelOps) {
    run_in_thread_pool(&run_sindex_low_level_operations_test);
}

void run_sindex_btree_store_api_test() {
    temp_file_t temp_file;

    io_backender_t io_backender;

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

            scoped_ptr_t<transaction_t> txn;
            scoped_ptr_t<real_superblock_t> super_block;

            store.acquire_superblock_for_write(rwi_write, repli_timestamp_t::invalid,
                                               1, WRITE_DURABILITY_SOFT, &token_pair,
                                               &txn, &super_block, &dummy_interuptor);

            UNUSED bool b =store.add_sindex(
                &token_pair,
                id,
                std::vector<char>(),
                txn.get(),
                super_block.get(),
                &dummy_interuptor);
        }

        {
            write_token_pair_t token_pair;
            store.new_write_token_pair(&token_pair);

            scoped_ptr_t<transaction_t> txn;
            scoped_ptr_t<real_superblock_t> super_block;

            store.acquire_superblock_for_write(rwi_write, repli_timestamp_t::invalid,
                                               1, WRITE_DURABILITY_SOFT, &token_pair,
                                               &txn, &super_block, &dummy_interuptor);

            scoped_ptr_t<buf_lock_t> sindex_block;
            store.acquire_sindex_block_for_write(
                    &token_pair, txn.get(), &sindex_block,
                    super_block->get_sindex_block_id(), &dummy_interuptor);

            store.mark_index_up_to_date(id, txn.get(), sindex_block.get());
        }

        {
            //Insert a piece of data in to the btree.
            write_token_pair_t token_pair;
            store.new_write_token_pair(&token_pair);

            scoped_ptr_t<transaction_t> txn;
            scoped_ptr_t<real_superblock_t> super_block;

            store.acquire_superblock_for_write(rwi_write,
                    repli_timestamp_t::invalid, 1, WRITE_DURABILITY_SOFT,
                    &token_pair, &txn, &super_block,
                    &dummy_interuptor);

            scoped_ptr_t<real_superblock_t> sindex_super_block;

            bool sindex_exists = store.acquire_sindex_superblock_for_write(id,
                    super_block->get_sindex_block_id(), &token_pair, txn.get(),
                    &sindex_super_block, &dummy_interuptor);
            ASSERT_TRUE(sindex_exists);

            boost::shared_ptr<scoped_cJSON_t> data(new
                    scoped_cJSON_t(cJSON_CreateNumber(1)));

            rdb_protocol_t::point_write_response_t response;
            rdb_modification_info_t mod_info;

            store_key_t key("foo");
            rdb_set(key, data, true, store.get_sindex_slice(id),
                    repli_timestamp_t::invalid, txn.get(),
                    sindex_super_block.get(), &response,
                    &mod_info);
        }

        {
            //Read that data
            read_token_pair_t token_pair;
            store.new_read_token_pair(&token_pair);

            scoped_ptr_t<transaction_t> txn;
            scoped_ptr_t<real_superblock_t> main_sb;
            scoped_ptr_t<real_superblock_t> sindex_super_block;

            store.acquire_superblock_for_read(rwi_read,
                    &token_pair.main_read_token, &txn, &main_sb,
                    &dummy_interuptor, true);

            store_key_t key("foo");

            bool sindex_exists = store.acquire_sindex_superblock_for_read(id,
                    main_sb->get_sindex_block_id(), &token_pair,
                    txn.get(), &sindex_super_block,
                    static_cast<std::vector<char>*>(NULL), &dummy_interuptor);
            ASSERT_TRUE(sindex_exists);

            point_read_response_t response;

            rdb_get(key, store.get_sindex_slice(id), txn.get(),
                    sindex_super_block.get(), &response);

            boost::shared_ptr<scoped_cJSON_t> data(new
                    scoped_cJSON_t(cJSON_CreateNumber(1)));

            ASSERT_EQ(query_language::json_cmp(response.data->get(),
                        data->get()), 0);
        }
    }

    for (auto it  = created_sindexs.begin(); it != created_sindexs.end(); ++it) {
        /* Drop the sindex */
        write_token_pair_t token_pair;
        store.new_write_token_pair(&token_pair);

        scoped_ptr_t<transaction_t> txn;
        scoped_ptr_t<real_superblock_t> super_block;

        store.acquire_superblock_for_write(rwi_write, repli_timestamp_t::invalid,
                                           1, WRITE_DURABILITY_SOFT, &token_pair,
                                           &txn, &super_block, &dummy_interuptor);

        value_sizer_t<rdb_value_t> sizer(store.cache->get_block_size());

        rdb_value_deleter_t deleter;

        store.drop_sindex( &token_pair, *it,
                txn.get(), super_block.get(), &sizer,
                &deleter, &dummy_interuptor);
    }
}

TEST(BTreeSindex, BtreeStoreAPI) {
    run_in_thread_pool(&run_sindex_btree_store_api_test);
}

} // namespace unittest
