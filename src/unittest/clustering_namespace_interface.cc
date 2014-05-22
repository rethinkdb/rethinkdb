// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "clustering/immediate_consistency/branch/broadcaster.hpp"
#include "clustering/immediate_consistency/branch/listener.hpp"
#include "clustering/immediate_consistency/branch/replier.hpp"
#include "clustering/immediate_consistency/query/master.hpp"
#include "clustering/reactor/blueprint.hpp"
#include "clustering/reactor/namespace_interface.hpp"
#include "rdb_protocol/protocol.hpp"
#include "unittest/branch_history_manager.hpp"
#include "unittest/clustering_utils.hpp"
#include "unittest/unittest_utils.hpp"
#include "unittest/test_cluster_group.hpp"

namespace unittest {

TPTEST(ClusteringNamespaceInterface, MissingMaster) {
    /* Set up a cluster so mailboxes can be created */
    simple_mailbox_cluster_t cluster;
    std::map<key_range_t, machine_id_t> region_to_primary;

    /* Set up a reactor directory with no reactors in it */
    std::map<peer_id_t, cow_ptr_t<reactor_business_card_t> > empty_reactor_directory;
    watchable_variable_t<std::map<peer_id_t, cow_ptr_t<reactor_business_card_t> > > reactor_directory(empty_reactor_directory);

    /* Set up a namespace dispatcher */
    cluster_namespace_interface_t namespace_interface(
        cluster.get_mailbox_manager(),
        &region_to_primary,
        reactor_directory.get_watchable(),
        NULL); //<-- this should be a valid context by passing null we're assuming this unit test doesn't do anything complicated enough to need it
    namespace_interface.get_initial_ready_signal()->wait_lazily_unordered();

    order_source_t order_source;

    /* Confirm that it throws an exception */
    read_t r = mock_read("a");
    read_response_t rr;
    cond_t non_interruptor;
    try {
        namespace_interface.read(r, &rr, order_source.check_in("unittest::run_missing_master_test(A)").with_read_mode(), &non_interruptor);
        ADD_FAILURE() << "That was supposed to fail.";
    } catch (const cannot_perform_query_exc_t &e) {
        /* expected */
    }

    write_t w = mock_overwrite("a", "b");
    write_response_t wr;
    try {
        namespace_interface.write(w, &wr, order_source.check_in("unittest::run_missing_master_test(B)"), &non_interruptor);
        ADD_FAILURE() << "That was supposed to fail.";
    } catch (const cannot_perform_query_exc_t &e) {
        /* expected */
    }
}

TPTEST(ClusteringNamespaceInterface, ReadOutdated) {
    test_cluster_group_t cluster_group(2);

    cluster_group.construct_all_reactors(cluster_group.compile_blueprint("p,s"));

    cluster_group.wait_until_blueprint_is_satisfied("p,s");

    scoped_ptr_t<cluster_namespace_interface_t> namespace_if;
    cluster_group.make_namespace_interface(0, &namespace_if);

    read_t r = mock_read("a");
    read_response_t rr;
    cond_t non_interruptor;
    namespace_if->read_outdated(r, &rr, &non_interruptor);
    EXPECT_EQ("", mock_parse_read_response(rr));
}

}   /* namespace unittest */

