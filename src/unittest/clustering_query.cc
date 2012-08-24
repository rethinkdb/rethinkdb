#include "unittest/gtest.hpp"

#include "clustering/immediate_consistency/branch/broadcaster.hpp"
#include "clustering/immediate_consistency/branch/listener.hpp"
#include "clustering/immediate_consistency/branch/replier.hpp"
#include "clustering/immediate_consistency/query/master.hpp"
#include "clustering/immediate_consistency/query/master_access.hpp"
#include "mock/branch_history_manager.hpp"
#include "mock/clustering_utils.hpp"
#include "mock/dummy_protocol.hpp"
#include "mock/unittest_utils.hpp"

using mock::dummy_protocol_t;

namespace unittest {

namespace {

boost::optional<boost::optional<broadcaster_business_card_t<dummy_protocol_t> > > wrap_in_optional(
        const boost::optional<broadcaster_business_card_t<dummy_protocol_t> > &inner) {
    return boost::optional<boost::optional<broadcaster_business_card_t<dummy_protocol_t> > >(inner);
}

}   /* anonymous namespace */

/* The `ReadWrite` test sends some reads and writes to some shards via a
`master_access_t`. */

static void run_read_write_test() {
    order_source_t order_source;

    /* Set up a cluster so mailboxes can be created */
    mock::simple_mailbox_cluster_t cluster;

    /* Set up branch history tracker */
    mock::in_memory_branch_history_manager_t<dummy_protocol_t> branch_history_manager;

    scoped_ptr_t<io_backender_t> io_backender;
    make_io_backender(aio_default, &io_backender);

    /* Set up a branch */
    mock::test_store_t<dummy_protocol_t> initial_store(io_backender.get(), &order_source);
    store_view_t<dummy_protocol_t> *initial_store_ptr = &initial_store.store;
    dummy_protocol_t::context_t ctx;
    multistore_ptr_t<dummy_protocol_t> multi_initial_store(&initial_store_ptr, 1, &ctx); //null ctx nothing copmlicated can happen
    cond_t interruptor;
    broadcaster_t<dummy_protocol_t> broadcaster(cluster.get_mailbox_manager(),
                                                &branch_history_manager,
                                                &multi_initial_store,
                                                &get_global_perfmon_collection(),
                                                &order_source,
                                                &interruptor);

    watchable_variable_t<boost::optional<broadcaster_business_card_t<dummy_protocol_t> > > broadcaster_metadata_controller(
        boost::optional<broadcaster_business_card_t<dummy_protocol_t> >(broadcaster.get_business_card()));

    listener_t<dummy_protocol_t> initial_listener(
        io_backender.get(),
        cluster.get_mailbox_manager(),
        broadcaster_metadata_controller.get_watchable()->subview(&wrap_in_optional),
        &branch_history_manager,
        &broadcaster,
        &get_global_perfmon_collection(),
        &interruptor,
        &order_source);

    replier_t<dummy_protocol_t> initial_replier(&initial_listener, cluster.get_mailbox_manager(), &branch_history_manager);

    /* Set up a master */
    class : public master_t<dummy_protocol_t>::ack_checker_t {
    public:
        bool is_acceptable_ack_set(const std::set<peer_id_t> &set) {
            return set.size() >= 1;
        }
    } ack_checker;
    master_t<dummy_protocol_t> master(cluster.get_mailbox_manager(), &ack_checker, mock::a_thru_z_region(), &broadcaster);

    /* Set up a master access */
    watchable_variable_t<boost::optional<boost::optional<master_business_card_t<dummy_protocol_t> > > > master_directory_view(
        boost::make_optional(boost::make_optional(master.get_business_card())));
    cond_t non_interruptor;
    master_access_t<dummy_protocol_t> master_access(
        cluster.get_mailbox_manager(),
        master_directory_view.get_watchable(),
        &non_interruptor);

    /* Send some writes to the namespace */
    std::map<std::string, std::string> inserter_state;
    mock::test_inserter_t inserter(
        &master_access,
        &mock::dummy_key_gen,
        &order_source,
        "run_read_write_test(clustering_query.cc)/inserter",
        &inserter_state);
    nap(100);
    inserter.stop();

    /* Now send some reads */
    for (std::map<std::string, std::string>::iterator it = inserter.values_inserted->begin();
            it != inserter.values_inserted->end(); it++) {
        dummy_protocol_t::read_t r;
        dummy_protocol_t::read_response_t rr;
        r.keys.keys.insert((*it).first);
        // TODO: What's with this fake interruptor?
        cond_t fake_interruptor;
        fifo_enforcer_sink_t::exit_read_t read_token;
        master_access.new_read_token(&read_token);
        master_access.read(r, &rr,
                           order_source.check_in("unittest::run_read_write_test(clustering_query.cc)").with_read_mode(),
                           &read_token,
                           &fake_interruptor);
        EXPECT_EQ((*it).second, rr.values[(*it).first]);
    }
}

TEST(ClusteringQuery, ReadWrite) {
    mock::run_in_thread_pool(&run_read_write_test);
}

static void run_broadcaster_problem_test() {
    order_source_t order_source;

    /* Set up a cluster so mailboxes can be created */
    mock::simple_mailbox_cluster_t cluster;

    /* Set up metadata meeting-places */
    mock::in_memory_branch_history_manager_t<dummy_protocol_t> branch_history_manager;

    // io backender.
    scoped_ptr_t<io_backender_t> io_backender;
    make_io_backender(aio_default, &io_backender);

    /* Set up a branch */
    mock::test_store_t<dummy_protocol_t> initial_store(io_backender.get(), &order_source);
    store_view_t<dummy_protocol_t> *initial_store_ptr = &initial_store.store;
    dummy_protocol_t::context_t ctx;
    multistore_ptr_t<dummy_protocol_t> multi_initial_store(&initial_store_ptr, 1, &ctx); //null ctx nothing complicated
    cond_t interruptor;
    broadcaster_t<dummy_protocol_t> broadcaster(cluster.get_mailbox_manager(),
                                                &branch_history_manager,
                                                &multi_initial_store,
                                                &get_global_perfmon_collection(),
                                                &order_source,
                                                &interruptor);

    watchable_variable_t<boost::optional<boost::optional<broadcaster_business_card_t<dummy_protocol_t> > > > broadcaster_metadata_controller(
        boost::optional<boost::optional<broadcaster_business_card_t<dummy_protocol_t> > >(broadcaster.get_business_card()));

    listener_t<dummy_protocol_t> initial_listener(
        io_backender.get(),
        cluster.get_mailbox_manager(),
        broadcaster_metadata_controller.get_watchable(),
        &branch_history_manager,
        &broadcaster,
        &get_global_perfmon_collection(),
        &interruptor,
        &order_source);

    replier_t<dummy_protocol_t> initial_replier(&initial_listener, cluster.get_mailbox_manager(), &branch_history_manager);

    /* Set up a master. The ack checker is impossible to satisfy, so every
    write will return an error. */
    class : public master_t<dummy_protocol_t>::ack_checker_t {
    public:
        bool is_acceptable_ack_set(const std::set<peer_id_t> &) {
            return false;
        }
    } ack_checker;
    master_t<dummy_protocol_t> master(cluster.get_mailbox_manager(), &ack_checker, mock::a_thru_z_region(), &broadcaster);

    /* Set up a master access */
    watchable_variable_t<boost::optional<boost::optional<master_business_card_t<dummy_protocol_t> > > > master_directory_view(
        boost::make_optional(boost::make_optional(master.get_business_card())));
    cond_t non_interruptor_2;
    master_access_t<dummy_protocol_t> master_access(
        cluster.get_mailbox_manager(),
        master_directory_view.get_watchable(),
        &non_interruptor_2);

    /* Confirm that it throws an exception */
    dummy_protocol_t::write_t w;
    dummy_protocol_t::write_response_t wr;
    w.values["a"] = "b";
    cond_t non_interruptor;
    fifo_enforcer_sink_t::exit_write_t write_token;
    master_access.new_write_token(&write_token);
    try {
        master_access.write(w, &wr,
            order_source.check_in("unittest::run_broadcaster_problem_test(clustering_query.cc)"),
            &write_token,
            &non_interruptor);
        ADD_FAILURE() << "That was supposed to fail.";
    } catch (cannot_perform_query_exc_t e) {
        /* expected */
    }
}

TEST(ClusteringQuery, BroadcasterProblem) {
    mock::run_in_thread_pool(&run_broadcaster_problem_test);
}

}   /* namespace unittest */

