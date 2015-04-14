// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "clustering/administration/metadata.hpp"
#include "clustering/immediate_consistency/backfill_throttler.hpp"
#include "clustering/immediate_consistency/local_replicator.hpp"
#include "clustering/immediate_consistency/primary_dispatcher.hpp"
#include "clustering/immediate_consistency/remote_replicator_client.hpp"
#include "clustering/immediate_consistency/remote_replicator_server.hpp"
#include "extproc/extproc_pool.hpp"
#include "extproc/extproc_spawner.hpp"
#include "rdb_protocol/minidriver.hpp"
#include "rdb_protocol/pb_utils.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/store.hpp"
#include "rdb_protocol/sym.hpp"
#include "rpc/directory/read_manager.hpp"
#include "rpc/semilattice/semilattice_manager.hpp"
#include "stl_utils.hpp"
#include "unittest/branch_history_manager.hpp"
#include "unittest/clustering_utils.hpp"
#include "unittest/dummy_metadata_controller.hpp"
#include "unittest/unittest_utils.hpp"

#include "kh_debug.hpp"

namespace unittest {

std::string read_from_dispatcher(
        primary_dispatcher_t *dispatcher,
        size_t value_padding_length,
        const std::string &key,
        order_token_t otok,
        signal_t *interruptor) {
    read_t read(point_read_t(store_key_t(key)), profile_bool_t::PROFILE);
    fifo_enforcer_source_t fifo_source;
    fifo_enforcer_sink_t fifo_sink;
    fifo_enforcer_sink_t::exit_read_t exiter(&fifo_sink, fifo_source.enter_read());
    read_response_t response;
    dispatcher->read(read, &exiter, otok, interruptor, &response);
    point_read_response_t get_result =
        boost::get<point_read_response_t>(response.response);
    if (get_result.data.get_type() != ql::datum_t::R_NULL) {
        EXPECT_EQ(3, get_result.data.obj_size());
        EXPECT_EQ(key, get_result.data.get_field("id").as_str().to_std());
        EXPECT_EQ(std::string(value_padding_length, 'a'),
            get_result.data.get_field("padding").as_str().to_std());
        return get_result.data.get_field("value").as_str().to_std();
    } else {
        return "";
    }
}

class dispatcher_inserter_t {
public:
    dispatcher_inserter_t(
            primary_dispatcher_t *_dispatcher,
            order_source_t *order_source,
            size_t _value_padding_length,
            std::map<std::string, std::string> *inserter_state,
            bool start = true) :
        dispatcher(_dispatcher),
        value_padding_length(_value_padding_length),
        inner(
            std::bind(&dispatcher_inserter_t::write, this,
                ph::_1, ph::_2, ph::_3, ph::_4),
            std::bind(&dispatcher_inserter_t::read, this, ph::_1, ph::_2, ph::_3),
            []() { return alpha_key_gen(20); },
            order_source,
            "dispatcher_inserter_t",
            inserter_state,
            start)
        { }
    void insert(size_t n) { inner.insert(n); }
    void start() { inner.start(); }
    void stop() { inner.stop(); }
    void validate() { inner.validate(); }
    void validate_no_extras(const std::map<std::string, std::string> &extras) {
        inner.validate_no_extras(extras);
    }
private:
    void write(const std::string &key, const std::string &value,
            order_token_t otok, signal_t *interruptor) {
        write_t write;
        if (!value.empty()) {
            ql::datum_object_builder_t doc;
            doc.overwrite("id", ql::datum_t(datum_string_t(key)));
            doc.overwrite("value", ql::datum_t(datum_string_t(value)));
            doc.overwrite("padding", ql::datum_t(datum_string_t(
                std::string(value_padding_length, 'a'))));
            write = write_t(
                    point_write_t(store_key_t(key), std::move(doc).to_datum(), true),
                    DURABILITY_REQUIREMENT_DEFAULT,
                    profile_bool_t::PROFILE,
                    ql::configured_limits_t());
        } else {
            write = write_t(
                    point_delete_t(store_key_t(key)),
                    DURABILITY_REQUIREMENT_DEFAULT,
                    profile_bool_t::PROFILE,
                    ql::configured_limits_t());
        }
        simple_write_callback_t write_callback;
        dispatcher->spawn_write(write, otok, &write_callback);
        wait_interruptible(&write_callback, interruptor);
    }
    std::string read(const std::string &key, order_token_t otok, signal_t *interruptor) {
        return read_from_dispatcher(
            dispatcher, value_padding_length, key, otok, interruptor);
    }
    primary_dispatcher_t *dispatcher;
    size_t value_padding_length;
    test_inserter_t inner;
};

region_map_t<version_t> get_store_version_map(store_view_t *store) {
    region_map_t<binary_blob_t> binary;
    read_token_t token;
    store->new_read_token(&token);
    cond_t non_interruptor;
    store->do_get_metainfo(order_token_t::ignore, &token, &non_interruptor, &binary);
    return to_version_map(binary);
}

void run_backfill_test(
        size_t value_padding_length,
        int num_initial,
        int num_step,
        bool stream_during_backfill,
        const backfill_config_t &backfill_config) {

    khd_all("begin backfill test");

    order_source_t order_source;
    simple_mailbox_cluster_t cluster;
    io_backender_t io_backender(file_direct_io_mode_t::buffered_desired);
    extproc_pool_t extproc_pool(2);
    rdb_context_t ctx(&extproc_pool, NULL);
    cond_t non_interruptor;

    in_memory_branch_history_manager_t bhm;
    test_store_t store1(&io_backender, &order_source, &ctx);
    test_store_t store2(&io_backender, &order_source, &ctx);
    test_store_t store3(&io_backender, &order_source, &ctx);

    khd_all("store1", &store1);
    khd_all("store2", &store2);
    khd_all("store3", &store3);

    std::map<std::string, std::string> first_inserter_state;

    {
        /* Set up `store1` as the primary and start streaming writes to it */
        primary_dispatcher_t dispatcher(
            &get_global_perfmon_collection(),
            region_map_t<version_t>(region_t::universe(), version_t::zero()));

        local_replicator_t local_replicator(cluster.get_mailbox_manager(),
            generate_uuid(), &dispatcher, &store1.store, &bhm, &non_interruptor);

        dispatcher_inserter_t inserter(
            &dispatcher, &order_source, value_padding_length, &first_inserter_state,
            false);
        khd_all("begin insert 1");
        inserter.insert(num_initial);
        khd_all("end insert 1");

        /* Set up `store2` and `store3` as secondaries. Backfill and then wait some time,
        but then unsubscribe. */
        {
            if (stream_during_backfill) {
                inserter.start();
            }

            remote_replicator_server_t remote_replicator_server(
                cluster.get_mailbox_manager(),
                &dispatcher);

            backfill_throttler_t backfill_throttler;
            khd_all("begin backfill 1 -> 2");
            remote_replicator_client_t remote_replicator_client_2(&backfill_throttler,
                backfill_config, cluster.get_mailbox_manager(), generate_uuid(),
                dispatcher.get_branch_id(), remote_replicator_server.get_bcard(),
                local_replicator.get_replica_bcard(), &store2.store, &bhm,
                &non_interruptor);
            khd_all("end backfill 1 -> 2");
            khd_all("begin backfill 1 -> 3");
            remote_replicator_client_t remote_replicator_client_3(&backfill_throttler,
                backfill_config, cluster.get_mailbox_manager(), generate_uuid(),
                dispatcher.get_branch_id(), remote_replicator_server.get_bcard(),
                local_replicator.get_replica_bcard(), &store3.store, &bhm,
                &non_interruptor);
            khd_all("end backfill 1 -> 3");

            if (stream_during_backfill) {
                inserter.stop();
            }
        }

        /* Keep running writes on `store1` for a bit longer, so that `store1` will be
        ahead of `store2` and `store3`. */
        khd_all("begin insert 1");
        inserter.insert(num_step);
        khd_all("end insert 1");
    }

    std::map<std::string, std::string> second_inserter_state;

    {
        /* Now set up `store2` as the primary */
        primary_dispatcher_t dispatcher(
            &get_global_perfmon_collection(),
            get_store_version_map(&store2.store));

        local_replicator_t local_replicator(cluster.get_mailbox_manager(),
            generate_uuid(), &dispatcher, &store2.store, &bhm, &non_interruptor);

        /* Find the subset of `first_inserter_state` that's actually present in `store2`
        */
        for (const auto &pair : first_inserter_state) {
            std::string res = read_from_dispatcher(&dispatcher, value_padding_length,
                pair.first, order_token_t::ignore, &non_interruptor);
            if (!res.empty()) {
                second_inserter_state[pair.first] = res;
            }
        }

        /* OK, now start performing some writes on `store2` */
        dispatcher_inserter_t inserter(
            &dispatcher, &order_source, value_padding_length, &second_inserter_state,
            false);
        khd_all("begin insert 2");
        inserter.insert(num_step);
        khd_all("end insert 2");

        if (stream_during_backfill) {
            inserter.start();
        }

        /* Set up `store1` as the secondary. So this backfill will require `store1` to
        reverse the writes that were performed earlier. */
        remote_replicator_server_t remote_replicator_server(
            cluster.get_mailbox_manager(),
            &dispatcher);

        khd_all("begin backfill 2 -> 1");
        backfill_throttler_t backfill_throttler;
        remote_replicator_client_t remote_replicator_client(&backfill_throttler,
            backfill_config, cluster.get_mailbox_manager(), generate_uuid(),
            dispatcher.get_branch_id(), remote_replicator_server.get_bcard(),
            local_replicator.get_replica_bcard(), &store1.store, &bhm,
            &non_interruptor);
        khd_all("end backfill 2 -> 1");

        if (stream_during_backfill) {
            inserter.stop();
        }
        inserter.validate();
    }

    {
        /* Set up `store1` as the primary again */
        primary_dispatcher_t dispatcher(
            &get_global_perfmon_collection(),
            get_store_version_map(&store1.store));

        local_replicator_t local_replicator(cluster.get_mailbox_manager(),
            generate_uuid(), &dispatcher, &store1.store, &bhm, &non_interruptor);

        /* Validate the state of `store1` to make sure
        that the backfill was completely correct */
        dispatcher_inserter_t inserter(
            &dispatcher, &order_source, value_padding_length, &second_inserter_state,
            false);
        inserter.validate();
        inserter.validate_no_extras(first_inserter_state);

        if (stream_during_backfill) {
            inserter.start();
        }

        /* Bring back up `store3`. This backfill will test transferring data from
        `store2` to `store3` via `store1`. */
        remote_replicator_server_t remote_replicator_server(
            cluster.get_mailbox_manager(),
            &dispatcher);

        khd_all("begin backfill 1 -> 3");
        backfill_throttler_t backfill_throttler;
        remote_replicator_client_t remote_replicator_client(&backfill_throttler,
            backfill_config, cluster.get_mailbox_manager(), generate_uuid(),
            dispatcher.get_branch_id(), remote_replicator_server.get_bcard(),
            local_replicator.get_replica_bcard(), &store3.store, &bhm,
            &non_interruptor);
        khd_all("end backfill 1 -> 3");

        if (stream_during_backfill) {
            inserter.stop();
        }
    }

    {
        /* Finally, set up `store3` as primary so that we can test its state */
        primary_dispatcher_t dispatcher(
            &get_global_perfmon_collection(),
            get_store_version_map(&store3.store));

        local_replicator_t local_replicator(cluster.get_mailbox_manager(),
            generate_uuid(), &dispatcher, &store3.store, &bhm, &non_interruptor);

        /* Validate the state of `store3` to make sure that the backfill was completely
        correct */
        dispatcher_inserter_t inserter(
            &dispatcher, &order_source, value_padding_length, &second_inserter_state,
            false);
        inserter.validate();
        inserter.validate_no_extras(first_inserter_state);
    }
}

backfill_config_t unlimited_queues_config() {
    backfill_config_t c;
    c.item_queue_mem_size = GIGABYTE;
    c.pre_item_queue_mem_size = GIGABYTE;
    c.write_queue_count = 1000000;
    return c;
}

TPTEST(RDBBackfill, DenseChangesAndStreaming) {
    run_backfill_test(100, 3000, 1000, true, unlimited_queues_config());
}

TPTEST(RDBBackfill, SparseChanges) {
    run_backfill_test(100, 1000, 10, false, unlimited_queues_config());
}

TPTEST(RDBBackfill, LargeValues) {
    run_backfill_test(MEGABYTE, 100, 10, true, unlimited_queues_config());
}

TPTEST(RDBBackfill, SmallValues) {
    run_backfill_test(1, 100, 10, true, unlimited_queues_config());
}

TPTEST(RDBBackfill, LargeTable) {
    run_backfill_test(100, 20000, 1000, true, backfill_config_t());
}

TPTEST(RDBBackfill, VeryLargeTable) {
    run_backfill_test(100, 100000, 1000, true, backfill_config_t());
}

TPTEST(RDBBackfill, EmptyTable) {
    run_backfill_test(100, 0, 0, false, unlimited_queues_config());
}

TPTEST(RDBBackfill, NearEmptyTable) {
    run_backfill_test(100, 1, 1, true, unlimited_queues_config());
}

TPTEST(RDBBackfill, FillItemQueue) {
    backfill_config_t c = unlimited_queues_config();
    c.item_queue_mem_size = 1;
    c.item_chunk_mem_size = 1;
    run_backfill_test(100, 3000, 1000, true, c);
}

TPTEST(RDBBackfill, FillPreItemQueue) {
    backfill_config_t c = unlimited_queues_config();
    c.pre_item_queue_mem_size = 1;
    c.pre_item_chunk_mem_size = 1;
    run_backfill_test(100, 3000, 1000, true, c);
}

TPTEST(RDBBackfill, FillWriteQueue) {
    backfill_config_t c = unlimited_queues_config();
    c.write_queue_count = 3;
    run_backfill_test(100, 3000, 1000, true, c);
}

TPTEST(RDBBackfill, FillAllQueues) {
    backfill_config_t c;
    c.item_queue_mem_size = 1;
    c.item_chunk_mem_size = 1;
    c.pre_item_queue_mem_size = 1;
    c.pre_item_chunk_mem_size = 1;
    c.write_queue_count = 1;
    run_backfill_test(1, 100, 10, true, c);
}

}   /* namespace unittest */

