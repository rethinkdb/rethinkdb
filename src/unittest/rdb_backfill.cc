// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "btree/backfill_debug.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/immediate_consistency/backfill_throttler.hpp"
#include "clustering/immediate_consistency/local_replicator.hpp"
#include "clustering/immediate_consistency/primary_dispatcher.hpp"
#include "clustering/immediate_consistency/remote_replicator_client.hpp"
#include "clustering/immediate_consistency/remote_replicator_server.hpp"
#include "extproc/extproc_pool.hpp"
#include "extproc/extproc_spawner.hpp"
#include "rapidjson/document.h"
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

namespace unittest {

std::string read_from_dispatcher(
        primary_dispatcher_t *dispatcher,
        size_t value_padding_length,
        const std::string &key,
        order_token_t otok,
        signal_t *interruptor) {
    read_t read(point_read_t(store_key_t(key)),
                profile_bool_t::PROFILE, read_mode_t::SINGLE);
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
        std::string value = get_result.data.get_field("value").as_str().to_std();
        backfill_debug_key(store_key_t(key), "read \"" + value + "\"");
        return value;
    } else {
        backfill_debug_key(store_key_t(key), "read empty");
        return "";
    }
}

class dispatcher_inserter_t : public test_inserter_t {
public:
    dispatcher_inserter_t(
            primary_dispatcher_t *_dispatcher,
            order_source_t *order_source,
            size_t _value_padding_length,
            std::map<std::string, std::string> *inserter_state,
            bool start = true) :
        test_inserter_t(order_source, "dispatcher_inserter_t", inserter_state, start),
        dispatcher(_dispatcher),
        value_padding_length(_value_padding_length)
        { }
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
                    DURABILITY_REQUIREMENT_SOFT,
                    profile_bool_t::PROFILE,
                    ql::configured_limits_t());
            backfill_debug_key(store_key_t(key), "write \"" + value + "\"");
        } else {
            write = write_t(
                    point_delete_t(store_key_t(key)),
                    DURABILITY_REQUIREMENT_SOFT,
                    profile_bool_t::PROFILE,
                    ql::configured_limits_t());
            backfill_debug_key(store_key_t(key), "write empty");
        }
        simple_write_callback_t write_callback;
        dispatcher->spawn_write(write, otok, &write_callback);
        wait_interruptible(&write_callback, interruptor);
    }
    std::string read(const std::string &key, order_token_t otok, signal_t *interruptor) {
        return read_from_dispatcher(
            dispatcher, value_padding_length, key, otok, interruptor);
    }
    std::string generate_key() {
        return alpha_key_gen(20);
    }
    void report_error(
            const std::string &key,
            const std::string &expect,
            const std::string &actual) {
        backfill_debug_dump_log(store_key_t(key));
        test_inserter_t::report_error(key, expect, actual);
    }
    primary_dispatcher_t *dispatcher;
    size_t value_padding_length;
};

region_map_t<version_t> get_store_version_map(store_view_t *store) {
    read_token_t token;
    store->new_read_token(&token);
    cond_t non_interruptor;
    return to_version_map(store->get_metainfo(
        order_token_t::ignore, &token, store->get_region(), &non_interruptor));
}

backfill_config_t unlimited_queues_config() {
    backfill_config_t c;
    c.item_queue_mem_size = GIGABYTE;
    c.pre_item_queue_mem_size = GIGABYTE;
    c.write_queue_count = 1000000;
    return c;
}

/* `backfill_test_config_t` is the parameters for the backfill test. Passing different
`backfill_test_config_t`s to `run_backfill_test()` will exercise different code paths in
the backfill logic. */
class backfill_test_config_t {
public:
    /* Most of the tests are minor variations on the default backfill config. */
    backfill_test_config_t() :
        value_padding_length(100),
        num_initial_writes(500),
        num_step_writes(100),
        stream_during_backfill(true)
        { }

    /* `value_padding_length` is the amount of extra padding to add to each document, in
    addition to the ~30 bytes for the key and the value. */
    size_t value_padding_length;

    /* `num_initial_writes` is the number of writes to send when filling up the first
    B-tree. */
    int num_initial_writes;

    /* `num_step_writes` is the number of writes to do between each pair of backfills. */
    int num_step_writes;

    /* `stream_during_backfill` controls whether or not we send a continuous stream of
    online writes during the backfill. */
    bool stream_during_backfill;

    /* This controls the queue sizes, etc. in the backfill logic. */
    backfill_config_t backfill;
};

void run_backfill_test(const backfill_test_config_t &cfg) {

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

    std::map<std::string, std::string> first_inserter_state;

    {
        /* Set up `store1` as the primary and start streaming writes to it */
        primary_dispatcher_t dispatcher(
            &get_global_perfmon_collection(),
            region_map_t<version_t>(region_t::universe(), version_t::zero()));

        local_replicator_t local_replicator(cluster.get_mailbox_manager(),
            generate_uuid(), &dispatcher, &store1.store, &bhm, &non_interruptor);

        dispatcher_inserter_t inserter(
            &dispatcher, &order_source, cfg.value_padding_length, &first_inserter_state,
            false);
        backfill_debug_all("begin insert store1");
        inserter.insert(cfg.num_initial_writes);
        backfill_debug_all("end insert store1");

        /* Set up `store2` and `store3` as secondaries. Backfill and then wait some time,
        but then unsubscribe. */
        {
            if (cfg.stream_during_backfill) {
                inserter.start();
            }

            remote_replicator_server_t remote_replicator_server(
                cluster.get_mailbox_manager(),
                &dispatcher);

            backfill_throttler_t backfill_throttler;
            backfill_debug_all("begin backfill store1 -> store2");
            remote_replicator_client_t remote_replicator_client_2(&backfill_throttler,
                cfg.backfill, cluster.get_mailbox_manager(), generate_uuid(),
                dispatcher.get_branch_id(), remote_replicator_server.get_bcard(),
                local_replicator.get_replica_bcard(), &store2.store, &bhm,
                &non_interruptor);
            backfill_debug_all("end backfill store1 -> store2");
            backfill_debug_all("begin backfill store1 -> store3");
            remote_replicator_client_t remote_replicator_client_3(&backfill_throttler,
                cfg.backfill, cluster.get_mailbox_manager(), generate_uuid(),
                dispatcher.get_branch_id(), remote_replicator_server.get_bcard(),
                local_replicator.get_replica_bcard(), &store3.store, &bhm,
                &non_interruptor);
            backfill_debug_all("end backfill store1 -> store3");

            if (cfg.stream_during_backfill) {
                inserter.stop();
            }
        }

        /* Keep running writes on `store1` for a bit longer, so that `store1` will be
        ahead of `store2` and `store3`. */
        backfill_debug_all("begin insert store1");
        inserter.insert(cfg.num_step_writes);
        backfill_debug_all("end insert store1");
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
            std::string res = read_from_dispatcher(&dispatcher, cfg.value_padding_length,
                pair.first, order_token_t::ignore, &non_interruptor);
            if (!res.empty()) {
                second_inserter_state[pair.first] = res;
            }
        }

        /* OK, now start performing some writes on `store2` */
        dispatcher_inserter_t inserter(
            &dispatcher, &order_source, cfg.value_padding_length, &second_inserter_state,
            false);
        backfill_debug_all("begin insert store2");
        inserter.insert(cfg.num_step_writes);
        backfill_debug_all("end insert store2");

        if (cfg.stream_during_backfill) {
            inserter.start();
        }

        /* Set up `store1` as the secondary. So this backfill will require `store1` to
        reverse the writes that were performed earlier. */
        remote_replicator_server_t remote_replicator_server(
            cluster.get_mailbox_manager(),
            &dispatcher);

        backfill_throttler_t backfill_throttler;
        backfill_debug_all("begin backfill store2 -> store1");
        remote_replicator_client_t remote_replicator_client(&backfill_throttler,
            cfg.backfill, cluster.get_mailbox_manager(), generate_uuid(),
            dispatcher.get_branch_id(), remote_replicator_server.get_bcard(),
            local_replicator.get_replica_bcard(), &store1.store, &bhm,
            &non_interruptor);
        backfill_debug_all("end backfill store2 -> store1");

        if (cfg.stream_during_backfill) {
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
            &dispatcher, &order_source, cfg.value_padding_length, &second_inserter_state,
            false);
        inserter.validate();
        inserter.validate_no_extras(first_inserter_state);

        if (cfg.stream_during_backfill) {
            inserter.start();
        }

        /* Bring back up `store3`. This backfill will test transferring data from
        `store2` to `store3` via `store1`. */
        remote_replicator_server_t remote_replicator_server(
            cluster.get_mailbox_manager(),
            &dispatcher);

        backfill_throttler_t backfill_throttler;
        backfill_debug_all("begin backfill store1 -> store3");
        remote_replicator_client_t remote_replicator_client(&backfill_throttler,
            cfg.backfill, cluster.get_mailbox_manager(), generate_uuid(),
            dispatcher.get_branch_id(), remote_replicator_server.get_bcard(),
            local_replicator.get_replica_bcard(), &store3.store, &bhm,
            &non_interruptor);
        backfill_debug_all("end backfill store1 -> store3");

        if (cfg.stream_during_backfill) {
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
            &dispatcher, &order_source, cfg.value_padding_length, &second_inserter_state,
            false);
        inserter.validate();
        inserter.validate_no_extras(first_inserter_state);
    }
}

/* This test does 3000 initial inserts and 3000 changes between every pair of backfills,
which will make it likely that many complete leaf nodes will be re-backfilled. */
TPTEST(RDBBackfill, DenseChanges) {
    backfill_test_config_t cfg;
    cfg.num_initial_writes = 3000;
    cfg.num_step_writes = 3000;
    run_backfill_test(cfg);
}

/* This test does 3000 initial writes but only 10 writes between ever pair of backfills,
so many leaf nodes will have only one or two changes. */
TPTEST(RDBBackfill, SparseChanges) {
    backfill_test_config_t cfg;
    cfg.num_initial_writes = 3000;
    cfg.num_step_writes = 10;
    /* Disable streaming during the backfill because we can't control how many writes
    will happen that way */
    cfg.stream_during_backfill = false;
    run_backfill_test(cfg);
}

TPTEST(RDBBackfill, LargeValues) {
    /* Make sure that the backfill works even with values that don't fit in the leaf
    node. */
    backfill_test_config_t cfg;
    cfg.value_padding_length = 16 * KILOBYTE;
    run_backfill_test(cfg);
}

TPTEST(RDBBackfill, VeryLargeValues) {
    /* Stress-test the system for super-large values. */
    backfill_test_config_t cfg;
    cfg.value_padding_length = 100 * MEGABYTE;
    /* Because each individual value is so expensive to backfill, don't do very many of
    them */
    cfg.num_initial_writes = 1;
    cfg.num_step_writes = 1;
    cfg.stream_during_backfill = false;
    run_backfill_test(cfg);
}

TPTEST(RDBBackfill, SmallValues) {
    /* The opposite extreme from `LargeValues`: fit as many values into each leaf node as
    possible. */
    backfill_test_config_t cfg;
    cfg.value_padding_length = 0;
    run_backfill_test(cfg);
}

TPTEST(RDBBackfill, LargeTable) {
    /* This approximates a realistic backfill scenario. So we insert a relatively large
    number of keys. */
    backfill_test_config_t cfg;
    cfg.num_initial_writes = 50000;
    cfg.num_step_writes = 1000;
    run_backfill_test(cfg);
}

TPTEST(RDBBackfill, EmptyTable) {
    /* Test the corner case where no data is actually present; we take a different code
    path if the B-tree has no root node. */
    backfill_test_config_t cfg;
    cfg.num_initial_writes = cfg.num_step_writes = 0;
    cfg.stream_during_backfill = false;
    run_backfill_test(cfg);
}

TPTEST(RDBBackfill, NearEmptyTable) {
    /* Test the corner case where almost no data is actually present. This test probably
    isn't useful, but it runs very fast, so there's little cost to having it. */
    backfill_test_config_t cfg;
    cfg.num_initial_writes = cfg.num_step_writes = 1;
    cfg.stream_during_backfill = false;
    run_backfill_test(cfg);
}

TPTEST(RDBBackfill, FillItemQueue) {
    /* Force the item queue to fill up, but make the other two queues unlimited. */
    backfill_test_config_t cfg;
    cfg.backfill = unlimited_queues_config();
    cfg.backfill.item_queue_mem_size = 1;
    cfg.backfill.item_chunk_mem_size = 1;
    /* Since the item queue will stall a lot, this backfill will be much slower than
    normal; so we backfill fewer items to speed up the test. */
    cfg.num_initial_writes = 100;
    cfg.num_step_writes = 10;
    run_backfill_test(cfg);
}

TPTEST(RDBBackfill, FillPreItemQueue) {
    /* Force the pre-item queue to fill up, but make the other two queues unlimited. */
    backfill_test_config_t cfg;
    cfg.backfill.pre_item_queue_mem_size = 1;
    cfg.backfill.pre_item_chunk_mem_size = 1;
    /* Since the pre-item queue will stall a lot, this backfill will be much slower than
    normal; so we backfill fewer items to speed up the test. */
    cfg.num_initial_writes = 100;
    cfg.num_step_writes = 100;
    run_backfill_test(cfg);
}

TPTEST(RDBBackfill, FillWriteQueue) {
    /* Force the write queue to fill up so that the backfill repeatedly switches between
    streaming and backfilling modes. */
    backfill_test_config_t cfg;
    cfg.backfill.write_queue_count = 3;
    run_backfill_test(cfg);
}

TPTEST(RDBBackfill, FillAllQueues) {
    /* Make all the queues very small, just to stress the system. */
    backfill_test_config_t cfg;
    cfg.backfill.item_queue_mem_size = 1;
    cfg.backfill.item_chunk_mem_size = 1;
    cfg.backfill.pre_item_queue_mem_size = 1;
    cfg.backfill.pre_item_chunk_mem_size = 1;
    cfg.backfill.write_queue_count = 1;
    /* Speed up the test by backfilling fewer items */
    cfg.num_initial_writes = 100;
    cfg.num_step_writes = 10;
    run_backfill_test(cfg);
}

}   /* namespace unittest */

