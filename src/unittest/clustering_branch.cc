#include "unittest/gtest.hpp"

#include "clustering/immediate_consistency/branch/broadcaster.hpp"
#include "clustering/immediate_consistency/branch/listener.hpp"
#include "clustering/immediate_consistency/branch/replier.hpp"
#include "containers/uuid.hpp"
#include "mock/branch_history_manager.hpp"
#include "mock/clustering_utils.hpp"
#include "mock/dummy_protocol.hpp"
#include "mock/unittest_utils.hpp"

using mock::dummy_protocol_t;

namespace unittest {

namespace {

boost::optional<boost::optional<broadcaster_business_card_t<dummy_protocol_t> > > wrap_broadcaster_in_optional(
        const boost::optional<broadcaster_business_card_t<dummy_protocol_t> > &inner) {
    return boost::optional<boost::optional<broadcaster_business_card_t<dummy_protocol_t> > >(inner);
}

boost::optional<boost::optional<replier_business_card_t<dummy_protocol_t> > > wrap_replier_in_optional(
        const boost::optional<replier_business_card_t<dummy_protocol_t> > &inner) {
    return boost::optional<boost::optional<replier_business_card_t<dummy_protocol_t> > >(inner);
}

// TODO: Make this's argument take the multistore_ptr_t in addition to the test_store_t.
void run_with_broadcaster(
        boost::function<void(io_backender_t *,
                             mock::simple_mailbox_cluster_t *,
                             branch_history_manager_t<dummy_protocol_t> *,
                             clone_ptr_t<watchable_t<boost::optional<broadcaster_business_card_t<dummy_protocol_t> > > >,
                             scoped_ptr_t<broadcaster_t<dummy_protocol_t> > *,
                             mock::test_store_t<dummy_protocol_t> *,
                             scoped_ptr_t<listener_t<dummy_protocol_t> > *,
                             order_source_t *)> fun) {
    order_source_t order_source;

    /* Set up a cluster so mailboxes can be created */
    mock::simple_mailbox_cluster_t cluster;

    /* Set up branch history manager */
    mock::in_memory_branch_history_manager_t<dummy_protocol_t> branch_history_manager;

    scoped_ptr_t<io_backender_t> io_backender;
    make_io_backender(aio_default, &io_backender);

    /* Set up a broadcaster and initial listener */
    mock::test_store_t<dummy_protocol_t> initial_store(io_backender.get(), &order_source);
    store_view_t<dummy_protocol_t> *initial_store_ptr = &initial_store.store;
    dummy_protocol_t::context_t ctx;
    multistore_ptr_t<dummy_protocol_t> initial_svs(&initial_store_ptr, 1, &ctx);
    cond_t interruptor;

    scoped_ptr_t<broadcaster_t<dummy_protocol_t> > broadcaster(
        new broadcaster_t<dummy_protocol_t>(
            cluster.get_mailbox_manager(),
            &branch_history_manager,
            &initial_svs,
            &get_global_perfmon_collection(),
            &order_source,
            &interruptor));

    watchable_variable_t<boost::optional<broadcaster_business_card_t<dummy_protocol_t> > > broadcaster_directory_controller(
        boost::optional<broadcaster_business_card_t<dummy_protocol_t> >(broadcaster->get_business_card()));

    scoped_ptr_t<listener_t<dummy_protocol_t> > initial_listener(
        new listener_t<dummy_protocol_t>(io_backender.get(),
                                         cluster.get_mailbox_manager(),
                                         broadcaster_directory_controller.get_watchable()->subview(&wrap_broadcaster_in_optional),
                                         &branch_history_manager,
                                         broadcaster.get(),
                                         &get_global_perfmon_collection(),
                                         &interruptor,
                                         &order_source));

    fun(io_backender.get(),
        &cluster,
        &branch_history_manager,
        broadcaster_directory_controller.get_watchable(),
        &broadcaster,
        &initial_store,
        &initial_listener,
        &order_source);
}

void run_in_thread_pool_with_broadcaster(
        boost::function<void(io_backender_t *,
                             mock::simple_mailbox_cluster_t *,
                             branch_history_manager_t<dummy_protocol_t> *,
                             clone_ptr_t<watchable_t<boost::optional<broadcaster_business_card_t<dummy_protocol_t> > > >,
                             scoped_ptr_t<broadcaster_t<dummy_protocol_t> > *,
                             mock::test_store_t<dummy_protocol_t> *,
                             scoped_ptr_t<listener_t<dummy_protocol_t> > *,
                             order_source_t *)> fun)
{
    mock::run_in_thread_pool(boost::bind(&run_with_broadcaster, fun));
}

}   /* anonymous namespace */

/* The `ReadWrite` test just sends some reads and writes via the broadcaster to a
single mirror. */

void run_read_write_test(UNUSED io_backender_t *io_backender,
                         UNUSED mock::simple_mailbox_cluster_t *cluster,
                         branch_history_manager_t<dummy_protocol_t> *branch_history_manager,
                         UNUSED clone_ptr_t<watchable_t<boost::optional<broadcaster_business_card_t<dummy_protocol_t> > > > broadcaster_metadata_view,
                         scoped_ptr_t<broadcaster_t<dummy_protocol_t> > *broadcaster,
                         UNUSED mock::test_store_t<dummy_protocol_t> *store,
                         scoped_ptr_t<listener_t<dummy_protocol_t> > *initial_listener,
                         order_source_t *order_source) {
    /* Set up a replier so the broadcaster can handle operations */
    EXPECT_FALSE((*initial_listener)->get_broadcaster_lost_signal()->is_pulsed());
    replier_t<dummy_protocol_t> replier(initial_listener->get(), cluster->get_mailbox_manager(), branch_history_manager);

    /* Give time for the broadcaster to see the replier */
    mock::let_stuff_happen();

    /* Send some writes via the broadcaster to the mirror */
    std::map<std::string, std::string> values_inserted;
    for (int i = 0; i < 10; i++) {
        mock::fake_fifo_enforcement_t enforce;
        fifo_enforcer_sink_t::exit_write_t exiter(&enforce.sink, enforce.source.enter_write());

        dummy_protocol_t::write_t w;
        std::string key = std::string(1, 'a' + randint(26));
        w.values[key] = values_inserted[key] = strprintf("%d", i);
        class : public broadcaster_t<dummy_protocol_t>::write_callback_t, public cond_t {
        public:
            void on_response(peer_id_t, const dummy_protocol_t::write_response_t &) {
                /* ignore */
            }
            void on_done() {
                pulse();
            }
        } write_callback;
        cond_t non_interruptor;
        (*broadcaster)->spawn_write(w, &exiter, order_source->check_in("unittest::run_read_write_test(write)"), &write_callback, &non_interruptor);
        write_callback.wait_lazily_unordered();
    }

    /* Now send some reads */
    for (std::map<std::string, std::string>::iterator it = values_inserted.begin();
            it != values_inserted.end(); it++) {
        mock::fake_fifo_enforcement_t enforce;
        fifo_enforcer_sink_t::exit_read_t exiter(&enforce.sink, enforce.source.enter_read());

        dummy_protocol_t::read_t r;
        r.keys.keys.insert((*it).first);
        cond_t non_interruptor;
        dummy_protocol_t::read_response_t resp;
        (*broadcaster)->read(r, &resp, &exiter, order_source->check_in("unittest::run_read_write_test(read)").with_read_mode(), &non_interruptor);
        EXPECT_EQ((*it).second, resp.values[(*it).first]);
    }
}

TEST(ClusteringBranch, ReadWrite) {
    run_in_thread_pool_with_broadcaster(&run_read_write_test);
}

/* The `Backfill` test starts up a node with one mirror, inserts some data, and
then adds another mirror. */

static void write_to_broadcaster(broadcaster_t<dummy_protocol_t> *broadcaster, const std::string& key, const std::string& value, order_token_t otok, signal_t *) {
    // TODO: Is this the right place?  Maybe we should have real fifo enforcement for this helper function.
    mock::fake_fifo_enforcement_t enforce;
    fifo_enforcer_sink_t::exit_write_t exiter(&enforce.sink, enforce.source.enter_write());
    dummy_protocol_t::write_t w;
    w.values[key] = value;
    class : public broadcaster_t<dummy_protocol_t>::write_callback_t, public cond_t {
    public:
        void on_response(peer_id_t, const dummy_protocol_t::write_response_t &) {
            /* ignore */
        }
        void on_done() {
            pulse();
        }
    } write_callback;
    cond_t non_interruptor;
    broadcaster->spawn_write(w, &exiter, otok, &write_callback, &non_interruptor);
    write_callback.wait_lazily_unordered();
}

void run_backfill_test(io_backender_t *io_backender,
                       mock::simple_mailbox_cluster_t *cluster,
                       branch_history_manager_t<dummy_protocol_t> *branch_history_manager,
                       clone_ptr_t<watchable_t<boost::optional<broadcaster_business_card_t<dummy_protocol_t> > > > broadcaster_metadata_view,
                       scoped_ptr_t<broadcaster_t<dummy_protocol_t> > *broadcaster,
                       mock::test_store_t<dummy_protocol_t> *store1,
                       scoped_ptr_t<listener_t<dummy_protocol_t> > *initial_listener,
                       order_source_t *order_source) {
    /* Set up a replier so the broadcaster can handle operations */
    EXPECT_FALSE((*initial_listener)->get_broadcaster_lost_signal()->is_pulsed());
    replier_t<dummy_protocol_t> replier(initial_listener->get(), cluster->get_mailbox_manager(), branch_history_manager);

    watchable_variable_t<boost::optional<replier_business_card_t<dummy_protocol_t> > > replier_directory_controller(
        boost::optional<replier_business_card_t<dummy_protocol_t> >(replier.get_business_card()));

    /* Start sending operations to the broadcaster */
    std::map<std::string, std::string> inserter_state;
    mock::test_inserter_t inserter(
        boost::bind(&write_to_broadcaster, broadcaster->get(), _1, _2, _3, _4),
        NULL,
        &mock::dummy_key_gen,
        order_source,
        "run_backfill_test/inserter",
        &inserter_state);
    nap(100);

    /* Set up a second mirror */
    mock::test_store_t<dummy_protocol_t> store2(io_backender, order_source);
    store_view_t<dummy_protocol_t> *store2_ptr = &store2.store;
    dummy_protocol_t::context_t ctx;
    multistore_ptr_t<dummy_protocol_t> store2_multi_ptr(&store2_ptr, 1, &ctx);
    cond_t interruptor;
    listener_t<dummy_protocol_t> listener2(
        io_backender,
        cluster->get_mailbox_manager(),
        broadcaster_metadata_view->subview(&wrap_broadcaster_in_optional),
        branch_history_manager,
        &store2_multi_ptr,
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
    mock::let_stuff_happen();

    /* Confirm that both mirrors have all of the writes */
    for (std::map<std::string, std::string>::iterator it = inserter.values_inserted->begin();
            it != inserter.values_inserted->end(); it++) {
        EXPECT_EQ((*it).second, store1->store.values[(*it).first]);
        EXPECT_EQ((*it).second, store2.store.values[(*it).first]);
    }
}
TEST(ClusteringBranch, Backfill) {
    run_in_thread_pool_with_broadcaster(&run_backfill_test);
}

/* `PartialBackfill` backfills only in a specific sub-region. */

void run_partial_backfill_test(io_backender_t *io_backender,
                               mock::simple_mailbox_cluster_t *cluster,
                               branch_history_manager_t<dummy_protocol_t> *branch_history_manager,
                               clone_ptr_t<watchable_t<boost::optional<broadcaster_business_card_t<dummy_protocol_t> > > > broadcaster_metadata_view,
                               scoped_ptr_t<broadcaster_t<dummy_protocol_t> > *broadcaster,
                               mock::test_store_t<dummy_protocol_t> *store1,
                               scoped_ptr_t<listener_t<dummy_protocol_t> > *initial_listener,
                               order_source_t *order_source) {
    /* Set up a replier so the broadcaster can handle operations */
    EXPECT_FALSE((*initial_listener)->get_broadcaster_lost_signal()->is_pulsed());
    replier_t<dummy_protocol_t> replier(initial_listener->get(), cluster->get_mailbox_manager(), branch_history_manager);

    watchable_variable_t<boost::optional<replier_business_card_t<dummy_protocol_t> > > replier_directory_controller(
        boost::optional<replier_business_card_t<dummy_protocol_t> >(replier.get_business_card()));

    /* Start sending operations to the broadcaster */
    std::map<std::string, std::string> inserter_state;
    mock::test_inserter_t inserter(
        boost::bind(&write_to_broadcaster, broadcaster->get(), _1, _2, _3, _4),
        NULL,
        &mock::dummy_key_gen,
        order_source,
        "run_partial_backfill_test/inserter",
        &inserter_state);
    nap(100);

    /* Set up a second mirror */
    mock::test_store_t<dummy_protocol_t> store2(io_backender, order_source);
    store_view_t<dummy_protocol_t> *store2_ptr = &store2.store;
    dummy_protocol_t::region_t subregion('a', 'm');
    dummy_protocol_t::context_t ctx;
    multistore_ptr_t<dummy_protocol_t> store_ptr(&store2_ptr, 1, &ctx, subregion);
    cond_t interruptor;
    listener_t<dummy_protocol_t> listener2(
        io_backender,
        cluster->get_mailbox_manager(),
        broadcaster_metadata_view->subview(&wrap_broadcaster_in_optional),
        branch_history_manager,
        &store_ptr,
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
    mock::let_stuff_happen();

    /* Confirm that both mirrors have all of the writes */
    for (std::map<std::string, std::string>::iterator it = inserter.values_inserted->begin();
            it != inserter.values_inserted->end(); it++) {
        if (subregion.keys.count((*it).first) > 0) {
            EXPECT_EQ((*it).second, store1->store.values[(*it).first]);
            EXPECT_EQ((*it).second, store2.store.values[(*it).first]);
        }
    }
}
TEST(ClusteringBranch, PartialBackfill) {
    run_in_thread_pool_with_broadcaster(&run_partial_backfill_test);
}

}   /* namespace unittest */
