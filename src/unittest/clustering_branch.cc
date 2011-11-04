#include "unittest/gtest.hpp"
#include "clustering/immediate_consistency/branch/broadcaster.hpp"
#include "clustering/immediate_consistency/branch/listener.hpp"
#include "clustering/immediate_consistency/branch/replier.hpp"
#include "unittest/dummy_metadata_controller.hpp"
#include "unittest/dummy_protocol.hpp"
#include "unittest/clustering_utils.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

namespace {

void run_with_broadcaster(
        boost::function<void(
            mailbox_cluster_t *,
            boost::shared_ptr<metadata_readwrite_view_t<namespace_branch_metadata_t<dummy_protocol_t> > >,
            boost::scoped_ptr<broadcaster_t<dummy_protocol_t> > *,
            test_store_t *,
            boost::scoped_ptr<listener_t<dummy_protocol_t> > *
            )> fun)
{
    /* Set up a cluster so mailboxes can be created */
    simple_mailbox_cluster_t cluster;

    /* Set up a metadata meeting-place */
    namespace_branch_metadata_t<dummy_protocol_t> initial_metadata;
    dummy_metadata_controller_t<namespace_branch_metadata_t<dummy_protocol_t> > metadata_controller(initial_metadata);

    /* Set up a broadcaster and initial listener */
    test_store_t initial_store;
    cond_t interruptor;
    boost::scoped_ptr<listener_t<dummy_protocol_t> > initial_listener;
    boost::scoped_ptr<broadcaster_t<dummy_protocol_t> > broadcaster(
        new broadcaster_t<dummy_protocol_t>(
            &cluster,
            metadata_controller.get_view(),
            &initial_store.store,
            &interruptor,
            &initial_listener
        ));

    fun(&cluster, metadata_controller.get_view(), &broadcaster, &initial_store, &initial_listener);
}

void run_in_thread_pool_with_broadcaster(
        boost::function<void(
            mailbox_cluster_t *,
            boost::shared_ptr<metadata_readwrite_view_t<namespace_branch_metadata_t<dummy_protocol_t> > >,
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

void run_read_write_test(mailbox_cluster_t *cluster,
        boost::shared_ptr<metadata_readwrite_view_t<namespace_branch_metadata_t<dummy_protocol_t> > > metadata_view,
        boost::scoped_ptr<broadcaster_t<dummy_protocol_t> > *broadcaster,
        UNUSED test_store_t *store,
        boost::scoped_ptr<listener_t<dummy_protocol_t> > *initial_listener)
{
    /* Set up a replier so the broadcaster can handle operations */
    EXPECT_FALSE((*initial_listener)->get_outdated_signal()->is_pulsed());
    replier_t<dummy_protocol_t> replier(cluster, metadata_view, initial_listener->get());

    order_source_t order_source;

    /* Send some writes via the broadcaster to the mirror */
    std::map<std::string, std::string> values_inserted;
    for (int i = 0; i < 10; i++) {
        dummy_protocol_t::write_t w;
        std::string key = std::string(1, 'a' + rand() % 26);
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
// TODO(sam): TODO(tim): @sam @tim Tim needs to fix the bug that
// causes this test to fail.  See issue #479.

// TEST(ClusteringBranch, ReadWrite) {
//     run_in_thread_pool_with_broadcaster(&run_read_write_test);
// }

/* The `Backfill` test starts up a node with one mirror, inserts some data, and
then adds another mirror. */

void run_backfill_test(mailbox_cluster_t *cluster,
        boost::shared_ptr<metadata_readwrite_view_t<namespace_branch_metadata_t<dummy_protocol_t> > > metadata_view,
        boost::scoped_ptr<broadcaster_t<dummy_protocol_t> > *broadcaster,
        test_store_t *store1,
        boost::scoped_ptr<listener_t<dummy_protocol_t> > *initial_listener)
{
    /* Set up a replier so the broadcaster can handle operations */
    EXPECT_FALSE((*initial_listener)->get_outdated_signal()->is_pulsed());
    replier_t<dummy_protocol_t> replier(cluster, metadata_view, initial_listener->get());

    order_source_t order_source;

    /* Start sending operations to the broadcaster */
    inserter_t inserter(
        boost::bind(&broadcaster_t<dummy_protocol_t>::write, broadcaster->get(), _1, _2),
        &order_source);
    nap(100);

    /* Set up a second mirror */
    test_store_t store2;
    cond_t interruptor;
    listener_t<dummy_protocol_t> listener2(
        cluster, metadata_view,
        &store2.store,
        (*broadcaster)->get_branch_id(),
        replier.get_backfiller_id(),
        &interruptor);

    EXPECT_FALSE((*initial_listener)->get_outdated_signal()->is_pulsed());
    EXPECT_FALSE(listener2.get_outdated_signal()->is_pulsed());

    nap(100);

    /* Stop the inserter, then let any lingering writes finish */
    inserter.stop();
    /* Let any lingering writes finish */
    let_stuff_happen();

    /* Confirm that both mirrors have all of the writes */
    for (std::map<std::string, std::string>::iterator it = inserter.values_inserted.begin();
            it != inserter.values_inserted.end(); it++) {
        EXPECT_EQ((*it).second, store1->underlying_store.values[(*it).first]);
        EXPECT_EQ((*it).second, store2.underlying_store.values[(*it).first]);
    }
}
TEST(ClusteringBranch, Backfill) {
    run_in_thread_pool_with_broadcaster(&run_backfill_test);
}

}   /* namespace unittest */
