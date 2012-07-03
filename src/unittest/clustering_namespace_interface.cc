#include "unittest/gtest.hpp"

#include "clustering/immediate_consistency/branch/broadcaster.hpp"
#include "clustering/immediate_consistency/branch/listener.hpp"
#include "clustering/immediate_consistency/branch/replier.hpp"
#include "clustering/immediate_consistency/query/master.hpp"
#include "clustering/reactor/namespace_interface.hpp"
#include "mock/branch_history_manager.hpp"
#include "mock/dummy_protocol.hpp"
#include "rpc/mailbox/mailbox.hpp"
#include "unittest/clustering_utils.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

static void run_missing_master_test() {
    /* Set up a cluster so mailboxes can be created */
    simple_mailbox_cluster_t cluster;

    /* Set up a reactor directory with no reactors in it */
    std::map<peer_id_t, reactor_business_card_t<dummy_protocol_t> > empty_reactor_directory;
    watchable_variable_t<std::map<peer_id_t, reactor_business_card_t<dummy_protocol_t> > > reactor_directory(empty_reactor_directory);

    /* Set up a namespace dispatcher */
    cluster_namespace_interface_t<dummy_protocol_t> namespace_interface(
        cluster.get_mailbox_manager(),
        reactor_directory.get_watchable()
        );
    namespace_interface.get_initial_ready_signal()->wait_lazily_unordered();

    order_source_t order_source;

    /* Confirm that it throws an exception */
    dummy_protocol_t::read_t r;
    r.keys.keys.insert("a");
    cond_t non_interruptor;
    try {
        namespace_interface.read(r, order_source.check_in("unittest::run_missing_master_test(A)"), &non_interruptor);
        ADD_FAILURE() << "That was supposed to fail.";
    } catch (cannot_perform_query_exc_t e) {
        /* expected */
    }

    dummy_protocol_t::write_t w;
    w.values["a"] = "b";
    try {
        namespace_interface.write(w, order_source.check_in("unittest::run_missing_master_test(B)"), &non_interruptor);
        ADD_FAILURE() << "That was supposed to fail.";
    } catch (cannot_perform_query_exc_t e) {
        /* expected */
    }
}

TEST(ClusteringNamespaceInterface, MissingMaster) {
    run_in_thread_pool(&run_missing_master_test);
}

}   /* namespace unittest */

