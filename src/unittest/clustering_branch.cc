// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "clustering/immediate_consistency/backfill_throttler.hpp"
#include "clustering/immediate_consistency/local_replicator.hpp"
#include "clustering/immediate_consistency/primary_dispatcher.hpp"
#include "clustering/immediate_consistency/remote_replicator_client.hpp"
#include "clustering/immediate_consistency/remote_replicator_server.hpp"
#include "containers/uuid.hpp"
#include "rdb_protocol/protocol.hpp"
#include "unittest/branch_history_manager.hpp"
#include "unittest/clustering_utils.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

namespace {

void run_with_primary(
        std::function<void(io_backender_t *,
                           simple_mailbox_cluster_t *,
                           primary_dispatcher_t *,
                           mock_store_t *,
                           local_replicator_t *,
                           order_source_t *)> fun) {
    debugf("start run_with_primary\n");
    order_source_t order_source;
    simple_mailbox_cluster_t cluster;
    io_backender_t io_backender(file_direct_io_mode_t::buffered_desired);
    cond_t interruptor;

    primary_dispatcher_t primary_dispatcher(
        &get_global_perfmon_collection(),
        region_map_t<version_t>(region_t::universe(), version_t::zero()));
    debugf("created dispatcher\n");

    mock_store_t initial_store((binary_blob_t(version_t::zero())));
    in_memory_branch_history_manager_t branch_history_manager;
    local_replicator_t local_replicator(
        cluster.get_mailbox_manager(),
        generate_uuid(),
        &primary_dispatcher,
        &initial_store,
        &branch_history_manager,
        &interruptor);
    debugf("created local replicator\n");

    fun(&io_backender,
        &cluster,
        &primary_dispatcher,
        &initial_store,
        &local_replicator,
        &order_source);
}

}   /* anonymous namespace */

/* The `ReadWrite` test just sends some reads and writes via the dispatcher to the single
local replica. */

void run_read_write_test(
        UNUSED io_backender_t *io_backender,
        UNUSED simple_mailbox_cluster_t *cluster,
        primary_dispatcher_t *dispatcher,
        UNUSED mock_store_t *store,
        UNUSED local_replicator_t *local_replicator,
        order_source_t *order_source) {
    /* Send some writes via the broadcaster to the mirror. */
    debugf("start run_read_write_test\n");
    std::map<std::string, std::string> values_inserted;
    for (int i = 0; i < 10; i++) {
        std::string key = std::string(1, 'a' + randint(26));
        values_inserted[key] = strprintf("%d", i);
        write_t w = mock_overwrite(key, strprintf("%d", i));
        simple_write_callback_t write_callback;
        dispatcher->spawn_write(
            w,
            order_source->check_in("run_read_write_test(write)"),
            &write_callback);
        debugf("waiting for write %d\n", i);
        write_callback.wait_lazily_unordered();
        debugf("done write %d\n", i);
    }

    /* Now send some reads. */
    for (std::map<std::string, std::string>::iterator it = values_inserted.begin();
            it != values_inserted.end(); it++) {
        fifo_enforcer_source_t fifo_source;
        fifo_enforcer_sink_t fifo_sink;
        fifo_enforcer_sink_t::exit_read_t exiter(&fifo_sink, fifo_source.enter_read());

        read_t r = mock_read(it->first);
        cond_t non_interruptor;
        read_response_t resp;
        debugf("waiting for read\n");
        dispatcher->read(
            r,
            &exiter,
            order_source->check_in("run_read_write_test(read)").with_read_mode(),
            &non_interruptor,
            &resp);
        debugf("done read\n");
        EXPECT_EQ(it->second, mock_parse_read_response(resp));
    }
}

TPTEST(ClusteringBranch, ReadWrite) {
    run_with_primary(&run_read_write_test);
}

/* The `Backfill` test starts up a node with one mirror, inserts some data, and
then adds another mirror. */

static void write_to_dispatcher(
        primary_dispatcher_t *dispatcher,
        const std::string& key,
        const std::string& value,
        order_token_t otok,
        signal_t *) {
    write_t w = mock_overwrite(key, value);
    simple_write_callback_t write_callback;
    dispatcher->spawn_write(w, otok, &write_callback);
    write_callback.wait_lazily_unordered();
}

void run_backfill_test(
        io_backender_t *io_backender,
        simple_mailbox_cluster_t *cluster,
        primary_dispatcher_t *dispatcher,
        mock_store_t *store1,
        local_replicator_t *local_replicator,
        order_source_t *order_source) {
    /* Start sending operations to the broadcaster */
    std::map<std::string, std::string> inserter_state;
    test_inserter_t inserter(
        std::bind(&write_to_dispatcher, dispatcher, ph::_1, ph::_2, ph::_3, ph::_4),
        std::function<std::string(const std::string &, order_token_t, signal_t *)>(),
        &dummy_key_gen,
        order_source,
        "run_backfill_test/inserter",
        &inserter_state);
    nap(100);

    remote_replicator_server_t remote_replicator_server(
        cluster->get_mailbox_manager(),
        dispatcher);

    backfill_throttler_t backfill_throttler;

    /* Set up a second mirror */
    mock_store_t store2((binary_blob_t(version_t::zero())));
    in_memory_branch_history_manager_t bhm2;
    cond_t interruptor;
    remote_replicator_client_t remote_replicator_client(
        base_path_t("."),
        io_backender,
        &backfill_throttler,
        cluster->get_mailbox_manager(),
        generate_uuid(),
        dispatcher->get_branch_id(),
        remote_replicator_server.get_bcard(),
        local_replicator->get_replica_bcard(),
        &store2,
        &bhm2,
        &get_global_perfmon_collection(),
        order_source,
        &interruptor,
        nullptr);

    nap(100);

    /* Stop the inserter, then let any lingering writes finish */
    inserter.stop();
    /* Let any lingering writes finish */
    let_stuff_happen();

    /* Confirm that both mirrors have all of the writes */
    for (std::map<std::string, std::string>::iterator it = inserter.values_inserted->begin();
            it != inserter.values_inserted->end(); it++) {
        EXPECT_EQ(it->second, mock_lookup(store1, it->first));
        EXPECT_EQ(it->second, mock_lookup(&store2, it->first));
    }
}
TPTEST(ClusteringBranch, Backfill) {
    run_with_primary(&run_backfill_test);
}

}   /* namespace unittest */
