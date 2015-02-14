// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "clustering/immediate_consistency/branch/backfill_throttler.hpp"
#include "clustering/immediate_consistency/branch/broadcaster.hpp"
#include "clustering/immediate_consistency/branch/listener.hpp"
#include "clustering/immediate_consistency/branch/replier.hpp"
#include "containers/uuid.hpp"
#include "rdb_protocol/protocol.hpp"
#include "unittest/branch_history_manager.hpp"
#include "unittest/clustering_utils.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

namespace {

void run_with_broadcaster(
        std::function<void(io_backender_t *,
                           simple_mailbox_cluster_t *,
                           branch_history_manager_t *,
                           scoped_ptr_t<broadcaster_t> *,
                           mock_store_t *,
                           scoped_ptr_t<listener_t> *,
                           order_source_t *)> fun) {
    order_source_t order_source;

    /* Set up a cluster so mailboxes can be created */
    simple_mailbox_cluster_t cluster;

    /* Set up branch history manager */
    in_memory_branch_history_manager_t branch_history_manager;

    io_backender_t io_backender(file_direct_io_mode_t::buffered_desired);

    /* Set up a broadcaster and initial listener */
    mock_store_t initial_store((binary_blob_t(version_t::zero())));
    cond_t interruptor;

    rdb_context_t rdb_context;

    branch_birth_certificate_t branch_info;
    branch_info.region = region_t::universe();
    branch_info.origin =
        region_map_t<version_t>(region_t::universe(), version_t::zero());
    branch_info.initial_timestamp = state_timestamp_t::zero();

    scoped_ptr_t<broadcaster_t> broadcaster(
        new broadcaster_t(
            cluster.get_mailbox_manager(),
            &initial_store,
            &get_global_perfmon_collection(),
            generate_uuid(),
            branch_info,
            &order_source,
            &interruptor));

    scoped_ptr_t<listener_t> initial_listener(new listener_t(
        base_path_t("."),
        &io_backender,
        cluster.get_mailbox_manager(),
        generate_uuid(),
        broadcaster.get(),
        &get_global_perfmon_collection(),
        &interruptor,
        &order_source));

    fun(&io_backender,
        &cluster,
        &branch_history_manager,
        &broadcaster,
        &initial_store,
        &initial_listener,
        &order_source);
}

}   /* anonymous namespace */

/* The `ReadWrite` test just sends some reads and writes via the broadcaster to a
single mirror. */

void run_read_write_test(UNUSED io_backender_t *io_backender,
                         UNUSED simple_mailbox_cluster_t *cluster,
                         branch_history_manager_t *branch_history_manager,
                         scoped_ptr_t<broadcaster_t> *broadcaster,
                         UNUSED mock_store_t *store,
                         scoped_ptr_t<listener_t> *initial_listener,
                         order_source_t *order_source) {
    /* Set up a replier so the broadcaster can handle operations. */
    replier_t replier(initial_listener->get(), cluster->get_mailbox_manager(), branch_history_manager);

    /* Give time for the broadcaster to see the replier. */
    let_stuff_happen();

    /* Send some writes via the broadcaster to the mirror. */
    std::map<std::string, std::string> values_inserted;
    for (int i = 0; i < 10; i++) {
        std::string key = std::string(1, 'a' + randint(26));
        values_inserted[key] = strprintf("%d", i);
        write_t w = mock_overwrite(key, strprintf("%d", i));
        simple_write_callback_t write_callback;
        (*broadcaster)->spawn_write(
            w,
            order_source->check_in("unittest::run_read_write_test(write)"),
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
        (*broadcaster)->read(r, &resp, &exiter, order_source->check_in("unittest::run_read_write_test(read)").with_read_mode(), &non_interruptor);
        EXPECT_EQ(it->second, mock_parse_read_response(resp));
    }
}

TPTEST(ClusteringBranch, ReadWrite) {
    run_with_broadcaster(&run_read_write_test);
}

/* The `Backfill` test starts up a node with one mirror, inserts some data, and
then adds another mirror. */

static void write_to_broadcaster(broadcaster_t *broadcaster, const std::string& key, const std::string& value, order_token_t otok, signal_t *) {
    write_t w = mock_overwrite(key, value);
    simple_write_callback_t write_callback;
    broadcaster->spawn_write(w, otok, &write_callback);
    write_callback.wait_lazily_unordered();
}

void run_backfill_test(io_backender_t *io_backender,
                       simple_mailbox_cluster_t *cluster,
                       branch_history_manager_t *branch_history_manager,
                       scoped_ptr_t<broadcaster_t> *broadcaster,
                       mock_store_t *store1,
                       scoped_ptr_t<listener_t> *initial_listener,
                       order_source_t *order_source) {
    /* Set up a replier so the broadcaster can handle operations */
    replier_t replier(initial_listener->get(), cluster->get_mailbox_manager(), branch_history_manager);

    /* Start sending operations to the broadcaster */
    std::map<std::string, std::string> inserter_state;
    test_inserter_t inserter(
        std::bind(&write_to_broadcaster, broadcaster->get(),
                  ph::_1, ph::_2, ph::_3, ph::_4),
        std::function<std::string(const std::string &, order_token_t, signal_t *)>(),
        &dummy_key_gen,
        order_source,
        "run_backfill_test/inserter",
        &inserter_state);
    nap(100);

    backfill_throttler_t backfill_throttler;

    /* Set up a second mirror */
    mock_store_t store2((binary_blob_t(version_t::zero())));
    cond_t interruptor;
    listener_t listener2(
        base_path_t("."),
        io_backender,
        cluster->get_mailbox_manager(),
        generate_uuid(),
        &backfill_throttler,
        (*broadcaster)->get_business_card(),
        branch_history_manager,
        &store2,
        replier.get_business_card(),
        &get_global_perfmon_collection(),
        &interruptor,
        order_source,
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
    run_with_broadcaster(&run_backfill_test);
}

/* `PartialBackfill` backfills only in a specific sub-region. */

void run_partial_backfill_test(io_backender_t *io_backender,
                               simple_mailbox_cluster_t *cluster,
                               branch_history_manager_t *branch_history_manager,
                               scoped_ptr_t<broadcaster_t> *broadcaster,
                               mock_store_t *store1,
                               scoped_ptr_t<listener_t> *initial_listener,
                               order_source_t *order_source) {
    /* Set up a replier so the broadcaster can handle operations */
    replier_t replier(initial_listener->get(), cluster->get_mailbox_manager(), branch_history_manager);

    /* Start sending operations to the broadcaster */
    std::map<std::string, std::string> inserter_state;
    test_inserter_t inserter(
        std::bind(&write_to_broadcaster, broadcaster->get(),
                  ph::_1, ph::_2, ph::_3, ph::_4),
        std::function<std::string(const std::string &, order_token_t, signal_t *)>(),
        &dummy_key_gen,
        order_source,
        "run_partial_backfill_test/inserter",
        &inserter_state);
    nap(100);

    backfill_throttler_t backfill_throttler;

    /* Set up a second mirror */
    mock_store_t store2((binary_blob_t(version_t::zero())));
    region_t subregion(key_range_t(key_range_t::closed, store_key_t("a"),
                                                   key_range_t::open, store_key_t("n")));
    cond_t interruptor;
    listener_t listener2(
        base_path_t("."),
        io_backender,
        cluster->get_mailbox_manager(),
        generate_uuid(),
        &backfill_throttler,
        (*broadcaster)->get_business_card(),
        branch_history_manager,
        &store2,
        replier.get_business_card(),
        &get_global_perfmon_collection(),
        &interruptor,
        order_source,
        nullptr);

    nap(100);

    /* Stop the inserter, then let any lingering writes finish */
    inserter.stop();

    /* Let any lingering writes finish */
    let_stuff_happen();

    /* Confirm that both mirrors have all of the writes */
    for (std::map<std::string, std::string>::iterator it = inserter.values_inserted->begin();
            it != inserter.values_inserted->end(); it++) {
        if (region_contains_key(subregion, store_key_t(it->first))) {
            EXPECT_EQ(it->second, mock_lookup(store1, it->first));
            EXPECT_EQ(it->second, mock_lookup(&store2, it->first));
        }
    }
}
TPTEST(ClusteringBranch, PartialBackfill) {
    run_with_broadcaster(&run_partial_backfill_test);
}

}   /* namespace unittest */
