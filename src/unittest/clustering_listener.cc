#if 0

#include "unittest/gtest.hpp"
#include "clustering/immediate_consistency/listener.hpp"
#include "clustering/immediate_consistency/master.hpp"
#include "unittest/dummy_protocol.hpp"
#include "rpc/metadata/view/controller.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

namespace {

void run_with_master(
        boost::function<void(
            mailbox_cluster_t *,
            boost::shared_ptr<metadata_readwrite_view_t<namespace_metadata_t<dummy_protocol_t> > >,
            boost::scoped_ptr<master_t<dummy_protocol_t> > *,
            dummy_underlying_store_t *,
            boost::scoped_ptr<listener_t<dummy_protocol_t> > *
            )> fun)
{
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

    namespace_metadata_t<dummy_protocol_t> initial_metadata;
    metadata_view_controller_t<namespace_metadata_t<dummy_protocol_t> > metadata_controller(initial_metadata);

    /* Set up a master and initial listener */

    dummy_protocol_t::region_t region;
    for (char c = 'a'; c <= 'z'; c++) {
        region.keys.insert(std::string(&c, 1));
    }

    dummy_underlying_store_t underlying_store(region);
    dummy_store_view_t store(&underlying_store, region);

    cond_t *interruptor;
    boost::scoped_ptr<listener_t<dummy_protocol_t> > initial_listener;
    boost::scoped_ptr<master_t<dummy_protocol_t> > master(
        new master_t<dummy_protocol_t>(
            &cluster,
            metadata_controller.get_view(),
            &store,
            &interruptor,
            &initial_listener
        ));

    fun(&cluster, metadata_controller.get_view(), &master, &underlying_store, &initial_listener);
}

void run_in_thread_pool_with_master(
        boost::function<void(
            mailbox_cluster_t *,
            boost::shared_ptr<metadata_readwrite_view_t<namespace_metadata_t<dummy_protocol_t> > >,
            boost::scoped_ptr<master_t<dummy_protocol_t> > *,
            dummy_underlying_store_t *,
            boost::scoped_ptr<listener_t<dummy_protocol_t> > *
            )> fun)
{
    run_in_thread_pool(boost::bind(&run_with_master, fun));
}

/* `let_stuff_happen()` delays for some time to let events occur */
void let_stuff_happen() {
    nap(1000);
}

class inserter_t {

public:
    inserter_t(master_t<dummy_protocol_t> *master, order_source_t *osource) :
        drainer(new auto_drainer_t)
    {
        coro_t::spawn_sometime(boost::bind(&inserter_t::insert_forever,
            this, master, osource, auto_drainer_t::lock_t(drainer.get())));
    }

    void stop() {
        rassert(drainer);
        drainer.reset();
    }

    std::map<std::string, std::string> values_inserted;

private:
    boost::scoped_ptr<auto_drainer_t> drainer;

    void insert_forever(master_t<dummy_protocol_t> *master, order_source_t *osource, auto_drainer_t::lock_t keepalive) {
        try {
            for (int i = 0; ; i++) {
                if (keepalive.get_drain_signal()->is_pulsed()) throw interrupted_exc_t();
                dummy_protocol_t::write_t w;
                std::string key = std::string(1, 'a' + rand() % 26);
                w.values[key] = values_inserted[key] = strprintf("%d", i);
                master->write(w, osource->check_in("unittest"));
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
        boost::shared_ptr<metadata_readwrite_view_t<namespace_metadata_t<dummy_protocol_t> > > metadata_view,
        UNUSED dummy_underlying_store_t *store,
        boost::scoped_ptr<master_t<dummy_protocol_t> > *master,
        boost::scoped_ptr<listener_t<dummy_protocol_t> > *initial_listener)
{
    /* Set up a replier so the master can handle operations */
    EXPECT_FALSE((*initial_listener)->get_outdated_signal()->is_pulsed());
    replier_t<dummy_protocol_t> replier(initial_listener->get());

    order_source_t order_source;

    /* Send some writes via the master to the mirror */
    std::map<std::string, std::string> values_inserted;
    for (int i = 0; i < 10; i++) {
        dummy_protocol_t::write_t w;
        std::string key = std::string(1, 'a' + rand() % 26);
        w.values[key] = values_inserted[key] = strprintf("%d", i);
        (*master)->write(w, order_source.check_in("unittest"));
    }

    /* Now send some reads */
    for (std::map<std::string, std::string>::iterator it = values_inserted.begin();
            it != values_inserted.end(); it++) {
        dummy_protocol_t::read_t r;
        r.keys.keys.insert((*it).first);
        dummy_protocol_t::read_response_t resp = (*master)->read(r, order_source.check_in("unittest"));
        EXPECT_EQ((*it).second, resp.values[(*it).first]);
    }
}
TEST(ClusteringListener, ReadWrite) {
    run_in_thread_pool_with_master(&run_read_write_test);
}

/* The `Backfill` test starts up a node with one mirror, inserts some data, and
then adds another mirror. */

void run_backfill_test(mailbox_cluster_t *cluster,
        boost::shared_ptr<metadata_readwrite_view_t<namespace_metadata_t<dummy_protocol_t> > > metadata_view,
        dummy_underlying_store_t *store,
        boost::scoped_ptr<master_t<dummy_protocol_t> > *master,
        boost::scoped_ptr<listener_t<dummy_protocol_t> > *initial_listener)
{
    /* Set up a replier so the master can handle operations */
    EXPECT_FALSE((*initial_listener)->get_outdated_signal()->is_pulsed());
    replier_t<dummy_protocol_t> replier(initial_listener->get());

    order_source_t order_source;

    /* Start sending operations to the dispatcher */
    inserter_t inserter(dispatcher->get(), &order_source);
    nap(100);

    /* Set up a second mirror */
    dummy_protocol_t::region_t region;
    for (char c = 'a'; c <= 'z'; c++) region.keys.insert(std::string(1, c));
    dummy_underlying_store_t store2(region);
    dummy_store_view_t store2_view(&store2, region);
    cond_t interruptor;
    listener_t<dummy_protocol_t> listener2(cluster, metadata_view, &store2_view, replier.get_backfiller_id(), &interruptor);

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
        EXPECT_EQ((*it).second, store->values[(*it).first]);
        EXPECT_EQ((*it).second, store2.values[(*it).first]);
    }
}
TEST(ClusteringListener, Backfill) {
    run_in_thread_pool_with_master(&run_backfill_test);
}

}   /* namespace unittest */

#endif
