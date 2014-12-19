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

boost::optional<boost::optional<broadcaster_business_card_t> > wrap_broadcaster_in_optional(
        const boost::optional<broadcaster_business_card_t> &inner) {
    return boost::optional<boost::optional<broadcaster_business_card_t> >(inner);
}

boost::optional<boost::optional<replier_business_card_t> > wrap_replier_in_optional(
        const boost::optional<replier_business_card_t> &inner) {
    return boost::optional<boost::optional<replier_business_card_t> >(inner);
}

void run_with_broadcaster(
        std::function<void(io_backender_t *,
                           simple_mailbox_cluster_t *,
                           branch_history_manager_t *,
                           clone_ptr_t<watchable_t<boost::optional<broadcaster_business_card_t> > >,
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
    mock_store_t initial_store((binary_blob_t(version_range_t(version_t::zero()))));
    cond_t interruptor;

    rdb_context_t rdb_context;

    scoped_ptr_t<broadcaster_t> broadcaster(
        new broadcaster_t(
            cluster.get_mailbox_manager(),
            &rdb_context,
            &branch_history_manager,
            &initial_store,
            &get_global_perfmon_collection(),
            &order_source,
            &interruptor));

    watchable_variable_t<boost::optional<broadcaster_business_card_t> > broadcaster_directory_controller(
        boost::optional<broadcaster_business_card_t>(broadcaster->get_business_card()));

    scoped_ptr_t<listener_t> initial_listener(new listener_t(
        base_path_t("."),
        &io_backender,
        cluster.get_mailbox_manager(),
        generate_uuid(),
        broadcaster_directory_controller.get_watchable()->subview(
            &wrap_broadcaster_in_optional),
        &branch_history_manager,
        broadcaster.get(),
        &get_global_perfmon_collection(),
        &interruptor,
        &order_source));

    fun(&io_backender,
        &cluster,
        &branch_history_manager,
        broadcaster_directory_controller.get_watchable(),
        &broadcaster,
        &initial_store,
        &initial_listener,
        &order_source);
}

void run_in_thread_pool_with_broadcaster(
        std::function<void(io_backender_t *,
                           simple_mailbox_cluster_t *,
                           branch_history_manager_t *,
                           clone_ptr_t<watchable_t<boost::optional<broadcaster_business_card_t> > >,
                           scoped_ptr_t<broadcaster_t> *,
                           mock_store_t *,
                           scoped_ptr_t<listener_t> *,
                           order_source_t *)> fun)
{
    unittest::run_in_thread_pool(std::bind(&run_with_broadcaster, fun));
}

}   /* anonymous namespace */

/* The `ReadWrite` test just sends some reads and writes via the broadcaster to a
single mirror. */

void run_read_write_test(UNUSED io_backender_t *io_backender,
                         UNUSED simple_mailbox_cluster_t *cluster,
                         branch_history_manager_t *branch_history_manager,
                         UNUSED clone_ptr_t<watchable_t<boost::optional<broadcaster_business_card_t> > > broadcaster_metadata_view,
                         scoped_ptr_t<broadcaster_t> *broadcaster,
                         UNUSED mock_store_t *store,
                         scoped_ptr_t<listener_t> *initial_listener,
                         order_source_t *order_source) {
    /* Set up a replier so the broadcaster can handle operations. */
    EXPECT_FALSE((*initial_listener)->get_broadcaster_lost_signal()->is_pulsed());
    replier_t replier(initial_listener->get(), cluster->get_mailbox_manager(), branch_history_manager);

    /* Give time for the broadcaster to see the replier. */
    let_stuff_happen();

    /* Send some writes via the broadcaster to the mirror. */
    std::map<std::string, std::string> values_inserted;
    for (int i = 0; i < 10; i++) {
        unittest::fake_fifo_enforcement_t enforce;
        fifo_enforcer_sink_t::exit_write_t exiter(&enforce.sink, enforce.source.enter_write());

        std::string key = std::string(1, 'a' + randint(26));
        values_inserted[key] = strprintf("%d", i);
        write_t w = mock_overwrite(key, strprintf("%d", i));

        class : public broadcaster_t::write_callback_t, public cond_t {
        public:
            void on_success(const write_response_t &) {
                pulse();
            }
            void on_failure(UNUSED bool might_have_been_run) {
                EXPECT_TRUE(false);
            }
        } write_callback;
        cond_t non_interruptor;
        fake_ack_checker_t ack_checker(1);
        (*broadcaster)->spawn_write(w, &exiter, order_source->check_in("unittest::run_read_write_test(write)"), &write_callback, &non_interruptor, &ack_checker);
        write_callback.wait_lazily_unordered();
    }

    /* Now send some reads. */
    for (std::map<std::string, std::string>::iterator it = values_inserted.begin();
            it != values_inserted.end(); it++) {
        unittest::fake_fifo_enforcement_t enforce;
        fifo_enforcer_sink_t::exit_read_t exiter(&enforce.sink, enforce.source.enter_read());

        read_t r = mock_read(it->first);
        cond_t non_interruptor;
        read_response_t resp;
        (*broadcaster)->read(r, &resp, &exiter, order_source->check_in("unittest::run_read_write_test(read)").with_read_mode(), &non_interruptor);
        EXPECT_EQ(it->second, mock_parse_read_response(resp));
    }
}

TEST(ClusteringBranch, ReadWrite) {
    run_in_thread_pool_with_broadcaster(&run_read_write_test);
}

/* The `Backfill` test starts up a node with one mirror, inserts some data, and
then adds another mirror. */

static void write_to_broadcaster(broadcaster_t *broadcaster, const std::string& key, const std::string& value, order_token_t otok, signal_t *) {
    // TODO: Is this the right place?  Maybe we should have real fifo enforcement for this helper function.
    unittest::fake_fifo_enforcement_t enforce;
    fifo_enforcer_sink_t::exit_write_t exiter(&enforce.sink, enforce.source.enter_write());
    write_t w = mock_overwrite(key, value);
    class : public broadcaster_t::write_callback_t, public cond_t {
    public:
        void on_success(const write_response_t &) {
            pulse();
        }
        void on_failure(UNUSED bool might_have_been_run) {
            EXPECT_TRUE(false);
        }
    } write_callback;
    fake_ack_checker_t ack_checker(1);
    cond_t non_interruptor;
    broadcaster->spawn_write(w, &exiter, otok, &write_callback, &non_interruptor, &ack_checker);
    write_callback.wait_lazily_unordered();
}

void run_backfill_test(io_backender_t *io_backender,
                       simple_mailbox_cluster_t *cluster,
                       branch_history_manager_t *branch_history_manager,
                       clone_ptr_t<watchable_t<boost::optional<broadcaster_business_card_t> > > broadcaster_metadata_view,
                       scoped_ptr_t<broadcaster_t> *broadcaster,
                       mock_store_t *store1,
                       scoped_ptr_t<listener_t> *initial_listener,
                       order_source_t *order_source) {
    /* Set up a replier so the broadcaster can handle operations */
    EXPECT_FALSE((*initial_listener)->get_broadcaster_lost_signal()->is_pulsed());
    replier_t replier(initial_listener->get(), cluster->get_mailbox_manager(), branch_history_manager);

    watchable_variable_t<boost::optional<replier_business_card_t> > replier_directory_controller(
        boost::optional<replier_business_card_t>(replier.get_business_card()));

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
    mock_store_t store2((binary_blob_t(version_range_t(version_t::zero()))));
    cond_t interruptor;
    listener_t listener2(
        base_path_t("."),
        io_backender,
        cluster->get_mailbox_manager(),
        generate_uuid(),
        &backfill_throttler,
        broadcaster_metadata_view->subview(&wrap_broadcaster_in_optional),
        branch_history_manager,
        &store2,
        replier_directory_controller.get_watchable()->subview(&wrap_replier_in_optional),
        generate_uuid(),
        &get_global_perfmon_collection(),
        &interruptor,
        order_source);

    EXPECT_FALSE((*initial_listener)->get_broadcaster_lost_signal()->is_pulsed());
    EXPECT_FALSE(listener2.get_broadcaster_lost_signal()->is_pulsed());

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
TEST(ClusteringBranch, Backfill) {
    run_in_thread_pool_with_broadcaster(&run_backfill_test);
}

/* `PartialBackfill` backfills only in a specific sub-region. */

void run_partial_backfill_test(io_backender_t *io_backender,
                               simple_mailbox_cluster_t *cluster,
                               branch_history_manager_t *branch_history_manager,
                               clone_ptr_t<watchable_t<boost::optional<broadcaster_business_card_t> > > broadcaster_metadata_view,
                               scoped_ptr_t<broadcaster_t> *broadcaster,
                               mock_store_t *store1,
                               scoped_ptr_t<listener_t> *initial_listener,
                               order_source_t *order_source) {
    /* Set up a replier so the broadcaster can handle operations */
    EXPECT_FALSE((*initial_listener)->get_broadcaster_lost_signal()->is_pulsed());
    replier_t replier(initial_listener->get(), cluster->get_mailbox_manager(), branch_history_manager);

    watchable_variable_t<boost::optional<replier_business_card_t> > replier_directory_controller(
        boost::optional<replier_business_card_t>(replier.get_business_card()));

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
    mock_store_t store2((binary_blob_t(version_range_t(version_t::zero()))));
    region_t subregion(key_range_t(key_range_t::closed, store_key_t("a"),
                                                   key_range_t::open, store_key_t("n")));
    cond_t interruptor;
    listener_t listener2(
        base_path_t("."),
        io_backender,
        cluster->get_mailbox_manager(),
        generate_uuid(),
        &backfill_throttler,
        broadcaster_metadata_view->subview(&wrap_broadcaster_in_optional),
        branch_history_manager,
        &store2,
        replier_directory_controller.get_watchable()->subview(&wrap_replier_in_optional),
        generate_uuid(),
        &get_global_perfmon_collection(),
        &interruptor,
        order_source);

    EXPECT_FALSE((*initial_listener)->get_broadcaster_lost_signal()->is_pulsed());
    EXPECT_FALSE(listener2.get_broadcaster_lost_signal()->is_pulsed());

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
TEST(ClusteringBranch, PartialBackfill) {
    run_in_thread_pool_with_broadcaster(&run_partial_backfill_test);
}

}   /* namespace unittest */
