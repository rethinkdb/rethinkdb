#include "unittest/gtest.hpp"
#include "clustering/mirror.hpp"
#include "clustering/mirror_dispatcher.hpp"
#include "unittest/dummy_protocol.hpp"
#include "rpc/metadata/view/controller.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

namespace {

void run_with_dispatcher(boost::function<void(mailbox_cluster_t *, boost::shared_ptr<metadata_readwrite_view_t<mirror_dispatcher_metadata_t<dummy_protocol_t> > >, boost::scoped_ptr<mirror_dispatcher_t<dummy_protocol_t> > *)> fun) {

    /* Set up a cluster so mailboxes can be created */

    class dummy_cluster_t : public mailbox_cluster_t {
    public:
        dummy_cluster_t(int port) : mailbox_cluster_t(port) { }
    private:
        void on_utility_message(peer_id_t, std::istream&, boost::function<void()>&) {
            ADD_FAILURE() << "no utility messages should be sent. WTF?";
        }
    } cluster(10000 + rand() % 20000);

    /* Set up a metadata meeting-place */

    mirror_dispatcher_metadata_t<dummy_protocol_t> initial_metadata;
    metadata_view_controller_t<mirror_dispatcher_metadata_t<dummy_protocol_t> > metadata_controller(initial_metadata);

    /* Set up an operation dispatcher */

    boost::scoped_ptr<mirror_dispatcher_t<dummy_protocol_t> > dispatcher(
        new mirror_dispatcher_t<dummy_protocol_t>(
            &cluster,
            metadata_controller.get_view(),
            state_timestamp_t::zero()
        ));

    fun(&cluster, metadata_controller.get_view(), &dispatcher);
}

void run_in_thread_pool_with_dispatcher(boost::function<void(mailbox_cluster_t *, boost::shared_ptr<metadata_readwrite_view_t<mirror_dispatcher_metadata_t<dummy_protocol_t> > >, boost::scoped_ptr<mirror_dispatcher_t<dummy_protocol_t> > *)> fun) {
    run_in_thread_pool(boost::bind(&run_with_dispatcher, fun));
}

/* `let_stuff_happen()` delays for some time to let events occur */
void let_stuff_happen() {
    nap(1000);
}

class inserter_t {

public:
    inserter_t(mirror_dispatcher_t<dummy_protocol_t> *dispatcher, order_source_t *osource) :
        drainer(new auto_drainer_t)
    {
        coro_t::spawn_sometime(boost::bind(&inserter_t::insert_forever,
            this, dispatcher, osource, auto_drainer_t::lock_t(drainer.get())));
    }

    void stop() {
        rassert(drainer);
        drainer.reset();
    }

    std::map<std::string, std::string> values_inserted;

private:
    boost::scoped_ptr<auto_drainer_t> drainer;

    void insert_forever(mirror_dispatcher_t<dummy_protocol_t> *dispatcher, order_source_t *osource, auto_drainer_t::lock_t keepalive) {
        try {
            for (int i = 0; ; i++) {
                if (keepalive.get_drain_signal()->is_pulsed()) throw interrupted_exc_t();
                dummy_protocol_t::write_t w;
                std::string key = std::string(1, 'a' + rand() % 26);
                w.values[key] = values_inserted[key] = strprintf("%d", i);
                dispatcher->write(w, osource->check_in("unittest"));
                nap(10, keepalive.get_drain_signal());
            }
        } catch (interrupted_exc_t) {
            /* Break out of loop */
        }
    }
};

}   /* anonymous namespace */

/* The `ReadWrite` test just sends some reads and writes via the master to a
single mirror. */

void run_read_write_test(mailbox_cluster_t *cluster,
        boost::shared_ptr<metadata_readwrite_view_t<mirror_dispatcher_metadata_t<dummy_protocol_t> > > metadata_view,
        boost::scoped_ptr<mirror_dispatcher_t<dummy_protocol_t> > *dispatcher)
{
    order_source_t order_source;

    /* Set up a mirror */

    dummy_protocol_t::region_t region;
    for (char c = 'a'; c <= 'z'; c++) region.keys.insert(std::string(1, c));

    dummy_protocol_t::store_t store1(region);

    cond_t interruptor;
    mirror_t<dummy_protocol_t> mirror1(&store1, cluster, metadata_view, &interruptor);

    EXPECT_FALSE(mirror1.get_outdated_signal()->is_pulsed());

    /* Send some writes from the operation dispatcher to the mirror */

    std::map<std::string, std::string> values_inserted;

    for (int i = 0; i < 10; i++) {

        dummy_protocol_t::write_t w;
        std::string key = std::string(1, 'a' + rand() % 26);
        w.values[key] = values_inserted[key] = strprintf("%d", i);

        (*dispatcher)->write(w, order_source.check_in("unittest"));
    }

    /* Now send some reads */

    for (std::map<std::string, std::string>::iterator it = values_inserted.begin();
            it != values_inserted.end(); it++) {
        dummy_protocol_t::read_t r;
        r.keys.keys.insert((*it).first);
        dummy_protocol_t::read_response_t resp = (*dispatcher)->read(r, order_source.check_in("unittest"));
        EXPECT_EQ((*it).second, resp.values[(*it).first]);
    }
}
TEST(ClusteringMirror, ReadWrite) {
    run_in_thread_pool_with_dispatcher(&run_read_write_test);
}

/* The `Backfill` test starts up a node with one mirror, inserts some data, and
then adds another mirror. */

void run_backfill_test(mailbox_cluster_t *cluster,
        boost::shared_ptr<metadata_readwrite_view_t<mirror_dispatcher_metadata_t<dummy_protocol_t> > > metadata_view,
        boost::scoped_ptr<mirror_dispatcher_t<dummy_protocol_t> > *dispatcher)
{
    order_source_t order_source;

    /* Set up a mirror */

    dummy_protocol_t::region_t region;
    for (char c = 'a'; c <= 'z'; c++) region.keys.insert(std::string(1, c));

    dummy_protocol_t::store_t store1(region);

    cond_t mirror1_interruptor;
    mirror_t<dummy_protocol_t> mirror1(&store1, cluster, metadata_view, &mirror1_interruptor);

    EXPECT_FALSE(mirror1.get_outdated_signal()->is_pulsed());

    /* Start sending operations to the dispatcher */

    inserter_t inserter(dispatcher->get(), &order_source);

    nap(100);

    /* Set up a second mirror */

    dummy_protocol_t::store_t store2(region);
    cond_t mirror2_interruptor;
    mirror_t<dummy_protocol_t> mirror2(&store2, cluster, metadata_view, mirror1.get_mirror_id(), &mirror2_interruptor);

    EXPECT_FALSE(mirror1.get_outdated_signal()->is_pulsed());

    nap(100);

    /* Stop the inserter */

    inserter.stop();
    /* Let any lingering writes finish */
    let_stuff_happen();

    /* Confirm that both mirrors have all of the writes */

    for (std::map<std::string, std::string>::iterator it = inserter.values_inserted.begin();
            it != inserter.values_inserted.end(); it++) {
        EXPECT_EQ((*it).second, store1.values[(*it).first]);
        EXPECT_EQ((*it).second, store2.values[(*it).first]);
    }
}
TEST(ClusteringMirror, Backfill) {
    run_in_thread_pool_with_dispatcher(&run_backfill_test);
}

/* The `Outdated` test makes sure that mirror behave reasonably when the master
is lost. */

void run_outdated_test(mailbox_cluster_t *cluster,
        boost::shared_ptr<metadata_readwrite_view_t<mirror_dispatcher_metadata_t<dummy_protocol_t> > > metadata_view,
        boost::scoped_ptr<mirror_dispatcher_t<dummy_protocol_t> > *dispatcher)
{
    order_source_t order_source;

    /* Set up a mirror */

    dummy_protocol_t::region_t region;
    for (char c = 'a'; c <= 'z'; c++) region.keys.insert(std::string(1, c));
    dummy_protocol_t::store_t store1(region);
    cond_t mirror1_interruptor;
    mirror_t<dummy_protocol_t> mirror1(&store1, cluster, metadata_view, &mirror1_interruptor);
    EXPECT_FALSE(mirror1.get_outdated_signal()->is_pulsed());

    /* Start sending operations to the dispatcher */

    inserter_t inserter(dispatcher->get(), &order_source);

    nap(100);

    /* Stop the inserter and destroy the dispatcher */

    inserter.stop();
    dispatcher->reset();
    let_stuff_happen();

    /* The mirror should notice the master is dead */
    EXPECT_TRUE(mirror1.get_outdated_signal()->is_pulsed());

    /* Bring up a second mirror */

    dummy_protocol_t::store_t store2(region);
    cond_t mirror2_interruptor;
    mirror_t<dummy_protocol_t> mirror2(&store2, cluster, metadata_view, mirror1.get_mirror_id(), &mirror2_interruptor);
    EXPECT_TRUE(mirror2.get_outdated_signal()->is_pulsed());

    /* Confirm that the backfill still happened */

    for (std::map<std::string, std::string>::iterator it = inserter.values_inserted.begin();
            it != inserter.values_inserted.end(); it++) {
        EXPECT_EQ((*it).second, store1.values[(*it).first]);
        EXPECT_EQ((*it).second, store2.values[(*it).first]);
    }
}
TEST(ClusteringMirror, Outdated) {
    run_in_thread_pool_with_dispatcher(&run_outdated_test);
}

/* The `BackfillerLost` test checks what happens if the mirror that's supposed
to provide a backfill dies during the backfill. */

void kill_mirror_eventually(boost::scoped_ptr<mirror_t<dummy_protocol_t> > *to_destroy, UNUSED auto_drainer_t::lock_t) {
    nap(20);
    to_destroy->reset();
}

void run_backfiller_lost_test(mailbox_cluster_t *cluster,
        boost::shared_ptr<metadata_readwrite_view_t<mirror_dispatcher_metadata_t<dummy_protocol_t> > > metadata_view,
        boost::scoped_ptr<mirror_dispatcher_t<dummy_protocol_t> > *dispatcher)
{
    order_source_t order_source;

    /* Set up a mirror */

    dummy_protocol_t::region_t region;
    for (char c = 'a'; c <= 'z'; c++) region.keys.insert(std::string(1, c));
    dummy_protocol_t::store_t store1(region);
    cond_t mirror1_interruptor;
    boost::scoped_ptr<mirror_t<dummy_protocol_t> > mirror1(
        new mirror_t<dummy_protocol_t>(
            &store1, cluster, metadata_view, &mirror1_interruptor
        ));
    EXPECT_FALSE(mirror1->get_outdated_signal()->is_pulsed());

    /* Start sending operations to the dispatcher */

    inserter_t inserter(dispatcher->get(), &order_source);
    nap(100);

    /* Schedule the first mirror to be destroyed (but make a note of its ID
    first) */

    mirror_id_t mirror1_id = mirror1->get_mirror_id();

    auto_drainer_t drainer;
    coro_t::spawn_sometime(boost::bind(&kill_mirror_eventually, &mirror1, auto_drainer_t::lock_t(&drainer)));

    /* Start bringing up the second mirror. We gamble that the backfill will
    take longer than `kill_mirror_eventually`'s timer. */

    dummy_protocol_t::store_t store2(region);
    cond_t mirror2_interruptor;
    boost::scoped_ptr<mirror_t<dummy_protocol_t> > mirror2;
    EXPECT_THROW(
        mirror2.reset(new mirror_t<dummy_protocol_t>(
            &store2, cluster, metadata_view, mirror1_id, &mirror2_interruptor)),
        resource_lost_exc_t
        );
}
TEST(ClusteringMirror, BackfillerLost) {
    run_in_thread_pool_with_dispatcher(&run_backfiller_lost_test);
}

}   /* namespace unittest */
