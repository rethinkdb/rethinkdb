#include "unittest/gtest.hpp"
#include "clustering/immediate_consistency/branch/broadcaster.hpp"
#include "clustering/immediate_consistency/branch/listener.hpp"
#include "clustering/immediate_consistency/branch/replier.hpp"
#include "unittest/dummy_metadata_controller.hpp"
#include "mock/dummy_protocol.hpp"
#include "unittest/clustering_utils.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

namespace {

void run_with_broadcaster(
        boost::function< void(
            simple_mailbox_cluster_t *,
            boost::shared_ptr<semilattice_readwrite_view_t<branch_history_t<dummy_protocol_t> > >,
            clone_ptr_t<directory_single_rview_t<boost::optional<broadcaster_business_card_t<dummy_protocol_t> > > >,
            boost::scoped_ptr<broadcaster_t<dummy_protocol_t> > *,
            test_store_t *,
            boost::scoped_ptr<listener_t<dummy_protocol_t> > *
            )> fun)
{
    /* Set up a cluster so mailboxes can be created */
    simple_mailbox_cluster_t cluster;

    /* Set up metadata meeting-places */
    branch_history_t<dummy_protocol_t> initial_branch_history;
    dummy_semilattice_controller_t<branch_history_t<dummy_protocol_t> >
        branch_history_controller(initial_branch_history);

    /* Set up a broadcaster and initial listener */
    test_store_t initial_store;
    cond_t interruptor;

    boost::scoped_ptr<broadcaster_t<dummy_protocol_t> > broadcaster(
        new broadcaster_t<dummy_protocol_t>(
            cluster.get_mailbox_manager(),
            branch_history_controller.get_view(),
            &initial_store.store,
            &interruptor
        ));

    simple_directory_manager_t<boost::optional<broadcaster_business_card_t<dummy_protocol_t> > >
        broadcaster_metadata_controller(&cluster,
            boost::optional<broadcaster_business_card_t<dummy_protocol_t> >(broadcaster->get_business_card())
            );

    boost::scoped_ptr<listener_t<dummy_protocol_t> > initial_listener(
        new listener_t<dummy_protocol_t>(
            cluster.get_mailbox_manager(),
            broadcaster_metadata_controller.get_root_view()->
                get_peer_view(cluster.get_connectivity_service()->get_me()),
            branch_history_controller.get_view(),
            broadcaster.get(),
            &interruptor
        ));

    fun(&cluster,
        branch_history_controller.get_view(),
        broadcaster_metadata_controller.get_root_view()->
            get_peer_view(cluster.get_connectivity_service()->get_me()),
        &broadcaster,
        &initial_store,
        &initial_listener);
}

void run_in_thread_pool_with_broadcaster(
        boost::function< void(
            simple_mailbox_cluster_t *,
            boost::shared_ptr<semilattice_readwrite_view_t<branch_history_t<dummy_protocol_t> > >,
            clone_ptr_t<directory_single_rview_t<boost::optional<broadcaster_business_card_t<dummy_protocol_t> > > >,
            boost::scoped_ptr<broadcaster_t<dummy_protocol_t> > *,
            test_store_t *,
            boost::scoped_ptr<listener_t<dummy_protocol_t> > *
            )> fun)
{
    run_in_thread_pool(boost::bind(&run_with_broadcaster, fun));
}

/* `let_stuff_happen()` delays for some time to let events occur */
void let_stuff_happen() {
    nap(1000);
}

}   /* anonymous namespace */

/* The `ReadWrite` test just sends some reads and writes via the broadcaster to a
single mirror. */

void run_read_write_test(UNUSED simple_mailbox_cluster_t *cluster,
        UNUSED boost::shared_ptr<semilattice_readwrite_view_t<branch_history_t<dummy_protocol_t> > > branch_history_view,
        UNUSED clone_ptr_t<directory_single_rview_t<boost::optional<broadcaster_business_card_t<dummy_protocol_t> > > > broadcaster_metadata_view,
        boost::scoped_ptr<broadcaster_t<dummy_protocol_t> > *broadcaster,
        UNUSED test_store_t *store,
        boost::scoped_ptr<listener_t<dummy_protocol_t> > *initial_listener)
{
    /* Set up a replier so the broadcaster can handle operations */
    EXPECT_FALSE((*initial_listener)->get_broadcaster_lost_signal()->is_pulsed());
    replier_t<dummy_protocol_t> replier(initial_listener->get());

    order_source_t order_source;

    /* Send some writes via the broadcaster to the mirror */
    std::map<std::string, std::string> values_inserted;
    for (int i = 0; i < 10; i++) {
        dummy_protocol_t::write_t w;
        std::string key = std::string(1, 'a' + randint(26));
        w.values[key] = values_inserted[key] = strprintf("%d", i);
        (*broadcaster)->write(w, order_source.check_in("unittest"));
    }

    /* Now send some reads */
    for (std::map<std::string, std::string>::iterator it = values_inserted.begin();
            it != values_inserted.end(); it++) {
        dummy_protocol_t::read_t r;
        r.keys.keys.insert((*it).first);
        dummy_protocol_t::read_response_t resp = (*broadcaster)->read(r, order_source.check_in("unittest"));
        EXPECT_EQ((*it).second, resp.values[(*it).first]);
    }
}

TEST(ClusteringBranch, ReadWrite) {
    run_in_thread_pool_with_broadcaster(&run_read_write_test);
}

/* The `Backfill` test starts up a node with one mirror, inserts some data, and
then adds another mirror. */

void write_to_broadcaster(broadcaster_t<dummy_protocol_t> *broadcaster, const std::string& key, const std::string& value, order_token_t otok, signal_t *) {
    dummy_protocol_t::write_t w;
    w.values[key] = value;
    broadcaster->write(w, otok);
}

void run_backfill_test(simple_mailbox_cluster_t *cluster,
        boost::shared_ptr<semilattice_readwrite_view_t<branch_history_t<dummy_protocol_t> > > branch_history_view,
        clone_ptr_t<directory_single_rview_t<boost::optional<broadcaster_business_card_t<dummy_protocol_t> > > > broadcaster_metadata_view,
        boost::scoped_ptr<broadcaster_t<dummy_protocol_t> > *broadcaster,
        test_store_t *store1,
        boost::scoped_ptr<listener_t<dummy_protocol_t> > *initial_listener)
{
    /* Set up a replier so the broadcaster can handle operations */
    EXPECT_FALSE((*initial_listener)->get_broadcaster_lost_signal()->is_pulsed());
    replier_t<dummy_protocol_t> replier(initial_listener->get());

    simple_directory_manager_t<boost::optional<replier_business_card_t<dummy_protocol_t> > >
        replier_directory_controller(
            cluster,
            boost::optional<replier_business_card_t<dummy_protocol_t> >(replier.get_business_card())
            );

    order_source_t order_source;

    /* Start sending operations to the broadcaster */
    std::map<std::string, std::string> inserter_state;
    test_inserter_t inserter(
        boost::bind(&write_to_broadcaster, broadcaster->get(), _1, _2, _3, _4),
        NULL,
        &order_source,
        &inserter_state);
    nap(100);

    /* Set up a second mirror */
    test_store_t store2;
    cond_t interruptor;
    listener_t<dummy_protocol_t> listener2(
        cluster->get_mailbox_manager(),
        broadcaster_metadata_view,
        branch_history_view,
        &store2.store,
        replier_directory_controller.get_root_view()->
            get_peer_view(cluster->get_connectivity_service()->get_me()),
        &interruptor);

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
        EXPECT_EQ((*it).second, store1->store.values[(*it).first]);
        EXPECT_EQ((*it).second, store2.store.values[(*it).first]);
    }
}
TEST(ClusteringBranch, Backfill) {
    run_in_thread_pool_with_broadcaster(&run_backfill_test);
}

}   /* namespace unittest */
