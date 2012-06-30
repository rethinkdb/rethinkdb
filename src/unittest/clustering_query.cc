#include "unittest/gtest.hpp"

#include "clustering/immediate_consistency/branch/broadcaster.hpp"
#include "clustering/immediate_consistency/branch/listener.hpp"
#include "clustering/immediate_consistency/branch/replier.hpp"
#include "clustering/immediate_consistency/query/master.hpp"
#include "clustering/immediate_consistency/query/namespace_interface.hpp"
#include "mock/branch_history_manager.hpp"
#include "mock/dummy_protocol.hpp"
#include "rpc/mailbox/mailbox.hpp"
#include "unittest/clustering_utils.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

namespace {

boost::optional<boost::optional<broadcaster_business_card_t<dummy_protocol_t> > > wrap_in_optional(
        const boost::optional<broadcaster_business_card_t<dummy_protocol_t> > &inner) {
    return boost::optional<boost::optional<broadcaster_business_card_t<dummy_protocol_t> > >(inner);
}

}   /* anonymous namespace */

/* The `ReadWrite` test sends some reads and writes to some shards via a
`cluster_namespace_interface_t`. */

static std::map<peer_id_t, std::map<master_id_t, master_business_card_t<dummy_protocol_t> > > wrap_in_peers_map(const std::map<master_id_t, master_business_card_t<dummy_protocol_t> > &peer_value, peer_id_t peer) {
    std::map<peer_id_t, std::map<master_id_t, master_business_card_t<dummy_protocol_t> > > map;
    map.insert(std::make_pair(peer, peer_value));
    return map;
}

static void run_read_write_test() {
    /* Set up a cluster so mailboxes can be created */
    simple_mailbox_cluster_t cluster;

    /* Set up branch history tracker */
    mock::in_memory_branch_history_manager_t<dummy_protocol_t> branch_history_manager;

    boost::scoped_ptr<io_backender_t> io_backender;
    make_io_backender(aio_default, &io_backender);

    /* Set up a branch */
    test_store_t<dummy_protocol_t> initial_store(io_backender.get());
    store_view_t<dummy_protocol_t> *initial_store_ptr = &initial_store.store;
    multistore_ptr_t<dummy_protocol_t> multi_initial_store(&initial_store_ptr, 1);
    cond_t interruptor;
    broadcaster_t<dummy_protocol_t> broadcaster(
        cluster.get_mailbox_manager(),
        &branch_history_manager,
        &multi_initial_store,
        &get_global_perfmon_collection(),
        &interruptor
        );

    watchable_variable_t<boost::optional<broadcaster_business_card_t<dummy_protocol_t> > > broadcaster_metadata_controller(
        boost::optional<broadcaster_business_card_t<dummy_protocol_t> >(broadcaster.get_business_card()));

    listener_t<dummy_protocol_t> initial_listener(
        io_backender.get(),
        cluster.get_mailbox_manager(),
        broadcaster_metadata_controller.get_watchable()->subview(&wrap_in_optional),
        &branch_history_manager,
        &broadcaster,
        &get_global_perfmon_collection(),
        &interruptor
        );

    replier_t<dummy_protocol_t> initial_replier(&initial_listener);

    /* Set up a metadata meeting-place for masters */
    std::map<master_id_t, master_business_card_t<dummy_protocol_t> > initial_master_metadata;
    watchable_variable_t<std::map<master_id_t, master_business_card_t<dummy_protocol_t> > > master_directory(initial_master_metadata);
    mutex_assertion_t master_directory_lock;

    /* Set up a master */
    class : public master_t<dummy_protocol_t>::ack_checker_t {
    public:
        bool is_acceptable_ack_set(const std::set<peer_id_t> &set) {
            return set.size() >= 1;
        }
    } ack_checker;
    master_t<dummy_protocol_t> master(cluster.get_mailbox_manager(), &ack_checker, &master_directory, &master_directory_lock, a_thru_z_region(), &broadcaster);

    /* Set up a namespace dispatcher */
    cluster_namespace_interface_t<dummy_protocol_t> namespace_interface(
        cluster.get_mailbox_manager(),
        master_directory.get_watchable()->subview(boost::bind(&wrap_in_peers_map, _1, cluster.get_connectivity_service()->get_me()))
        );
    namespace_interface.get_initial_ready_signal()->wait_lazily_unordered();

    /* Send some writes to the namespace */
    order_source_t order_source;
    std::map<std::string, std::string> inserter_state;
    test_inserter_t inserter(
        &namespace_interface,
        &dummy_key_gen,
        &order_source,
        "run_read_write_test(clustering_query.cc)/inserter",
        &inserter_state);
    nap(100);
    inserter.stop();

    /* Now send some reads */
    for (std::map<std::string, std::string>::iterator it = inserter.values_inserted->begin();
            it != inserter.values_inserted->end(); it++) {
        dummy_protocol_t::read_t r;
        r.keys.keys.insert((*it).first);
        cond_t interruptor;
        dummy_protocol_t::read_response_t resp = namespace_interface.read(r, order_source.check_in("unittest::run_read_write_test(clustering_query.cc)"), &interruptor);
        EXPECT_EQ((*it).second, resp.values[(*it).first]);
    }
}

TEST(ClusteringNamespace, ReadWrite) {
    run_in_thread_pool(&run_read_write_test);
}

static void run_broadcaster_problem_test() {
    /* Set up a cluster so mailboxes can be created */
    simple_mailbox_cluster_t cluster;

    /* Set up metadata meeting-places */
    mock::in_memory_branch_history_manager_t<dummy_protocol_t> branch_history_manager;

    // io backender.
    boost::scoped_ptr<io_backender_t> io_backender;
    make_io_backender(aio_default, &io_backender);

    /* Set up a branch */
    test_store_t<dummy_protocol_t> initial_store(io_backender.get());
    store_view_t<dummy_protocol_t> *initial_store_ptr = &initial_store.store;
    multistore_ptr_t<dummy_protocol_t> multi_initial_store(&initial_store_ptr, 1);
    cond_t interruptor;
    broadcaster_t<dummy_protocol_t> broadcaster(
        cluster.get_mailbox_manager(),
        &branch_history_manager,
        &multi_initial_store,
        &get_global_perfmon_collection(),
        &interruptor
        );

    watchable_variable_t<boost::optional<boost::optional<broadcaster_business_card_t<dummy_protocol_t> > > > broadcaster_metadata_controller(
        boost::optional<boost::optional<broadcaster_business_card_t<dummy_protocol_t> > >(broadcaster.get_business_card()));

    listener_t<dummy_protocol_t> initial_listener(
        io_backender.get(),
        cluster.get_mailbox_manager(),
        broadcaster_metadata_controller.get_watchable(),
        &branch_history_manager,
        &broadcaster,
        &get_global_perfmon_collection(),
        &interruptor
        );

    replier_t<dummy_protocol_t> initial_replier(&initial_listener);

    /* Set up a metadata meeting-place for masters */
    std::map<master_id_t, master_business_card_t<dummy_protocol_t> > initial_master_metadata;
    watchable_variable_t<std::map<master_id_t, master_business_card_t<dummy_protocol_t> > > master_directory(initial_master_metadata);
    mutex_assertion_t master_directory_lock;

    /* Set up a master. The ack checker is impossible to satisfy, so every
    write will return an error. */
    class : public master_t<dummy_protocol_t>::ack_checker_t {
    public:
        bool is_acceptable_ack_set(const std::set<peer_id_t> &) {
            return false;
        }
    } ack_checker;
    master_t<dummy_protocol_t> master(cluster.get_mailbox_manager(), &ack_checker, &master_directory, &master_directory_lock, a_thru_z_region(), &broadcaster);

    /* Set up a namespace dispatcher */
    cluster_namespace_interface_t<dummy_protocol_t> namespace_interface(
        cluster.get_mailbox_manager(),
        master_directory.get_watchable()->subview(boost::bind(&wrap_in_peers_map, _1, cluster.get_connectivity_service()->get_me()))
        );
    namespace_interface.get_initial_ready_signal()->wait_lazily_unordered();

    order_source_t order_source;

    /* Confirm that it throws an exception */
    dummy_protocol_t::write_t w;
    w.values["a"] = "b";
    cond_t non_interruptor;
    try {
        namespace_interface.write(w, order_source.check_in("unittest::run_broadcaster_problem_test(clustering_query.cc)"), &non_interruptor);
        ADD_FAILURE() << "That was supposed to fail.";
    } catch (cannot_perform_query_exc_t e) {
        /* expected */
    }
}

TEST(ClusteringNamespace, BroadcasterProblem) {
    run_in_thread_pool(&run_broadcaster_problem_test);
}

static void run_missing_master_test() {
    /* Set up a cluster so mailboxes can be created */
    simple_mailbox_cluster_t cluster;

    /* Set up a metadata meeting-place for masters */
    std::map<master_id_t, master_business_card_t<dummy_protocol_t> > initial_master_metadata;
    watchable_variable_t<std::map<master_id_t, master_business_card_t<dummy_protocol_t> > > master_directory(initial_master_metadata);

    /* Set up a namespace dispatcher */
    cluster_namespace_interface_t<dummy_protocol_t> namespace_interface(
        cluster.get_mailbox_manager(),
        master_directory.get_watchable()->subview(boost::bind(&wrap_in_peers_map, _1, cluster.get_connectivity_service()->get_me()))
        );
    namespace_interface.get_initial_ready_signal()->wait_lazily_unordered();

    order_source_t order_source;

    /* Confirm that it throws an exception */
    dummy_protocol_t::read_t r;
    r.keys.keys.insert("a");
    cond_t non_interruptor;
    try {
        namespace_interface.read(r, order_source.check_in("unittest::run_missing_master_test(A)"), &non_interruptor);
        ADD_FAILURE() << "That wasn't supposed to work.";
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

TEST(ClusteringNamespace, MissingMaster) {
    run_in_thread_pool(&run_missing_master_test);
}


}   /* namespace unittest */

