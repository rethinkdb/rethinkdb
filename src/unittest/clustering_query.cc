// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "clustering/immediate_consistency/branch/broadcaster.hpp"
#include "clustering/immediate_consistency/branch/listener.hpp"
#include "clustering/immediate_consistency/branch/replier.hpp"
#include "clustering/immediate_consistency/query/master.hpp"
#include "clustering/immediate_consistency/query/master_access.hpp"
#include "unittest/branch_history_manager.hpp"
#include "unittest/clustering_utils.hpp"
#include "rdb_protocol/protocol.hpp"
#include "unittest/mock_store.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

namespace {

boost::optional<boost::optional<broadcaster_business_card_t> > wrap_in_optional(
        const boost::optional<broadcaster_business_card_t> &inner) {
    return boost::optional<boost::optional<broadcaster_business_card_t> >(inner);
}

}   /* anonymous namespace */

/* The `ReadWrite` test sends some reads and writes to some shards via a
`master_access_t`. */

static void run_read_write_test() {
    order_source_t order_source;

    /* Set up a cluster so mailboxes can be created */
    simple_mailbox_cluster_t cluster;

    /* Set up branch history tracker */
    in_memory_branch_history_manager_t branch_history_manager;

    io_backender_t io_backender(file_direct_io_mode_t::buffered_desired);

    /* Set up a branch */
    mock_store_t initial_store((binary_blob_t(version_range_t(version_t::zero()))));

    rdb_context_t rdb_context;

    cond_t interruptor;
    broadcaster_t broadcaster(cluster.get_mailbox_manager(),
                              &rdb_context,
                              &branch_history_manager,
                              &initial_store,
                              &get_global_perfmon_collection(),
                              &order_source,
                              &interruptor);

    watchable_variable_t<boost::optional<broadcaster_business_card_t> > broadcaster_metadata_controller(
        boost::optional<broadcaster_business_card_t>(broadcaster.get_business_card()));

    listener_t initial_listener(
        base_path_t("."),
        &io_backender,
        cluster.get_mailbox_manager(),
        generate_uuid(),
        broadcaster_metadata_controller.get_watchable()->subview(&wrap_in_optional),
        &branch_history_manager,
        &broadcaster,
        &get_global_perfmon_collection(),
        &interruptor,
        &order_source);

    replier_t initial_replier(&initial_listener, cluster.get_mailbox_manager(), &branch_history_manager);

    /* Set up a master */
    fake_ack_checker_t ack_checker(1);
    master_t master(cluster.get_mailbox_manager(), &ack_checker, region_t::universe(), &broadcaster);

    /* Set up a master access */
    watchable_variable_t<boost::optional<boost::optional<master_business_card_t> > > master_directory_view(
        boost::make_optional(boost::make_optional(master.get_business_card())));
    cond_t non_interruptor;
    master_access_t master_access(
        cluster.get_mailbox_manager(),
        master_directory_view.get_watchable(),
        &non_interruptor);

    /* Send some writes to the namespace */
    std::map<std::string, std::string> inserter_state;
    test_inserter_t inserter(
        &master_access,
        &dummy_key_gen,
        &order_source,
        "run_read_write_test(clustering_query.cc)/inserter",
        &inserter_state);
    nap(100);
    inserter.stop();

    /* Now send some reads */
    for (std::map<std::string, std::string>::iterator it = inserter.values_inserted->begin();
            it != inserter.values_inserted->end(); it++) {
        read_t r = mock_read(it->first);
        read_response_t rr;
        cond_t fake_interruptor;
        fifo_enforcer_sink_t::exit_read_t read_token;
        master_access.new_read_token(&read_token);
        master_access.read(r, &rr,
                           order_source.check_in("unittest::run_read_write_test(clustering_query.cc)").with_read_mode(),
                           &read_token,
                           &fake_interruptor);
        EXPECT_EQ(it->second, mock_parse_read_response(rr));
    }
}

TEST(ClusteringQuery, ReadWrite) {
    unittest::run_in_thread_pool(&run_read_write_test);
}

static void run_broadcaster_problem_test() {
    order_source_t order_source;

    /* Set up a cluster so mailboxes can be created */
    simple_mailbox_cluster_t cluster;

    /* Set up metadata meeting-places */
    in_memory_branch_history_manager_t branch_history_manager;

    // io backender.
    io_backender_t io_backender(file_direct_io_mode_t::buffered_desired);

    /* Set up a branch */
    mock_store_t initial_store((binary_blob_t(version_range_t(version_t::zero()))));

    rdb_context_t rdb_context;

    cond_t interruptor;
    broadcaster_t broadcaster(cluster.get_mailbox_manager(),
                              &rdb_context,
                              &branch_history_manager,
                              &initial_store,
                              &get_global_perfmon_collection(),
                              &order_source,
                              &interruptor);

    watchable_variable_t<boost::optional<boost::optional<broadcaster_business_card_t> > > broadcaster_metadata_controller(
        boost::optional<boost::optional<broadcaster_business_card_t> >(broadcaster.get_business_card()));

    listener_t initial_listener(
        base_path_t("."),
        &io_backender,
        cluster.get_mailbox_manager(),
        generate_uuid(),
        broadcaster_metadata_controller.get_watchable(),
        &branch_history_manager,
        &broadcaster,
        &get_global_perfmon_collection(),
        &interruptor,
        &order_source);

    replier_t initial_replier(&initial_listener, cluster.get_mailbox_manager(), &branch_history_manager);

    /* Set up a master. The ack checker is impossible to satisfy, so every
    write will return an error. */
    class : public ack_checker_t {
    public:
        bool is_acceptable_ack_set(const std::set<server_id_t> &) const {
            return false;
        }
        write_durability_t get_write_durability() const {
            return write_durability_t::SOFT;
        }
    } ack_checker;
    master_t master(cluster.get_mailbox_manager(), &ack_checker, region_t::universe(), &broadcaster);

    /* Set up a master access */
    watchable_variable_t<boost::optional<boost::optional<master_business_card_t> > > master_directory_view(
        boost::make_optional(boost::make_optional(master.get_business_card())));
    cond_t non_interruptor_2;
    master_access_t master_access(
        cluster.get_mailbox_manager(),
        master_directory_view.get_watchable(),
        &non_interruptor_2);

    /* Confirm that it throws an exception */
    write_t w = mock_overwrite("a", "b");
    write_response_t wr;
    cond_t non_interruptor;
    fifo_enforcer_sink_t::exit_write_t write_token;
    master_access.new_write_token(&write_token);
    try {
        master_access.write(w, &wr,
            order_source.check_in("unittest::run_broadcaster_problem_test(clustering_query.cc)"),
            &write_token,
            &non_interruptor);
        ADD_FAILURE() << "That was supposed to fail.";
    } catch (const cannot_perform_query_exc_t &e) {
        /* expected */
    }
}

TEST(ClusteringQuery, BroadcasterProblem) {
    unittest::run_in_thread_pool(&run_broadcaster_problem_test);
}

}   /* namespace unittest */

