// Copyright 2010-2013 RethinkDB, all rights reserved.

#include "errors.hpp"

#include <boost/bind.hpp>

#include "unittest/gtest.hpp"

#include "arch/io/disk.hpp"
#include "btree/btree_store.hpp"
#include "buffer_cache/mirrored/config.hpp"
#include "rdb_protocol/btree.hpp"
#include "rdb_protocol/pb_utils.hpp"
#include "rdb_protocol/proto_utils.hpp"
#include "rdb_protocol/protocol.hpp"
#include "serializer/log/log_serializer.hpp"
#include "unittest/unittest_utils.hpp"

#define TOTAL_KEYS_TO_INSERT 1000

#pragma GCC diagnostic ignored "-Wshadow"

namespace unittest {

void insert_rows(int start, int finish, btree_store_t<rdb_protocol_t> *store) {
    guarantee(start <= finish);
    for (int i = start; i < finish; ++i) {
        cond_t dummy_interuptor;
        scoped_ptr_t<transaction_t> txn;
        scoped_ptr_t<real_superblock_t> superblock;
        write_token_pair_t token_pair;
        store->new_write_token_pair(&token_pair);
        store->acquire_superblock_for_write(rwi_write, repli_timestamp_t::invalid,
                                            1, WRITE_DURABILITY_SOFT,
                                            &token_pair.main_write_token, &txn, &superblock, &dummy_interuptor);
        block_id_t sindex_block_id = superblock->get_sindex_block_id();

        std::string data = strprintf("{\"id\" : %d, \"sid\" : %d}", i, i * i);
        point_write_response_t response;

        store_key_t pk(cJSON_print_primary(scoped_cJSON_t(cJSON_CreateNumber(i)).get(), backtrace_t()));
        rdb_modification_report_t mod_report(pk);
        rdb_set(pk, boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_Parse(data.c_str()))),
                false, store->btree.get(), repli_timestamp_t::invalid, txn.get(),
                superblock.get(), &response, &mod_report);

        {
            scoped_ptr_t<buf_lock_t> sindex_block;
            store->acquire_sindex_block_for_write(
                    &token_pair, txn.get(), &sindex_block,
                    sindex_block_id, &dummy_interuptor);

            btree_store_t<rdb_protocol_t>::sindex_access_vector_t sindexes;
            store->aquire_post_constructed_sindex_superblocks_for_write(
                     sindex_block.get(), txn.get(), &sindexes);
            rdb_update_sindexes(sindexes, &mod_report, txn.get());

            mutex_t::acq_t acq;
            store->lock_sindex_queue(sindex_block.get(), &acq);

            write_message_t wm;
            wm << mod_report;

            store->sindex_queue_push(wm, &acq);
        }
    }
}

void insert_rows_and_pulse_when_done(int start, int finish,
        btree_store_t<rdb_protocol_t> *store, cond_t *pulse_when_done) {
    insert_rows(start, finish, store);
    pulse_when_done->pulse();
}

void run_sindex_post_construction() {
    recreate_temporary_directory(base_path_t("."));
    temp_file_t temp_file;

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
            NULL,
            io_backender.get(),
            base_path_t("."));

    cond_t dummy_interuptor;

    insert_rows(0, (TOTAL_KEYS_TO_INSERT * 9) / 10, &store);

    uuid_u sindex_id;
    {
        sindex_id = generate_uuid();
        write_token_pair_t token_pair;
        store.new_write_token_pair(&token_pair);

        scoped_ptr_t<transaction_t> txn;
        scoped_ptr_t<real_superblock_t> super_block;

        store.acquire_superblock_for_write(rwi_write, repli_timestamp_t::invalid,
                                           1, WRITE_DURABILITY_SOFT,
                                           &token_pair.main_write_token, &txn, &super_block, &dummy_interuptor);

        Term mapping;
        Term *arg = ql::pb::set_func(&mapping, 1);
        N2(GETATTR, NVAR(1), NDATUM("sid"));

        ql::map_wire_func_t m(mapping, static_cast<std::map<int64_t, Datum> *>(NULL));

        write_message_t wm;
        wm << m;

        vector_stream_t stream;
        int res = send_write_message(&stream, &wm);
        guarantee(res == 0);

        store.add_sindex(
                &token_pair,
                sindex_id,
                stream.vector(),
                txn.get(),
                super_block.get(),
                &dummy_interuptor);
    }

    cond_t background_inserts_done;
    {
        write_token_pair_t token_pair;
        store.new_write_token_pair(&token_pair);

        scoped_ptr_t<transaction_t> txn;
        scoped_ptr_t<real_superblock_t> super_block;
        store.acquire_superblock_for_write(rwi_write, repli_timestamp_t::invalid,
                                           1, WRITE_DURABILITY_SOFT,
                                           &token_pair.main_write_token, &txn, &super_block, &dummy_interuptor);

        scoped_ptr_t<buf_lock_t> sindex_block;
        store.acquire_sindex_block_for_write(
                &token_pair, txn.get(), &sindex_block,
                super_block->get_sindex_block_id(),
                &dummy_interuptor);

        coro_t::spawn_sometime(boost::bind(&insert_rows_and_pulse_when_done, (TOTAL_KEYS_TO_INSERT * 9) / 10, TOTAL_KEYS_TO_INSERT,
                    &store, &background_inserts_done));

        std::set<uuid_u> created_sindexes;
        created_sindexes.insert(sindex_id);

        rdb_protocol_details::bring_sindexes_up_to_date(created_sindexes, &store,
                sindex_block.get());
    }

    background_inserts_done.wait();

    {
        for (int i = 0; i < TOTAL_KEYS_TO_INSERT; ++i) {
            read_token_pair_t token_pair;
            store.new_read_token_pair(&token_pair);

            scoped_ptr_t<transaction_t> txn;
            scoped_ptr_t<real_superblock_t> super_block;

            store.acquire_superblock_for_read(rwi_read,
                    &token_pair.main_read_token, &txn, &super_block,
                    &dummy_interuptor, true);

            scoped_ptr_t<real_superblock_t> sindex_sb;

            store.acquire_sindex_superblock_for_read(sindex_id,
                    super_block->get_sindex_block_id(), &token_pair,
                    txn.get(), &sindex_sb, &dummy_interuptor);

            rdb_protocol_t::rget_read_response_t res;
            rdb_rget_slice(store.get_sindex_slice(sindex_id),
                   rdb_protocol_t::sindex_key_range(store_key_t(cJSON_print_primary(scoped_cJSON_t(cJSON_CreateNumber(i * i)).get(), backtrace_t()))),
                   txn.get(), sindex_sb.get(), NULL, rdb_protocol_details::transform_t(),
                   boost::optional<rdb_protocol_details::terminal_t>(), &res);

            rdb_protocol_t::rget_read_response_t::stream_t *stream = boost::get<rdb_protocol_t::rget_read_response_t::stream_t>(&res.result);
            ASSERT_TRUE(stream != NULL);
            ASSERT_EQ(stream->size(), 1ul);

            std::string expected_data = strprintf("{\"id\" : %d, \"sid\" : %d}", i, i * i);
            scoped_cJSON_t expected_value(cJSON_Parse(expected_data.c_str()));

            ASSERT_EQ(query_language::json_cmp(expected_value.get(), stream->front().second->get()), 0);
        }
    }
}

TEST(RDBBtree, SindexPostConstruct) {
    run_in_thread_pool(&run_sindex_post_construction);
}

} //namespace unittest
