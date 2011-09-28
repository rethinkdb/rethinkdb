/* This test is broken until the mirror/listener split is done */
#if 0

#include "unittest/gtest.hpp"

#include "clustering/namespace_interface.hpp"
#include "clustering/namespace_metadata.hpp"
#include "clustering/master.hpp"
#include "clustering/mirror.hpp"
#include "rpc/mailbox/mailbox.hpp"
#include "rpc/metadata/view/controller.hpp"
#include "unittest/dummy_protocol.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

namespace {

/* `let_stuff_happen()` delays for some time to let events occur */
void let_stuff_happen() {
    nap(1000);
}

class inserter_t {

public:
    inserter_t(namespace_interface_t<dummy_protocol_t> *interface, order_source_t *osource) :
        drainer(new auto_drainer_t)
    {
        coro_t::spawn_sometime(boost::bind(&inserter_t::insert_forever,
            this, interface, osource, auto_drainer_t::lock_t(drainer.get())));
    }

    void stop() {
        rassert(drainer);
        drainer.reset();
    }

    std::map<std::string, std::string> values_inserted;

private:
    boost::scoped_ptr<auto_drainer_t> drainer;

    void insert_forever(namespace_interface_t<dummy_protocol_t> *interface, order_source_t *osource, auto_drainer_t::lock_t keepalive) {
        try {
            for (int i = 0; ; i++) {

                if (keepalive.get_drain_signal()->is_pulsed()) throw interrupted_exc_t();

                dummy_protocol_t::write_t w;
                std::string key = std::string(1, 'a' + rand() % 26);
                w.values[key] = values_inserted[key] = strprintf("%d", i);

                cond_t interruptor;
                interface->write(w, osource->check_in("unittest"), &interruptor);

                nap(10, keepalive.get_drain_signal());
            }
        } catch (interrupted_exc_t) {
            /* Break out of loop */
        }
    }
};

}   /* anonymous namespace */

/* The `Write` test sends some reads and writes to some shards via a
`cluster_namespace_interface_t`. */

static void run_read_write_test() {

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

    /* Set up a branch */

    dummy_protocol_t::region_t region;
    for (char c = 'a'; c <= 'z'; c++) region.keys.insert(std::string(1, c));

    dummy_protocol_t::store_t store1(region);

    cond_t master_interruptor;

    mirror_t<dummy_protocol_t> *mirror_raw_pointer;
    master_t<dummy_protocol_t> master(&cluster, &store1, metadata_controller.get_view(), &master_interruptor, &mirror_raw_pointer);
    EXPECT_TRUE(mirror_raw_pointer != NULL);
    boost::scoped_ptr<mirror_t<dummy_protocol_t> > mirror(mirror_raw_pointer);

    EXPECT_FALSE(mirror->get_outdated_signal()->is_pulsed());

    /* Set up a namespace dispatcher */

    cluster_namespace_interface_t<dummy_protocol_t> namespace_interface(&cluster, metadata_controller.get_view());

    /* Send some writes to the namespace */

    order_source_t order_source;

    inserter_t inserter(&namespace_interface, &order_source);
    nap(100);
    inserter.stop();

    /* Now send some reads */

    for (std::map<std::string, std::string>::iterator it = inserter.values_inserted.begin();
            it != inserter.values_inserted.end(); it++) {
        dummy_protocol_t::read_t r;
        r.keys.keys.insert((*it).first);
        cond_t interruptor;
        dummy_protocol_t::read_response_t resp = namespace_interface.read(r, order_source.check_in("unittest"), &interruptor);
        EXPECT_EQ((*it).second, resp.values[(*it).first]);
    }
}
TEST(ClusteringNamespace, ReadWrite) {
    run_in_thread_pool(&run_read_write_test);
}

}   /* namespace unittest */

#endif
