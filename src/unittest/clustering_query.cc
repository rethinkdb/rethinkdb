#include "unittest/gtest.hpp"

#include "clustering/immediate_consistency/branch/broadcaster.hpp"
#include "clustering/immediate_consistency/branch/listener.hpp"
#include "clustering/immediate_consistency/branch/replier.hpp"
#include "clustering/immediate_consistency/query/master.hpp"
#include "clustering/immediate_consistency/query/namespace_interface.hpp"
#include "rpc/mailbox/mailbox.hpp"
#include "unittest/clustering_utils.hpp"
#include "unittest/dummy_metadata_controller.hpp"
#include "unittest/dummy_protocol.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

namespace {

/* `let_stuff_happen()` delays for some time to let events occur */
void let_stuff_happen() {
    nap(1000);
}

}   /* anonymous namespace */

/* The `Write` test sends some reads and writes to some shards via a
`cluster_namespace_interface_t`. */

static void run_read_write_test() {

    /* Set up a cluster so mailboxes can be created */
    simple_mailbox_cluster_t cluster;

    /* Set up a metadata meeting-place for branches */
    namespace_branch_metadata_t<dummy_protocol_t> initial_branch_metadata;
    dummy_metadata_controller_t<namespace_branch_metadata_t<dummy_protocol_t> > branch_metadata_controller(initial_branch_metadata);

    /* Set up a branch */
    test_store_t initial_store;
    cond_t interruptor;
    boost::scoped_ptr<listener_t<dummy_protocol_t> > initial_listener;
    broadcaster_t<dummy_protocol_t> broadcaster(&cluster, branch_metadata_controller.get_view(), &initial_store.store, &interruptor, &initial_listener);
    replier_t<dummy_protocol_t> initial_replier(&cluster, branch_metadata_controller.get_view(), initial_listener.get());

    /* Set up a metadata meeting-place for masters */
    namespace_master_metadata_t<dummy_protocol_t> initial_master_metadata;
    dummy_metadata_controller_t<namespace_master_metadata_t<dummy_protocol_t> > master_metadata_controller(initial_master_metadata);

    /* Set up a master */
    master_t<dummy_protocol_t> master(&cluster, master_metadata_controller.get_view(), a_thru_z_region(), &broadcaster);

    /* Set up a namespace dispatcher */
    cluster_namespace_interface_t<dummy_protocol_t> namespace_interface(&cluster, master_metadata_controller.get_view());

    /* Send some writes to the namespace */
    order_source_t order_source;
    inserter_t inserter(
        boost::bind(&namespace_interface_t<dummy_protocol_t>::write, &namespace_interface, _1, _2, _3),
        &order_source);
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

