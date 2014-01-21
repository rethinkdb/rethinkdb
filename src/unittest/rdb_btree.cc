// Copyright 2010-2014 RethinkDB, all rights reserved.
#include <functional>

#include "arch/io/disk.hpp"
#include "arch/runtime/coroutines.hpp"
#include "arch/timing.hpp"
#include "btree/btree_store.hpp"
#include "btree/operations.hpp"
#include "buffer_cache/mirrored/config.hpp"
#include "containers/archive/boost_types.hpp"
#include "rdb_protocol/btree.hpp"
#include "rdb_protocol/pb_utils.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/sym.hpp"
#include "serializer/config.hpp"
#include "unittest/gtest.hpp"
#include "unittest/unittest_utils.hpp"
#include "rdb_protocol/minidriver.hpp"

#define TOTAL_KEYS_TO_INSERT 1000
#define MAX_RETRIES_FOR_SINDEX_POSTCONSTRUCT 5

#pragma GCC diagnostic ignored "-Wshadow"

using namespace alt;  // RSI

namespace unittest {

void insert_rows(int start, int finish, btree_store_t<rdb_protocol_t> *store) {
    guarantee(start <= finish);
    for (int i = start; i < finish; ++i) {
        cond_t dummy_interruptor;
        scoped_ptr_t<alt_txn_t> txn;
        scoped_ptr_t<real_superblock_t> superblock;
        write_token_pair_t token_pair;
        store->new_write_token_pair(&token_pair);
        store->acquire_superblock_for_write(
            repli_timestamp_t::invalid,
            1, write_durability_t::SOFT,
            &token_pair, &txn, &superblock, &dummy_interruptor);
        block_id_t sindex_block_id = superblock->get_sindex_block_id();

        std::string data = strprintf("{\"id\" : %d, \"sid\" : %d}", i, i * i);
        point_write_response_t response;

        store_key_t pk(make_counted<const ql::datum_t>(static_cast<double>(i))->print_primary());
        rdb_modification_report_t mod_report(pk);
        rdb_set(pk,
                make_counted<ql::datum_t>(scoped_cJSON_t(cJSON_Parse(data.c_str()))),
                false, store->btree.get(), repli_timestamp_t::invalid,
                superblock.get(), &response, &mod_report.info,
                static_cast<profile::trace_t *>(NULL));

        {
            scoped_ptr_t<alt::alt_buf_lock_t> sindex_block;

            store->acquire_sindex_block_for_write(
                    superblock->expose_buf(), &sindex_block, sindex_block_id);
            btree_store_t<rdb_protocol_t>::sindex_access_vector_t sindexes;
            store->acquire_post_constructed_sindex_superblocks_for_write(
                     sindex_block.get(),
                     &sindexes);
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

    scoped_ptr_t<alt_txn_t> txn;
    scoped_ptr_t<real_superblock_t> super_block;

    store->acquire_superblock_for_write(repli_timestamp_t::invalid,
                                        1, write_durability_t::SOFT,
                                        &token_pair, &txn, &super_block,
                                        &dummy_interruptor);

    ql::sym_t one(1);
    ql::protob_t<const Term> mapping = ql::r::var(one)["sid"].release_counted();
    ql::map_wire_func_t m(mapping, make_vector(one), get_backtrace(mapping));

    sindex_multi_bool_t multi_bool = sindex_multi_bool_t::SINGLE;

    write_message_t wm;
    wm << m;
    wm << multi_bool;

    vector_stream_t stream;
    int res = send_write_message(&stream, &wm);
    guarantee(res == 0);

    scoped_ptr_t<alt_buf_lock_t> sindex_block;
    store->acquire_sindex_block_for_write(super_block->expose_buf(),
                                          &sindex_block,
                                          super_block->get_sindex_block_id());
    UNUSED bool b = store->add_sindex(
            sindex_id,
            stream.vector(),
            sindex_block.get());
    return sindex_id;
}

void drop_sindex(btree_store_t<rdb_protocol_t> *store,
                 const std::string &sindex_id) {
    cond_t dummy_interuptor;
    write_token_pair_t token_pair;
    store->new_write_token_pair(&token_pair);

    scoped_ptr_t<alt_txn_t> txn;
    scoped_ptr_t<real_superblock_t> super_block;

    store->acquire_superblock_for_write(repli_timestamp_t::invalid,
                                        1, write_durability_t::SOFT, &token_pair,
                                        &txn, &super_block, &dummy_interuptor);

    value_sizer_t<rdb_value_t> sizer(store->cache->get_block_size());
    rdb_value_deleter_t deleter;

    scoped_ptr_t<alt_buf_lock_t> sindex_block;
    store->acquire_sindex_block_for_write(super_block->expose_buf(),
                                          &sindex_block,
                                          super_block->get_sindex_block_id());
    store->drop_sindex(
            sindex_id,
            sindex_block.get(),
            &sizer,
            &deleter,
            &dummy_interuptor);
}

void bring_sindexes_up_to_date(
        btree_store_t<rdb_protocol_t> *store, std::string sindex_id) {
    cond_t dummy_interruptor;
    write_token_pair_t token_pair;
    store->new_write_token_pair(&token_pair);

    scoped_ptr_t<alt_txn_t> txn;
    scoped_ptr_t<real_superblock_t> super_block;
    store->acquire_superblock_for_write(repli_timestamp_t::invalid,
                                        1, write_durability_t::SOFT,
                                        &token_pair, &txn, &super_block, &dummy_interruptor);

    scoped_ptr_t<alt_buf_lock_t> sindex_block;
    store->acquire_sindex_block_for_write(
            super_block->expose_buf(),
            &sindex_block,
            super_block->get_sindex_block_id());

    std::set<std::string> created_sindexes;
    created_sindexes.insert(sindex_id);

    rdb_protocol_details::bring_sindexes_up_to_date(created_sindexes, store,
                                                    sindex_block.get());
    nap(1000);
}

void spawn_writes_and_bring_sindexes_up_to_date(btree_store_t<rdb_protocol_t> *store,
        std::string sindex_id, cond_t *background_inserts_done) {
    cond_t dummy_interruptor;
    write_token_pair_t token_pair;
    store->new_write_token_pair(&token_pair);

    scoped_ptr_t<alt_txn_t> txn;
    scoped_ptr_t<real_superblock_t> super_block;
    store->acquire_superblock_for_write(
        repli_timestamp_t::invalid,
        1, write_durability_t::SOFT,
        &token_pair, &txn, &super_block, &dummy_interruptor);

    scoped_ptr_t<alt_buf_lock_t> sindex_block;
    store->acquire_sindex_block_for_write(
            super_block->expose_buf(),
            &sindex_block,
            super_block->get_sindex_block_id());

    coro_t::spawn_sometime(std::bind(&insert_rows_and_pulse_when_done,
                (TOTAL_KEYS_TO_INSERT * 9) / 10, TOTAL_KEYS_TO_INSERT,
                store, background_inserts_done));

    std::set<std::string> created_sindexes;
    created_sindexes.insert(sindex_id);

    rdb_protocol_details::bring_sindexes_up_to_date(created_sindexes, store,
                                                    sindex_block.get());
}

void _check_keys_are_present(btree_store_t<rdb_protocol_t> *store,
        std::string sindex_id) {
    cond_t dummy_interruptor;
    for (int i = 0; i < TOTAL_KEYS_TO_INSERT; ++i) {
        read_token_pair_t token_pair;
        store->new_read_token_pair(&token_pair);

        scoped_ptr_t<alt_txn_t> txn;
        scoped_ptr_t<real_superblock_t> super_block;

        store->acquire_superblock_for_read(
                &token_pair.main_read_token, &txn, &super_block,
                &dummy_interruptor, true);

        scoped_ptr_t<real_superblock_t> sindex_sb;

        bool sindex_exists = store->acquire_sindex_superblock_for_read(
                sindex_id,
                super_block->get_sindex_block_id(),
                super_block->expose_buf(),
                &sindex_sb,
                static_cast<std::vector<char>*>(NULL));
        ASSERT_TRUE(sindex_exists);

        rdb_protocol_t::rget_read_response_t res;
        double ii = i * i;
        /* The only thing this does is have a NULL scoped_ptr_t<trace_t> in it
         * which prevents to profiling code from crashing. */
        ql::env_t dummy_env(NULL);
        rdb_rget_slice(
            store->get_sindex_slice(sindex_id),
            rdb_protocol_t::sindex_key_range(
                store_key_t(make_counted<const ql::datum_t>(ii)->print_primary()),
                store_key_t(make_counted<const ql::datum_t>(ii)->print_primary())),
            sindex_sb.get(),
            &dummy_env, // env_t
            ql::batchspec_t::user(ql::batch_type_t::NORMAL,
                                  counted_t<const ql::datum_t>()),
            rdb_protocol_details::transform_t(),
            boost::optional<rdb_protocol_details::terminal_t>(),
            sorting_t::ASCENDING,
            &res);

        rdb_protocol_t::rget_read_response_t::stream_t *stream
            = boost::get<rdb_protocol_t::rget_read_response_t::stream_t>(&res.result);
        ASSERT_TRUE(stream != NULL);
        ASSERT_EQ(1ul, stream->size());

        std::string expected_data = strprintf("{\"id\" : %d, \"sid\" : %d}", i, i * i);
        scoped_cJSON_t expected_value(cJSON_Parse(expected_data.c_str()));
        ASSERT_EQ(ql::datum_t(expected_value.get()), *stream->front().data);
    }
}

void check_keys_are_present(btree_store_t<rdb_protocol_t> *store,
        std::string sindex_id) {
    for (int i = 0; i < MAX_RETRIES_FOR_SINDEX_POSTCONSTRUCT; ++i) {
        try {
            _check_keys_are_present(store, sindex_id);
        } catch (const sindex_not_post_constructed_exc_t&) { }
        /* Unfortunately we don't have an easy way right now to tell if the
         * sindex has actually been postconstructed so we just need to
         * check by polling. */
        nap(100);
    }
}

void _check_keys_are_NOT_present(btree_store_t<rdb_protocol_t> *store,
        std::string sindex_id) {
    /* Check that we don't have any of the keys (we just deleted them all) */
    cond_t dummy_interruptor;
    for (int i = 0; i < TOTAL_KEYS_TO_INSERT; ++i) {
        read_token_pair_t token_pair;
        store->new_read_token_pair(&token_pair);

        scoped_ptr_t<alt_txn_t> txn;
        scoped_ptr_t<real_superblock_t> super_block;

        store->acquire_superblock_for_read(
                &token_pair.main_read_token, &txn, &super_block,
                &dummy_interruptor, true);

        scoped_ptr_t<real_superblock_t> sindex_sb;

        bool sindex_exists = store->acquire_sindex_superblock_for_read(
                sindex_id,
                super_block->get_sindex_block_id(),
                super_block->expose_buf(),
                &sindex_sb,
                static_cast<std::vector<char>*>(NULL));
        ASSERT_TRUE(sindex_exists);

        rdb_protocol_t::rget_read_response_t res;
        double ii = i * i;
        /* The only thing this does is have a NULL scoped_ptr_t<trace_t> in it
         * which prevents to profiling code from crashing. */
        ql::env_t dummy_env(NULL);
        rdb_rget_slice(
            store->get_sindex_slice(sindex_id),
            rdb_protocol_t::sindex_key_range(
                store_key_t(make_counted<const ql::datum_t>(ii)->print_primary()),
                store_key_t(make_counted<const ql::datum_t>(ii)->print_primary())),
            sindex_sb.get(),
            &dummy_env, // env_t
            ql::batchspec_t::user(ql::batch_type_t::NORMAL,
                                  counted_t<const ql::datum_t>()),
            rdb_protocol_details::transform_t(),
            boost::optional<rdb_protocol_details::terminal_t>(),
            sorting_t::ASCENDING,
            &res);

        rdb_protocol_t::rget_read_response_t::stream_t *stream
            = boost::get<rdb_protocol_t::rget_read_response_t::stream_t>(&res.result);
        ASSERT_TRUE(stream != NULL);
        ASSERT_EQ(0ul, stream->size());
    }
}

void check_keys_are_NOT_present(btree_store_t<rdb_protocol_t> *store,
        std::string sindex_id) {
    for (int i = 0; i < MAX_RETRIES_FOR_SINDEX_POSTCONSTRUCT; ++i) {
        try {
            _check_keys_are_NOT_present(store, sindex_id);
        } catch (const sindex_not_post_constructed_exc_t&) { }
        /* Unfortunately we don't have an easy way right now to tell if the
         * sindex has actually been postconstructed so we just need to
         * check by polling. */
        nap(100);
    }
}

void run_sindex_post_construction() {
    recreate_temporary_directory(base_path_t("."));
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

        scoped_ptr_t<alt_txn_t> txn;
        scoped_ptr_t<real_superblock_t> super_block;
        store.acquire_superblock_for_write(repli_timestamp_t::invalid,
                                           1,
                                           write_durability_t::SOFT,
                                           &token_pair,
                                           &txn,
                                           &super_block,
                                           &dummy_interruptor);

        const hash_region_t<key_range_t> test_range = hash_region_t<key_range_t>::universe();
        rdb_protocol_details::range_key_tester_t tester(&test_range);
        scoped_ptr_t<alt_buf_lock_t> sindex_block;
        store.acquire_sindex_block_for_write(
            super_block->expose_buf(),
            &sindex_block,
            super_block->get_sindex_block_id());
        rdb_erase_range(store.btree.get(), &tester,
                        key_range_t::universe(),
                        sindex_block.get(),
                        super_block.get(), &store,
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

    io_backender_t io_backender(file_direct_io_mode_t::buffered_desired);

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
