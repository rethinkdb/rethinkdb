// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "clustering/administration/metadata.hpp"
#include "clustering/immediate_consistency/branch/broadcaster.hpp"
#include "clustering/immediate_consistency/branch/listener.hpp"
#include "clustering/immediate_consistency/branch/replier.hpp"
#include "extproc/pool.hpp"
#include "extproc/spawner.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rpc/directory/read_manager.hpp"
#include "rpc/semilattice/semilattice_manager.hpp"
#include "unittest/branch_history_manager.hpp"
#include "unittest/clustering_utils.hpp"
#include "unittest/dummy_metadata_controller.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

void run_with_broadcaster(
        boost::function< void(io_backender_t *,
                              simple_mailbox_cluster_t *,
                              branch_history_manager_t<rdb_protocol_t> *,
                              clone_ptr_t<watchable_t<boost::optional<boost::optional<broadcaster_business_card_t<rdb_protocol_t> > > > >,
                              scoped_ptr_t<broadcaster_t<rdb_protocol_t> > *,
                              test_store_t<rdb_protocol_t> *,
                              scoped_ptr_t<listener_t<rdb_protocol_t> > *,
                              rdb_protocol_t::context_t *ctx,
                              order_source_t *)> fun) {
    order_source_t order_source;

    /* Set up a cluster so mailboxes can be created */
    simple_mailbox_cluster_t cluster;

    /* Set up branch history manager */
    in_memory_branch_history_manager_t<rdb_protocol_t> branch_history_manager;

    // io backender
    scoped_ptr_t<io_backender_t> io_backender;
    make_io_backender(aio_default, &io_backender);

    /* Create some structures for the rdb_protocol_t::context_t, warning some
     * boilerplate is about to follow, avert your eyes if you have a weak
     * stomach for such things. */
    extproc::spawner_info_t spawner_info;
    extproc::spawner_t::create(&spawner_info);
    extproc::pool_group_t pool_group(&spawner_info, extproc::pool_group_t::DEFAULTS);

    int port = randport();
    connectivity_cluster_t c;
    semilattice_manager_t<cluster_semilattice_metadata_t> slm(&c, cluster_semilattice_metadata_t());
    connectivity_cluster_t::run_t cr(&c, get_unittest_addresses(), port, &slm, 0, NULL);

    int port2 = randport();
    connectivity_cluster_t c2;
    directory_read_manager_t<cluster_directory_metadata_t> read_manager(&c2);
    connectivity_cluster_t::run_t cr2(&c2, get_unittest_addresses(), port2, &read_manager, 0, NULL);

    rdb_protocol_t::context_t ctx(&pool_group, NULL, slm.get_root_view(), &read_manager, generate_uuid());

    /* Set up a broadcaster and initial listener */
    test_store_t<rdb_protocol_t> initial_store(io_backender.get(), &order_source, &ctx);
    cond_t interruptor;

    scoped_ptr_t<broadcaster_t<rdb_protocol_t> > broadcaster(
        new broadcaster_t<rdb_protocol_t>(
            cluster.get_mailbox_manager(),
            &branch_history_manager,
            &initial_store.store,
            &get_global_perfmon_collection(),
            &order_source,
            &interruptor));

    watchable_variable_t<boost::optional<boost::optional<broadcaster_business_card_t<rdb_protocol_t> > > > broadcaster_business_card_watchable_variable(boost::optional<boost::optional<broadcaster_business_card_t<rdb_protocol_t> > >(boost::optional<broadcaster_business_card_t<rdb_protocol_t> >(broadcaster->get_business_card())));

    scoped_ptr_t<listener_t<rdb_protocol_t> > initial_listener(
        new listener_t<rdb_protocol_t>(base_path_t("."), //TODO is it bad that this isn't configurable?
                                       io_backender.get(),
                                       cluster.get_mailbox_manager(),
                                       broadcaster_business_card_watchable_variable.get_watchable(),
                                       &branch_history_manager,
                                       broadcaster.get(),
                                       &get_global_perfmon_collection(),
                                       &interruptor,
                                       &order_source));

    fun(io_backender.get(),
        &cluster,
        &branch_history_manager,
        broadcaster_business_card_watchable_variable.get_watchable(),
        &broadcaster,
        &initial_store,
        &initial_listener,
        &ctx,
        &order_source);
}

void run_in_thread_pool_with_broadcaster(
        boost::function< void(io_backender_t *,
                              simple_mailbox_cluster_t *,
                              branch_history_manager_t<rdb_protocol_t> *,
                              clone_ptr_t<watchable_t<boost::optional<boost::optional<broadcaster_business_card_t<rdb_protocol_t> > > > >,
                              scoped_ptr_t<broadcaster_t<rdb_protocol_t> > *,
                              test_store_t<rdb_protocol_t> *,
                              scoped_ptr_t<listener_t<rdb_protocol_t> > *,
                              rdb_protocol_t::context_t *,
                              order_source_t *)> fun)
{
    run_in_thread_pool(boost::bind(&run_with_broadcaster, fun));
}


/* `PartialBackfill` backfills only in a specific sub-region. */

void write_to_broadcaster(broadcaster_t<rdb_protocol_t> *broadcaster, const std::string& key, const std::string& value, order_token_t otok, signal_t *) {
    rdb_protocol_t::write_t write(rdb_protocol_t::point_write_t(store_key_t(key), boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_Parse(value.c_str()))), true));

    fake_fifo_enforcement_t enforce;
    fifo_enforcer_sink_t::exit_write_t exiter(&enforce.sink, enforce.source.enter_write());
    class : public broadcaster_t<rdb_protocol_t>::write_callback_t, public cond_t {
    public:
        void on_response(peer_id_t, const rdb_protocol_t::write_response_t &) {
            /* ignore */
        }
        void on_done() {
            pulse();
        }
        void on_disk_ack(peer_id_t) { /* Nobody cares. */ }
    } write_callback;
    cond_t non_interruptor;
    broadcaster->spawn_write(write, &exiter, otok, &write_callback, &non_interruptor);
    write_callback.wait_lazily_unordered();
}

void run_partial_backfill_test(io_backender_t *io_backender,
                               simple_mailbox_cluster_t *cluster,
                               branch_history_manager_t<rdb_protocol_t> *branch_history_manager,
                               clone_ptr_t<watchable_t<boost::optional<boost::optional<broadcaster_business_card_t<rdb_protocol_t> > > > > broadcaster_metadata_view,
                               scoped_ptr_t<broadcaster_t<rdb_protocol_t> > *broadcaster,
                               test_store_t<rdb_protocol_t> *,
                               scoped_ptr_t<listener_t<rdb_protocol_t> > *initial_listener,
                               rdb_protocol_t::context_t *ctx,
                               order_source_t *order_source) {
    recreate_temporary_directory(base_path_t("."));
    /* Set up a replier so the broadcaster can handle operations */
    EXPECT_FALSE((*initial_listener)->get_broadcaster_lost_signal()->is_pulsed());
    replier_t<rdb_protocol_t> replier(initial_listener->get(), cluster->get_mailbox_manager(), branch_history_manager);

    watchable_variable_t<boost::optional<boost::optional<replier_business_card_t<rdb_protocol_t> > > >
        replier_business_card_variable(boost::optional<boost::optional<replier_business_card_t<rdb_protocol_t> > >(boost::optional<replier_business_card_t<rdb_protocol_t> >(replier.get_business_card())));

    /* Start sending operations to the broadcaster */
    std::map<std::string, std::string> inserter_state;
    test_inserter_t inserter(
        boost::bind(&write_to_broadcaster, broadcaster->get(), _1, _2, _3, _4),
        NULL,
        &mc_key_gen,
        order_source,
        "rdb_backfill run_partial_backfill_test inserter",
        &inserter_state);
    nap(10000);

    /* Set up a second mirror */
    test_store_t<rdb_protocol_t> store2(io_backender, order_source, ctx);
    cond_t interruptor;
    listener_t<rdb_protocol_t> listener2(
        base_path_t("."),
        io_backender,
        cluster->get_mailbox_manager(),
        broadcaster_metadata_view,
        branch_history_manager,
        &store2.store,
        replier_business_card_variable.get_watchable(),
        generate_uuid(),
        &get_global_perfmon_collection(),
        &interruptor,
        order_source);

    EXPECT_FALSE((*initial_listener)->get_broadcaster_lost_signal()->is_pulsed());
    EXPECT_FALSE(listener2.get_broadcaster_lost_signal()->is_pulsed());

    nap(10000);

    /* Stop the inserter, then let any lingering writes finish */
    inserter.stop();
    /* Let any lingering writes finish */
    // TODO: 100 seconds?
    nap(100000);

    for (std::map<std::string, std::string>::iterator it = inserter_state.begin();
            it != inserter_state.end(); it++) {
        rdb_protocol_t::read_t read(rdb_protocol_t::point_read_t(store_key_t(it->first)));
        fake_fifo_enforcement_t enforce;
        fifo_enforcer_sink_t::exit_read_t exiter(&enforce.sink, enforce.source.enter_read());
        cond_t non_interruptor;
        rdb_protocol_t::read_response_t response;
        broadcaster->get()->read(read, &response, &exiter, order_source->check_in("unittest::(rdb)run_partial_backfill_test").with_read_mode(), &non_interruptor);
        rdb_protocol_t::point_read_response_t get_result = boost::get<rdb_protocol_t::point_read_response_t>(response.response);
        EXPECT_TRUE(get_result.data.get() != NULL);
        EXPECT_EQ(query_language::json_cmp(scoped_cJSON_t(cJSON_Parse(it->second.c_str())).get(), get_result.data->get()), 0);
    }
}

TEST(RDBProtocolBackfill, Backfill) {
     run_in_thread_pool_with_broadcaster(&run_partial_backfill_test);
}

}   /* namespace unittest */

