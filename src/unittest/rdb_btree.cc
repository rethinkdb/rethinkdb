// Copyright 2010-2014 RethinkDB, all rights reserved.
#include <functional>

#include "arch/io/disk.hpp"
#include "arch/runtime/coroutines.hpp"
#include "arch/timing.hpp"
#include "btree/operations.hpp"
#include "btree/reql_specific.hpp"
#include "buffer_cache/cache_balancer.hpp"
#include "containers/archive/boost_types.hpp"
#include "containers/archive/vector_stream.hpp"
#include "containers/uuid.hpp"
#include "rapidjson/document.h"
#include "rdb_protocol/btree.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/erase_range.hpp"
#include "rdb_protocol/minidriver.hpp"
#include "rdb_protocol/pb_utils.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/store.hpp"
#include "rdb_protocol/sym.hpp"
#include "stl_utils.hpp"
#include "serializer/config.hpp"
#include "unittest/gtest.hpp"
#include "unittest/unittest_utils.hpp"

#define TOTAL_KEYS_TO_INSERT 1000
#define MAX_RETRIES_FOR_SINDEX_POSTCONSTRUCT 50

namespace unittest {

void insert_rows(int start, int finish, store_t *store) {
    ql::configured_limits_t limits;

    guarantee(start <= finish);
    for (int i = start; i < finish; ++i) {
        cond_t dummy_interruptor;
        scoped_ptr_t<txn_t> txn;
        scoped_ptr_t<real_superblock_t> superblock;
        write_token_t token;
        store->new_write_token(&token);
        store->acquire_superblock_for_write(
            1, write_durability_t::SOFT,
            &token, &txn, &superblock, &dummy_interruptor);
        buf_lock_t sindex_block(superblock->expose_buf(),
                                superblock->get_sindex_block_id(),
                                access_t::write);

        std::string data = strprintf("{\"id\" : %d, \"sid\" : %d}", i, i * i);
        point_write_response_t response;

        store_key_t pk(ql::datum_t(static_cast<double>(i)).print_primary());
        rdb_modification_report_t mod_report(pk);
        rdb_live_deletion_context_t deletion_context;
        rapidjson::Document doc;
        doc.Parse(data.c_str());
        rdb_set(pk,
                ql::to_datum(doc, limits, reql_version_t::LATEST),
                false, store->btree.get(), repli_timestamp_t::distant_past,
                superblock.get(), &deletion_context, &response, &mod_report.info,
                static_cast<profile::trace_t *>(NULL));

        store_t::sindex_access_vector_t sindexes;
        store->acquire_post_constructed_sindex_superblocks_for_write(
                 &sindex_block,
                 &sindexes);
        rdb_update_sindexes(store,
                            sindexes,
                            &mod_report,
                            txn.get(),
                            &deletion_context,
                            NULL,
                            NULL,
                            NULL);

        new_mutex_in_line_t acq = store->get_in_line_for_sindex_queue(&sindex_block);
        store->sindex_queue_push(mod_report, &acq);
    }
}

void insert_rows_and_pulse_when_done(int start, int finish,
        store_t *store, cond_t *pulse_when_done) {
    insert_rows(start, finish, store);
    pulse_when_done->pulse();
}

sindex_name_t create_sindex(store_t *store) {
    std::string name = uuid_to_str(generate_uuid());
    ql::sym_t one(1);
    ql::protob_t<const Term> mapping = ql::r::var(one)["sid"].release_counted();
    sindex_config_t config(
        ql::map_wire_func_t(mapping, make_vector(one), ql::backtrace_id_t::empty()),
        reql_version_t::LATEST,
        sindex_multi_bool_t::SINGLE,
        sindex_geo_bool_t::REGULAR);

    cond_t non_interruptor;
    store->sindex_create(name, config, &non_interruptor);

    return sindex_name_t(name);
}

void spawn_writes(store_t *store, cond_t *background_inserts_done) {
    coro_t::spawn_sometime(std::bind(&insert_rows_and_pulse_when_done,
                (TOTAL_KEYS_TO_INSERT * 9) / 10, TOTAL_KEYS_TO_INSERT,
                store, background_inserts_done));
}

ql::grouped_t<ql::stream_t> read_row_via_sindex(
        store_t *store,
        const sindex_name_t &sindex_name,
        int sindex_value) {
    cond_t dummy_interruptor;
    read_token_t token;
    store->new_read_token(&token);

    scoped_ptr_t<txn_t> txn;
    scoped_ptr_t<real_superblock_t> super_block;

    store->acquire_superblock_for_read(
            &token, &txn, &super_block,
            &dummy_interruptor, true);

    scoped_ptr_t<sindex_superblock_t> sindex_sb;
    uuid_u sindex_uuid;

    std::vector<char> opaque_definition;
    bool sindex_exists = store->acquire_sindex_superblock_for_read(
            sindex_name,
            "",
            super_block.get(),
            &sindex_sb,
            &opaque_definition,
            &sindex_uuid);
    guarantee(sindex_exists);

    sindex_disk_info_t sindex_info;
    try {
        deserialize_sindex_info(opaque_definition, &sindex_info);
    } catch (const archive_exc_t &e) {
        crash("%s", e.what());
    }

    rget_read_response_t res;
    ql::datum_range_t datum_range(ql::datum_t(static_cast<double>(sindex_value)));
    /* The only thing this does is have a NULL `profile::trace_t *` in it which
     * prevents to profiling code from crashing. */
    ql::env_t dummy_env(&dummy_interruptor,
                        ql::return_empty_normal_batches_t::NO,
                        reql_version_t::LATEST);

    rdb_rget_secondary_slice(
        store->get_sindex_slice(sindex_uuid),
        datum_range,
        region_t(datum_range.to_sindex_keyrange(ql::skey_version_t::post_1_16)),
        sindex_sb.get(),
        &dummy_env, // env_t
        ql::batchspec_t::default_for(ql::batch_type_t::NORMAL),
        std::vector<ql::transform_variant_t>(),
        boost::optional<ql::terminal_variant_t>(),
        key_range_t::universe(),
        sorting_t::ASCENDING,
        sindex_info,
        &res,
        release_superblock_t::RELEASE);

    ql::grouped_t<ql::stream_t> *groups =
        boost::get<ql::grouped_t<ql::stream_t> >(&res.result);
    guarantee(groups != NULL);
    return *groups;
}

void _check_keys_are_present(store_t *store,
        sindex_name_t sindex_name) {
    ql::configured_limits_t limits;
    for (int i = 0; i < TOTAL_KEYS_TO_INSERT; ++i) {
        ql::grouped_t<ql::stream_t> groups =
            read_row_via_sindex(store, sindex_name, i * i);
        ASSERT_EQ(1, groups.size());
        // The order of `groups` doesn't matter because this is a small unit test.
        ql::stream_t *stream = &groups.begin()->second;
        ASSERT_TRUE(stream != NULL);
        ASSERT_EQ(1ul, stream->size());

        std::string expected_data = strprintf("{\"id\" : %d, \"sid\" : %d}", i, i * i);
        rapidjson::Document expected_value;
        expected_value.Parse(expected_data.c_str());
        ASSERT_EQ(ql::to_datum(expected_value, limits, reql_version_t::LATEST),
                  stream->front().data);
    }
}

void check_keys_are_present(store_t *store,
        sindex_name_t sindex_name) {
    for (int i = 0; i < MAX_RETRIES_FOR_SINDEX_POSTCONSTRUCT; ++i) {
        try {
            _check_keys_are_present(store, sindex_name);
            return;
        } catch (const sindex_not_ready_exc_t&) { }
        /* Unfortunately we don't have an easy way right now to tell if the
         * sindex has actually been postconstructed so we just need to
         * check by polling. */
        nap(200);
    }
    ADD_FAILURE() << "Sindex still not available after many tries.";
}

void _check_keys_are_NOT_present(store_t *store,
        sindex_name_t sindex_name) {
    /* Check that we don't have any of the keys (we just deleted them all) */
    for (int i = 0; i < TOTAL_KEYS_TO_INSERT; ++i) {
        ql::grouped_t<ql::stream_t> groups =
            read_row_via_sindex(store, sindex_name, i * i);
        if (groups.size() != 0) {
            debugf_print("groups is non-empty", groups);
        }
        ASSERT_EQ(0, groups.size());
    }
}

void check_keys_are_NOT_present(store_t *store,
        sindex_name_t sindex_name) {
    for (int i = 0; i < MAX_RETRIES_FOR_SINDEX_POSTCONSTRUCT; ++i) {
        try {
            _check_keys_are_NOT_present(store, sindex_name);
            return;
        } catch (const sindex_not_ready_exc_t&) { }
        /* Unfortunately we don't have an easy way right now to tell if the
         * sindex has actually been postconstructed so we just need to
         * check by polling. */
        nap(100);
    }
    ADD_FAILURE() << "Sindex still not available after many tries.";
}

TPTEST(RDBBtree, SindexPostConstruct) {
    recreate_temporary_directory(base_path_t("."));
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
            region_t::universe(),
            &serializer,
            &balancer,
            "unit_test_store",
            true,
            &get_global_perfmon_collection(),
            NULL,
            &io_backender,
            base_path_t("."),
            scoped_ptr_t<outdated_index_report_t>(),
            generate_uuid());

    cond_t dummy_interruptor;

    insert_rows(0, (TOTAL_KEYS_TO_INSERT * 9) / 10, &store);

    sindex_name_t sindex_name = create_sindex(&store);

    cond_t background_inserts_done;
    spawn_writes(&store, &background_inserts_done);
    background_inserts_done.wait();

    check_keys_are_present(&store, sindex_name);
}

TPTEST(RDBBtree, SindexEraseRange) {
    recreate_temporary_directory(base_path_t("."));
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
            region_t::universe(),
            &serializer,
            &balancer,
            "unit_test_store",
            true,
            &get_global_perfmon_collection(),
            NULL,
            &io_backender,
            base_path_t("."),
            scoped_ptr_t<outdated_index_report_t>(),
            generate_uuid());

    cond_t dummy_interruptor;

    insert_rows(0, (TOTAL_KEYS_TO_INSERT * 9) / 10, &store);

    sindex_name_t sindex_name = create_sindex(&store);

    cond_t background_inserts_done;
    spawn_writes(&store, &background_inserts_done);
    background_inserts_done.wait();

    check_keys_are_present(&store, sindex_name);

    {
        /* Now we erase all of the keys we just inserted. */
        write_token_t token;
        store.new_write_token(&token);

        scoped_ptr_t<txn_t> txn;
        scoped_ptr_t<real_superblock_t> super_block;
        store.acquire_superblock_for_write(1,
                                           write_durability_t::SOFT,
                                           &token,
                                           &txn,
                                           &super_block,
                                           &dummy_interruptor);

        const hash_region_t<key_range_t> test_range = hash_region_t<key_range_t>::universe();
        rdb_protocol::range_key_tester_t tester(&test_range);
        buf_lock_t sindex_block(super_block->expose_buf(),
                                super_block->get_sindex_block_id(),
                                access_t::write);

        rdb_live_deletion_context_t deletion_context;
        std::vector<rdb_modification_report_t> mod_reports;
        key_range_t deleted_range;
        rdb_erase_small_range(store.btree.get(),
                              &tester,
                              key_range_t::universe(),
                              super_block.get(),
                              &deletion_context,
                              &dummy_interruptor,
                              0,
                              &mod_reports,
                              &deleted_range);

        store.update_sindexes(txn.get(), &sindex_block, mod_reports, true);
    }

    check_keys_are_NOT_present(&store, sindex_name);
}

TPTEST(RDBBtree, SindexInterruptionViaDrop) {
    recreate_temporary_directory(base_path_t("."));
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
            region_t::universe(),
            &serializer,
            &balancer,
            "unit_test_store",
            true,
            &get_global_perfmon_collection(),
            NULL,
            &io_backender,
            base_path_t("."),
            scoped_ptr_t<outdated_index_report_t>(),
            generate_uuid());

    cond_t dummy_interruptor;

    insert_rows(0, (TOTAL_KEYS_TO_INSERT * 9) / 10, &store);

    sindex_name_t sindex_name = create_sindex(&store);

    cond_t background_inserts_done;
    spawn_writes(&store, &background_inserts_done);

    store.sindex_drop(sindex_name.name, &dummy_interruptor);
    background_inserts_done.wait();
}

TPTEST(RDBBtree, SindexInterruptionViaStoreDelete) {
    recreate_temporary_directory(base_path_t("."));
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

    scoped_ptr_t<store_t> store(new store_t(
            region_t::universe(),
            &serializer,
            &balancer,
            "unit_test_store",
            true,
            &get_global_perfmon_collection(),
            NULL,
            &io_backender,
            base_path_t("."),
            scoped_ptr_t<outdated_index_report_t>(),
            generate_uuid()));

    insert_rows(0, (TOTAL_KEYS_TO_INSERT * 9) / 10, store.get());

    create_sindex(store.get());
    nap(1000);
    store.reset();
}

} //namespace unittest
