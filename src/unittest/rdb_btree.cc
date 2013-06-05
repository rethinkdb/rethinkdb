// Copyright 2010-2013 RethinkDB, all rights reserved.

#include "errors.hpp"

#include <boost/bind.hpp>

#include "arch/io/disk.hpp"
#include "btree/btree_store.hpp"
#include "buffer_cache/mirrored/config.hpp"
#include "containers/archive/boost_types.hpp"
#include "rdb_protocol/btree.hpp"
#include "rdb_protocol/pb_utils.hpp"
#include "rdb_protocol/proto_utils.hpp"
#include "rdb_protocol/protocol.hpp"
#include "serializer/log/log_serializer.hpp"
#include "unittest/gtest.hpp"
#include "unittest/unittest_utils.hpp"

#define TOTAL_KEYS_TO_INSERT 1000

#pragma GCC diagnostic ignored "-Wshadow"

namespace unittest {

void insert_rows(int start, int finish, btree_store_t<rdb_protocol_t> *store) {
    guarantee(start <= finish);
    for (int i = start; i < finish; ++i) {
        cond_t dummy_interruptor;
        scoped_ptr_t<transaction_t> txn;
        scoped_ptr_t<real_superblock_t> superblock;
        write_token_pair_t token_pair;
        store->new_write_token_pair(&token_pair);
        store->acquire_superblock_for_write(rwi_write, repli_timestamp_t::invalid,
                                            1, WRITE_DURABILITY_SOFT,
                                            &token_pair, &txn, &superblock, &dummy_interruptor);
        block_id_t sindex_block_id = superblock->get_sindex_block_id();

        std::string data = strprintf("{\"id\" : %d, \"sid\" : %d}", i, i * i);
        point_write_response_t response;

        store_key_t pk(cJSON_print_primary(scoped_cJSON_t(cJSON_CreateNumber(i)).get(), backtrace_t()));
        rdb_modification_report_t mod_report(pk);
        rdb_set(pk, boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_Parse(data.c_str()))),
                false, store->btree.get(), repli_timestamp_t::invalid, txn.get(),
                superblock.get(), &response, &mod_report.info);

        {
            scoped_ptr_t<buf_lock_t> sindex_block;
            store->acquire_sindex_block_for_write(
                    &token_pair, txn.get(), &sindex_block,
                    sindex_block_id, &dummy_interruptor);

            btree_store_t<rdb_protocol_t>::sindex_access_vector_t sindexes;
            store->aquire_post_constructed_sindex_superblocks_for_write(
                     sindex_block.get(), txn.get(), &sindexes);
            rdb_update_sindexes(sindexes, &mod_report, txn.get());

            mutex_t::acq_t acq;
            store->lock_sindex_queue(sindex_block.get(), &acq);

            write_message_t wm;
            wm << rdb_sindex_change_t(mod_report);

            store->sindex_queue_push(wm, &acq);
        }
    }
}

void insert_rows_and_pulse_when_done(int start, int finish,
        btree_store_t<rdb_protocol_t> *store, cond_t *pulse_when_done) {
    insert_rows(start, finish, store);
    pulse_when_done->pulse();
}

std::string create_sindex(btree_store_t<rdb_protocol_t> *store) {
    cond_t dummy_interruptor;
    std::string sindex_id = uuid_to_str(generate_uuid());
    write_token_pair_t token_pair;
    store->new_write_token_pair(&token_pair);

    scoped_ptr_t<transaction_t> txn;
    scoped_ptr_t<real_superblock_t> super_block;

    store->acquire_superblock_for_write(rwi_write, repli_timestamp_t::invalid,
                                        1, WRITE_DURABILITY_SOFT,
                                        &token_pair, &txn, &super_block, &dummy_interruptor);

    Term mapping;
    Term *arg = ql::pb::set_func(&mapping, 1);
    N2(GETATTR, NVAR(1), NDATUM("sid"));

    ql::map_wire_func_t m(mapping, std::map<int64_t, Datum>());

    write_message_t wm;
    wm << m;

    vector_stream_t stream;
    int res = send_write_message(&stream, &wm);
    guarantee(res == 0);

    UNUSED bool b = store->add_sindex(
            &token_pair,
            sindex_id,
            stream.vector(),
            txn.get(),
            super_block.get(),
            &dummy_interruptor);
    return sindex_id;
}

void drop_sindex(btree_store_t<rdb_protocol_t> *store,
                 const std::string &sindex_id) {
    cond_t dummy_interuptor;
    write_token_pair_t token_pair;
    store->new_write_token_pair(&token_pair);

    scoped_ptr_t<transaction_t> txn;
    scoped_ptr_t<real_superblock_t> super_block;

    store->acquire_superblock_for_write(rwi_write, repli_timestamp_t::invalid,
                                        1, WRITE_DURABILITY_SOFT, &token_pair,
                                        &txn, &super_block, &dummy_interuptor);

    value_sizer_t<rdb_value_t> sizer(store->cache->get_block_size());
    rdb_value_deleter_t deleter;

    store->drop_sindex(
            &token_pair,
            sindex_id,
            txn.get(),
            super_block.get(),
            &sizer,
            &deleter,
            &dummy_interuptor);
}

void bring_sindexes_up_to_date(
        btree_store_t<rdb_protocol_t> *store, std::string sindex_id) {
    cond_t dummy_interruptor;
    write_token_pair_t token_pair;
    store->new_write_token_pair(&token_pair);

    scoped_ptr_t<transaction_t> txn;
    scoped_ptr_t<real_superblock_t> super_block;
    store->acquire_superblock_for_write(rwi_write, repli_timestamp_t::invalid,
                                        1, WRITE_DURABILITY_SOFT,
                                        &token_pair, &txn, &super_block, &dummy_interruptor);

    scoped_ptr_t<buf_lock_t> sindex_block;
    store->acquire_sindex_block_for_write(
            &token_pair, txn.get(), &sindex_block,
            super_block->get_sindex_block_id(),
            &dummy_interruptor);

    std::set<std::string> created_sindexes;
    created_sindexes.insert(sindex_id);

    rdb_protocol_details::bring_sindexes_up_to_date(created_sindexes, store,
            sindex_block.get(), txn.get());
    nap(1000);
}

void spawn_writes_and_bring_sindexes_up_to_date(btree_store_t<rdb_protocol_t> *store,
        std::string sindex_id, cond_t *background_inserts_done) {
    cond_t dummy_interruptor;
    write_token_pair_t token_pair;
    store->new_write_token_pair(&token_pair);

    scoped_ptr_t<transaction_t> txn;
    scoped_ptr_t<real_superblock_t> super_block;
    store->acquire_superblock_for_write(rwi_write, repli_timestamp_t::invalid,
                                        1, WRITE_DURABILITY_SOFT,
                                        &token_pair, &txn, &super_block, &dummy_interruptor);

    scoped_ptr_t<buf_lock_t> sindex_block;
    store->acquire_sindex_block_for_write(
            &token_pair, txn.get(), &sindex_block,
            super_block->get_sindex_block_id(),
            &dummy_interruptor);

    coro_t::spawn_sometime(boost::bind(&insert_rows_and_pulse_when_done,
                (TOTAL_KEYS_TO_INSERT * 9) / 10, TOTAL_KEYS_TO_INSERT,
                store, background_inserts_done));

    std::set<std::string> created_sindexes;
    created_sindexes.insert(sindex_id);

    rdb_protocol_details::bring_sindexes_up_to_date(created_sindexes, store,
            sindex_block.get(), txn.get());
}

void check_keys_are_present(btree_store_t<rdb_protocol_t> *store,
        std::string sindex_id) {
    cond_t dummy_interruptor;
    for (int i = 0; i < TOTAL_KEYS_TO_INSERT; ++i) {
        read_token_pair_t token_pair;
        store->new_read_token_pair(&token_pair);

        scoped_ptr_t<transaction_t> txn;
        scoped_ptr_t<real_superblock_t> super_block;

        store->acquire_superblock_for_read(rwi_read,
                &token_pair.main_read_token, &txn, &super_block,
                &dummy_interruptor, true);

        scoped_ptr_t<real_superblock_t> sindex_sb;

        bool sindex_exists = store->acquire_sindex_superblock_for_read(sindex_id,
                super_block->get_sindex_block_id(), &token_pair,
                txn.get(), &sindex_sb,
                static_cast<std::vector<char>*>(NULL), &dummy_interruptor);
        ASSERT_TRUE(sindex_exists);

        rdb_protocol_t::rget_read_response_t res;
        rdb_rget_slice(store->get_sindex_slice(sindex_id),
               rdb_protocol_t::sindex_key_range(store_key_t(cJSON_print_primary(scoped_cJSON_t(cJSON_CreateNumber(i * i)).get(), backtrace_t())),
                                                store_key_t(cJSON_print_primary(scoped_cJSON_t(cJSON_CreateNumber(i * i)).get(), backtrace_t()))),
               txn.get(), sindex_sb.get(), NULL, rdb_protocol_details::transform_t(),
               boost::optional<rdb_protocol_details::terminal_t>(), &res);

        rdb_protocol_t::rget_read_response_t::stream_t *stream = boost::get<rdb_protocol_t::rget_read_response_t::stream_t>(&res.result);
        ASSERT_TRUE(stream != NULL);
        ASSERT_EQ(1ul, stream->size());

        std::string expected_data = strprintf("{\"id\" : %d, \"sid\" : %d}", i, i * i);
        scoped_cJSON_t expected_value(cJSON_Parse(expected_data.c_str()));

        ASSERT_EQ(0, query_language::json_cmp(expected_value.get(), stream->front().second->get()));
    }
}

void check_keys_are_NOT_present(btree_store_t<rdb_protocol_t> *store,
        std::string sindex_id) {
    /* Check that we don't have any of the keys (we just deleted them all) */
    cond_t dummy_interruptor;
    for (int i = 0; i < TOTAL_KEYS_TO_INSERT; ++i) {
        read_token_pair_t token_pair;
        store->new_read_token_pair(&token_pair);

        scoped_ptr_t<transaction_t> txn;
        scoped_ptr_t<real_superblock_t> super_block;

        store->acquire_superblock_for_read(rwi_read,
                &token_pair.main_read_token, &txn, &super_block,
                &dummy_interruptor, true);

        scoped_ptr_t<real_superblock_t> sindex_sb;

        bool sindex_exists = store->acquire_sindex_superblock_for_read(sindex_id,
                super_block->get_sindex_block_id(), &token_pair,
                txn.get(), &sindex_sb,
                static_cast<std::vector<char>*>(NULL), &dummy_interruptor);
        ASSERT_TRUE(sindex_exists);

        rdb_protocol_t::rget_read_response_t res;
        rdb_rget_slice(store->get_sindex_slice(sindex_id),
               rdb_protocol_t::sindex_key_range(store_key_t(cJSON_print_primary(scoped_cJSON_t(cJSON_CreateNumber(i * i)).get(), backtrace_t())),
                                                store_key_t(cJSON_print_primary(scoped_cJSON_t(cJSON_CreateNumber(i * i)).get(), backtrace_t()))),
               txn.get(), sindex_sb.get(), NULL, rdb_protocol_details::transform_t(),
               boost::optional<rdb_protocol_details::terminal_t>(), &res);

        rdb_protocol_t::rget_read_response_t::stream_t *stream = boost::get<rdb_protocol_t::rget_read_response_t::stream_t>(&res.result);
        ASSERT_TRUE(stream != NULL);
        ASSERT_EQ(0ul, stream->size());
    }
}

void run_sindex_post_construction() {
    recreate_temporary_directory(base_path_t("."));
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

    cond_t dummy_interruptor;

    insert_rows(0, (TOTAL_KEYS_TO_INSERT * 9) / 10, &store);

    std::string sindex_id = create_sindex(&store);

    cond_t background_inserts_done;
    spawn_writes_and_bring_sindexes_up_to_date(&store, sindex_id,
            &background_inserts_done);
    background_inserts_done.wait();

    check_keys_are_present(&store, sindex_id);
}

TEST(RDBBtree, SindexPostConstruct) {
    run_in_thread_pool(&run_sindex_post_construction);
}

void run_erase_range_test() {
    recreate_temporary_directory(base_path_t("."));
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

    cond_t dummy_interruptor;

    insert_rows(0, (TOTAL_KEYS_TO_INSERT * 9) / 10, &store);

    std::string sindex_id = create_sindex(&store);

    cond_t background_inserts_done;
    spawn_writes_and_bring_sindexes_up_to_date(&store, sindex_id,
            &background_inserts_done);
    background_inserts_done.wait();

    check_keys_are_present(&store, sindex_id);

    {
        /* Now we erase all of the keys we just inserted. */
        write_token_pair_t token_pair;
        store.new_write_token_pair(&token_pair);

        scoped_ptr_t<transaction_t> txn;
        scoped_ptr_t<real_superblock_t> super_block;
        store.acquire_superblock_for_write(rwi_write,
                                           repli_timestamp_t::invalid,
                                           1,
                                           WRITE_DURABILITY_SOFT,
                                           &token_pair,
                                           &txn,
                                           &super_block,
                                           &dummy_interruptor);

        const hash_region_t<key_range_t> test_range = hash_region_t<key_range_t>::universe();
        rdb_protocol_details::range_key_tester_t tester(&test_range);
        rdb_erase_range(store.btree.get(), &tester,
                key_range_t::universe(),
            txn.get(), super_block.get(), &store, &token_pair,
            &dummy_interruptor);
    }

    check_keys_are_NOT_present(&store, sindex_id);
}

TEST(RDBBtree, SindexEraseRange) {
    run_in_thread_pool(&run_erase_range_test);
}

void run_sindex_interruption_via_drop_test() {
    recreate_temporary_directory(base_path_t("."));
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

    insert_rows(0, (TOTAL_KEYS_TO_INSERT * 9) / 10, &store);

    std::string sindex_id = create_sindex(&store);

    cond_t background_inserts_done;
    spawn_writes_and_bring_sindexes_up_to_date(&store, sindex_id,
            &background_inserts_done);

    drop_sindex(&store, sindex_id);
    background_inserts_done.wait();
}

TEST(RDBBtree, SindexInterruptionViaDrop) {
    run_in_thread_pool(&run_sindex_interruption_via_drop_test);
}

void run_sindex_interruption_via_store_delete() {
    recreate_temporary_directory(base_path_t("."));
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

    scoped_ptr_t<rdb_protocol_t::store_t> store(
            new rdb_protocol_t::store_t(
            &serializer,
            "unit_test_store",
            GIGABYTE,
            true,
            &get_global_perfmon_collection(),
            NULL,
            &io_backender,
            base_path_t(".")));

    cond_t dummy_interuptor;

    insert_rows(0, (TOTAL_KEYS_TO_INSERT * 9) / 10, store.get());

    std::string sindex_id = create_sindex(store.get());

    bring_sindexes_up_to_date(store.get(), sindex_id);

    store.reset();
}

TEST(RDBBtree, SindexInterruptionViaStoreDelete) {
    run_in_thread_pool(&run_sindex_interruption_via_store_delete);
}

} //namespace unittest
