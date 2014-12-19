// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "arch/io/disk.hpp"
#include "btree/operations.hpp"
#include "btree/slice.hpp"
#include "buffer_cache/alt.hpp"
#include "buffer_cache/blob.hpp"
#include "buffer_cache/cache_balancer.hpp"
#include "containers/uuid.hpp"
#include "unittest/unittest_utils.hpp"
#include "rdb_protocol/btree.hpp"
#include "rdb_protocol/store.hpp"
#include "rdb_protocol/protocol.hpp"
#include "serializer/config.hpp"

namespace unittest {

TPTEST(BTreeSindex, LowLevelOps) {
    temp_file_t temp_file;

    io_backender_t io_backender(file_direct_io_mode_t::buffered_desired);
    dummy_cache_balancer_t balancer(GIGABYTE);

    filepath_file_opener_t file_opener(temp_file.name(), &io_backender);
    standard_serializer_t::create(
        &file_opener,
        standard_serializer_t::static_config_t());

    standard_serializer_t serializer(
        standard_serializer_t::dynamic_config_t(),
        &file_opener,
        &get_global_perfmon_collection());

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

    order_source_t order_source;

    std::map<sindex_name_t, secondary_index_t> mirror;

    {
        scoped_ptr_t<txn_t> txn;
        scoped_ptr_t<real_superblock_t> superblock;
        get_btree_superblock_and_txn(&cache_conn, write_access_t::write, 1,
                                     repli_timestamp_t::distant_past,
                                     write_durability_t::SOFT,
                                     &superblock, &txn);

        buf_lock_t sindex_block(superblock->expose_buf(),
                                superblock->get_sindex_block_id(),
                                access_t::write);

        initialize_secondary_indexes(&sindex_block);
    }

    for (int i = 0; i < 100; ++i) {
        sindex_name_t name(uuid_to_str(generate_uuid()));

        secondary_index_t s;
        s.superblock = randint(1000);

        std::string opaque_blob = rand_string(1000);
        s.opaque_definition.assign(opaque_blob.begin(), opaque_blob.end());

        mirror[name] = s;

        scoped_ptr_t<txn_t> txn;
        scoped_ptr_t<real_superblock_t> superblock;
        get_btree_superblock_and_txn(&cache_conn, write_access_t::write, 1,
                                     repli_timestamp_t::distant_past,
                                     write_durability_t::SOFT,
                                     &superblock, &txn);
        buf_lock_t sindex_block(superblock->expose_buf(),
                                superblock->get_sindex_block_id(),
                                access_t::write);

        set_secondary_index(&sindex_block, name, s);
    }

    {
        scoped_ptr_t<txn_t> txn;
        scoped_ptr_t<real_superblock_t> superblock;
        get_btree_superblock_and_txn(&cache_conn, write_access_t::write, 1,
                                     repli_timestamp_t::distant_past,
                                     write_durability_t::SOFT,
                                     &superblock, &txn);
        buf_lock_t sindex_block(superblock->expose_buf(),
                                superblock->get_sindex_block_id(),
                                access_t::write);

        std::map<sindex_name_t, secondary_index_t> sindexes;
        get_secondary_indexes(&sindex_block, &sindexes);

        auto it = sindexes.begin();
        auto jt = mirror.begin();

        for (;;) {
            if (it == sindexes.end()) {
                ASSERT_TRUE(jt == mirror.end());
                break;
            }
            ASSERT_TRUE(jt != mirror.end());

            ASSERT_TRUE(it->first == jt->first);
            ASSERT_TRUE(it->second.superblock == jt->second.superblock &&
                        it->second.opaque_definition == jt->second.opaque_definition);
            ++it;
            ++jt;
        }
    }
}

TPTEST(BTreeSindex, BtreeStoreAPI) {
    temp_file_t temp_file;

    io_backender_t io_backender(file_direct_io_mode_t::buffered_desired);
    dummy_cache_balancer_t balancer(GIGABYTE);

    filepath_file_opener_t file_opener(temp_file.name(), &io_backender);
    standard_serializer_t::create(
        &file_opener,
        standard_serializer_t::static_config_t());

    standard_serializer_t serializer(
        standard_serializer_t::dynamic_config_t(),
        &file_opener,
        &get_global_perfmon_collection());

    store_t store(
            &serializer,
            &balancer,
            "unit_test_store",
            true,
            &get_global_perfmon_collection(),
            NULL,
            &io_backender,
            base_path_t("."),
            NULL,
            generate_uuid());

    cond_t dummy_interruptor;

    std::set<sindex_name_t> created_sindexs;

    for (int i = 0; i < 50; ++i) {
        sindex_name_t name = sindex_name_t(uuid_to_str(generate_uuid()));
        created_sindexs.insert(name);
        {
            write_token_t token;
            store.new_write_token(&token);

            scoped_ptr_t<txn_t> txn;
            scoped_ptr_t<real_superblock_t> super_block;

            store.acquire_superblock_for_write(repli_timestamp_t::distant_past,
                    1, write_durability_t::SOFT, &token,
                    &txn, &super_block, &dummy_interruptor);

            buf_lock_t sindex_block(super_block->expose_buf(),
                                    super_block->get_sindex_block_id(),
                                    access_t::write);

            UNUSED bool b = store.add_sindex(name, std::vector<char>(), &sindex_block);
        }

        {
            write_token_t token;
            store.new_write_token(&token);

            scoped_ptr_t<txn_t> txn;
            scoped_ptr_t<real_superblock_t> super_block;

            store.acquire_superblock_for_write(repli_timestamp_t::distant_past,
                                               1, write_durability_t::SOFT, &token,
                                               &txn, &super_block, &dummy_interruptor);

            buf_lock_t sindex_block(super_block->expose_buf(),
                                    super_block->get_sindex_block_id(),
                                    access_t::write);

            store.mark_index_up_to_date(name, &sindex_block);
        }

        {
            //Insert a piece of data in to the btree.
            write_token_t token;
            store.new_write_token(&token);

            scoped_ptr_t<txn_t> txn;
            scoped_ptr_t<real_superblock_t> super_block;

            store.acquire_superblock_for_write(
                    repli_timestamp_t::distant_past, 1, write_durability_t::SOFT,
                    &token, &txn, &super_block,
                    &dummy_interruptor);

            scoped_ptr_t<real_superblock_t> sindex_super_block;
            uuid_u sindex_uuid;

            bool sindex_exists = store.acquire_sindex_superblock_for_write(
                    name,
                    "",
                    super_block.get(),
                    &sindex_super_block,
                    &sindex_uuid);
            ASSERT_TRUE(sindex_exists);

            ql::datum_t data = ql::datum_t(1.0);

            point_write_response_t response;
            rdb_modification_info_t mod_info;

            store_key_t key("foo");
            rdb_live_deletion_context_t deletion_context;
            rdb_set(key, data, true, store.get_sindex_slice(sindex_uuid),
                    repli_timestamp_t::distant_past,
                    sindex_super_block.get(), &deletion_context, &response,
                    &mod_info, static_cast<profile::trace_t *>(NULL));
        }

        {
            //Read that data
            read_token_t token;
            store.new_read_token(&token);

            scoped_ptr_t<txn_t> txn;
            scoped_ptr_t<real_superblock_t> main_sb;
            scoped_ptr_t<real_superblock_t> sindex_super_block;
            uuid_u sindex_uuid;

            store.acquire_superblock_for_read(
                    &token, &txn, &main_sb,
                    &dummy_interruptor, true);

            store_key_t key("foo");

            {
                std::vector<char> opaque_definition;
                bool sindex_exists = store.acquire_sindex_superblock_for_read(
                        name,
                        "",
                        main_sb.get(),
                        &sindex_super_block,
                        &opaque_definition,
                        &sindex_uuid);
                ASSERT_TRUE(sindex_exists);
            }

            point_read_response_t response;

            rdb_get(key, store.get_sindex_slice(sindex_uuid),
                    sindex_super_block.get(), &response, NULL);

            ASSERT_EQ(ql::datum_t(1.0), response.data);
        }
    }

    for (auto it  = created_sindexs.begin(); it != created_sindexs.end(); ++it) {
        /* Drop the sindex */
        write_token_t token;
        store.new_write_token(&token);

        scoped_ptr_t<txn_t> txn;
        scoped_ptr_t<real_superblock_t> super_block;

        store.acquire_superblock_for_write(repli_timestamp_t::distant_past,
                                           1, write_durability_t::SOFT, &token,
                                           &txn, &super_block, &dummy_interruptor);

        buf_lock_t sindex_block(super_block->expose_buf(),
                                super_block->get_sindex_block_id(),
                                access_t::write);

        store.drop_sindex(*it, &sindex_block);
    }
}

} // namespace unittest
