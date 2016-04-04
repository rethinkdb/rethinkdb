// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "clustering/immediate_consistency/local_replicator.hpp"
#include "clustering/immediate_consistency/primary_dispatcher.hpp"
#include "clustering/immediate_consistency/remote_replicator_client.hpp"
#include "clustering/immediate_consistency/remote_replicator_server.hpp"
#include "clustering/immediate_consistency/standard_backfill_throttler.hpp"
#include "clustering/table_manager/backfill_progress_tracker.hpp"
#include "containers/uuid.hpp"
#include "rdb_protocol/protocol.hpp"
#include "unittest/branch_history_manager.hpp"
#include "unittest/clustering_utils.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

namespace {

void run_with_primary(
        std::function<void(simple_mailbox_cluster_t *,
                           primary_dispatcher_t *,
                           mock_store_t *,
                           local_replicator_t *,
                           order_source_t *)> fun) {
    order_source_t order_source;
    simple_mailbox_cluster_t cluster;
    cond_t interruptor;

    primary_dispatcher_t primary_dispatcher(
        &get_global_perfmon_collection(),
        region_map_t<version_t>(region_t::universe(), version_t::zero()));

    mock_store_t initial_store((binary_blob_t(version_t::zero())));
    in_memory_branch_history_manager_t branch_history_manager;
    local_replicator_t local_replicator(
        cluster.get_mailbox_manager(),
        server_id_t::generate_server_id(),
        &primary_dispatcher,
        &initial_store,
        &branch_history_manager,
        &interruptor);

    fun(&cluster,
        &primary_dispatcher,
        &initial_store,
        &local_replicator,
        &order_source);
}

}   /* anonymous namespace */

/* The `ReadWrite` test just sends some reads and writes via the dispatcher to the single
local replica. */

void run_read_write_test(
        UNUSED simple_mailbox_cluster_t *cluster,
        primary_dispatcher_t *dispatcher,
        UNUSED mock_store_t *store,
        UNUSED local_replicator_t *local_replicator,
        order_source_t *order_source) {
    /* Send some writes via the broadcaster to the mirror. */
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
        write_callback.wait_lazily_unordered();
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
        dispatcher->read(
            r,
            &exiter,
            order_source->check_in("run_read_write_test(read)").with_read_mode(),
            &non_interruptor,
            &resp);
        EXPECT_EQ(it->second, mock_parse_read_response(resp));
    }
}

TPTEST(ClusteringBranch, ReadWrite) {
    run_with_primary(&run_read_write_test);
}

/* The `Backfill` test starts up a node with one mirror, inserts some data, and
then adds another mirror. */

void run_backfill_test(
        simple_mailbox_cluster_t *cluster,
        primary_dispatcher_t *dispatcher,
        mock_store_t *store1,
        local_replicator_t *local_replicator,
        order_source_t *order_source) {
    /* Start sending operations to the broadcaster */
    std::map<std::string, std::string> inserter_state;
    class inserter_t : public test_inserter_t {
    public:
        inserter_t(
                primary_dispatcher_t *d,
                std::map<std::string, std::string> *s,
                order_source_t *os) :
            test_inserter_t(os, "run_backfill_test::inserter_t", s),
            dispatcher(d)
        {
            start();
        }
        ~inserter_t() {
            if (running()) {
                stop();
            }
        }
    private:
        void write(const std::string &key, const std::string &value,
                order_token_t otok, signal_t *) {
            write_t w = mock_overwrite(key, value);
            simple_write_callback_t write_callback;
            dispatcher->spawn_write(w, otok, &write_callback);
            write_callback.wait_lazily_unordered();
        }
        std::string read(const std::string &, order_token_t, signal_t *) {
            unreachable();
        }
        std::string generate_key() {
            return dummy_key_gen();
        }
        primary_dispatcher_t *dispatcher;
    } inserter(dispatcher, &inserter_state, order_source);

    nap(100);

    remote_replicator_server_t remote_replicator_server(
        cluster->get_mailbox_manager(),
        dispatcher);

    standard_backfill_throttler_t backfill_throttler;
    backfill_progress_tracker_t backfill_progress_tracker;
    peer_id_t nil_peer;

    /* Set up a second mirror */
    mock_store_t store2((binary_blob_t(version_t::zero())));
    in_memory_branch_history_manager_t bhm2;
    cond_t interruptor;
    remote_replicator_client_t remote_replicator_client(
        &backfill_throttler,
        backfill_config_t(),
        &backfill_progress_tracker,
        cluster->get_mailbox_manager(),
        server_id_t::generate_server_id(),
        backfill_throttler_t::priority_t::critical_t::NO,
        dispatcher->get_branch_id(),
        remote_replicator_server.get_bcard(),
        local_replicator->get_replica_bcard(),
        server_id_t::generate_server_id(),
        &store2,
        &bhm2,
        &interruptor);

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
