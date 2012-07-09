#include "unittest/gtest.hpp"

#include "clustering/immediate_consistency/branch/broadcaster.hpp"
#include "clustering/immediate_consistency/branch/listener.hpp"
#include "clustering/immediate_consistency/branch/replier.hpp"
#include "clustering/immediate_consistency/query/master.hpp"
#include "clustering/reactor/namespace_interface.hpp"
#include "mock/branch_history_manager.hpp"
#include "mock/clustering_utils.hpp"
#include "mock/dummy_protocol.hpp"
#include "mock/test_cluster_group.hpp"
#include "mock/unittest_utils.hpp"

using mock::dummy_protocol_t;

namespace unittest {

static void run_missing_master_test() {
    /* Set up a cluster so mailboxes can be created */
    mock::simple_mailbox_cluster_t cluster;

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
    mock::run_in_thread_pool(&run_missing_master_test);
}

static void run_read_outdated_test() {
    mock::test_cluster_group_t<dummy_protocol_t> cluster_group(2);

    cluster_group.construct_all_reactors(cluster_group.compile_blueprint("p,s"));

    cluster_group.wait_until_blueprint_is_satisfied("p,s");

    boost::scoped_ptr<cluster_namespace_interface_t<dummy_protocol_t> > namespace_if;
    cluster_group.make_namespace_interface(0, &namespace_if);

    dummy_protocol_t::read_t r;
    r.keys.keys.insert("a");
    cond_t non_interruptor;
    dummy_protocol_t::read_response_t resp = namespace_if->read_outdated(r, &non_interruptor);
    EXPECT_EQ("", resp.values["a"]);
}

TEST(ClusteringNamespaceInterface, ReadOutdated) {
    mock::run_in_thread_pool(&run_read_outdated_test);
}

}   /* namespace unittest */

