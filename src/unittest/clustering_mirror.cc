#include "unittest/gtest.hpp"
#include "clustering/mirror.hpp"
#include "clustering/mirror_dispatcher.hpp"
#include "unittest/dummy_protocol.hpp"
#include "rpc/metadata/view/controller.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

namespace {

void run_with_dispatcher(boost::function<void(mailbox_cluster_t *, metadata_readwrite_view_t<mirror_dispatcher_metadata_t<dummy_protocol_t> > *, mirror_dispatcher_t<dummy_protocol_t> *)> fun) {

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

    mirror_dispatcher_t<dummy_protocol_t> dispatcher(
        &cluster,
        metadata_controller.get_view());

    fun(&cluster, metadata_controller.get_view(), &dispatcher);
}

void run_in_thread_pool_with_dispatcher(boost::function<void(mailbox_cluster_t *, metadata_readwrite_view_t<mirror_dispatcher_metadata_t<dummy_protocol_t> > *, mirror_dispatcher_t<dummy_protocol_t> *)> fun) {
    run_in_thread_pool(boost::bind(&run_with_dispatcher, fun));
}

}   /* anonymous namespace */

void run_read_write_test(mailbox_cluster_t *cluster,
        metadata_readwrite_view_t<mirror_dispatcher_metadata_t<dummy_protocol_t> > *metadata_view,
        mirror_dispatcher_t<dummy_protocol_t> *dispatcher)
{
    transition_timestamp_t next_timestamp = transition_timestamp_t::first();

    order_source_t order_source;

    /* Set up a mirror */

    dummy_protocol_t::region_t region;
    for (char c = 'a'; c <= 'z'; c++) region.keys.insert(std::string(1, c));

    dummy_protocol_t::store_t store1(region);

    cond_t interruptor;
    mirror_t<dummy_protocol_t> mirror1(&store1, cluster, metadata_view, &interruptor);

    /* Send some writes from the operation dispatcher to the mirror */

    std::map<std::string, std::string> values_inserted;

    for (int i = 0; i < 10; i++) {
        dummy_protocol_t::write_t w;
        std::string key = std::string(1, 'a' + rand() % 26);
        w.values[key] = values_inserted[key] = strprintf("%d", i);
        dispatcher->write(w, next_timestamp, order_source.check_in("unittest"));
        next_timestamp = next_timestamp.next();
    }

    /* Now send some reads */

    for (std::map<std::string, std::string>::iterator it = values_inserted.begin();
            it != values_inserted.end(); it++) {
        dummy_protocol_t::read_t r;
        r.keys.keys.insert((*it).first);
        dummy_protocol_t::read_response_t resp = dispatcher->read(r, order_source.check_in("unittest"));
        EXPECT_EQ((*it).second, resp.values[(*it).first]);
    }
}
TEST(ClusteringMirror, ReadWrite) {
    run_in_thread_pool_with_dispatcher(&run_read_write_test);
}

}   /* namespace unittest */
