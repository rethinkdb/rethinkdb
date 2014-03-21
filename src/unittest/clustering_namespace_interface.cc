// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "clustering/immediate_consistency/branch/broadcaster.hpp"
#include "clustering/immediate_consistency/branch/listener.hpp"
#include "clustering/immediate_consistency/branch/replier.hpp"
#include "clustering/immediate_consistency/query/master.hpp"
#include "clustering/reactor/blueprint.hpp"
#include "clustering/reactor/namespace_interface.hpp"
#include "unittest/branch_history_manager.hpp"
#include "unittest/clustering_utils.hpp"
#include "mock/dummy_protocol.hpp"
#include "unittest/unittest_utils.hpp"
#include "unittest/test_cluster_group.hpp"

using mock::dummy_protocol_t;

namespace unittest {

TPTEST(ClusteringNamespaceInterface, MissingMaster) {
    /* Set up a cluster so mailboxes can be created */
    simple_mailbox_cluster_t cluster;

    /* Set up a reactor directory with no reactors in it */
    std::map<peer_id_t, cow_ptr_t<reactor_business_card_t<dummy_protocol_t> > > empty_reactor_directory;
    watchable_variable_t<std::map<peer_id_t, cow_ptr_t<reactor_business_card_t<dummy_protocol_t> > > > reactor_directory(empty_reactor_directory);

    /* Set up a namespace dispatcher */
    cluster_namespace_interface_t<dummy_protocol_t> namespace_interface(
        cluster.get_mailbox_manager(),
        reactor_directory.get_watchable(),
        NULL); //<-- this should be a valid context by passing null we're assuming this unit test doesn't do anything complicated enough to need it
    namespace_interface.get_initial_ready_signal()->wait_lazily_unordered();

    order_source_t order_source;

    /* Confirm that it throws an exception */
    dummy_protocol_t::read_t r;
    dummy_protocol_t::read_response_t rr;
    r.keys.keys.insert("a");
    cond_t non_interruptor;
    try {
        namespace_interface.read(r, &rr, order_source.check_in("unittest::run_missing_master_test(A)").with_read_mode(), &non_interruptor);
        ADD_FAILURE() << "That was supposed to fail.";
    } catch (const cannot_perform_query_exc_t &e) {
        /* expected */
    }

    dummy_protocol_t::write_t w;
    dummy_protocol_t::write_response_t wr;
    w.values["a"] = "b";
    try {
        namespace_interface.write(w, &wr, order_source.check_in("unittest::run_missing_master_test(B)"), &non_interruptor);
        ADD_FAILURE() << "That was supposed to fail.";
    } catch (const cannot_perform_query_exc_t &e) {
        /* expected */
    }
}

TPTEST(ClusteringNamespaceInterface, ReadOutdated) {
    test_cluster_group_t<dummy_protocol_t> cluster_group(2);

    cluster_group.construct_all_reactors(cluster_group.compile_blueprint("p,s"));

    cluster_group.wait_until_blueprint_is_satisfied("p,s");

    scoped_ptr_t<cluster_namespace_interface_t<dummy_protocol_t> > namespace_if;
    cluster_group.make_namespace_interface(0, &namespace_if);

    dummy_protocol_t::read_t r;
    dummy_protocol_t::read_response_t rr;
    r.keys.keys.insert("a");
    cond_t non_interruptor;
    namespace_if->read_outdated(r, &rr, &non_interruptor);
    EXPECT_EQ("", rr.values["a"]);
}

}   /* namespace unittest */

