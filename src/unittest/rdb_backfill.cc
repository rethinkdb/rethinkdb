// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "clustering/administration/metadata.hpp"
#include "clustering/immediate_consistency/branch/broadcaster.hpp"
#include "clustering/immediate_consistency/branch/listener.hpp"
#include "clustering/immediate_consistency/branch/replier.hpp"
#include "extproc/extproc_pool.hpp"
#include "extproc/extproc_spawner.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/pb_utils.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/sym.hpp"
#include "rpc/directory/read_manager.hpp"
#include "rpc/semilattice/semilattice_manager.hpp"
#include "unittest/branch_history_manager.hpp"
#include "unittest/clustering_utils.hpp"
#include "unittest/dummy_metadata_controller.hpp"
#include "unittest/unittest_utils.hpp"

#pragma GCC diagnostic ignored "-Wshadow"

namespace unittest {

void run_with_broadcaster(
    boost::function< void(
        std::pair<io_backender_t *, simple_mailbox_cluster_t *>,
        branch_history_manager_t<rdb_protocol_t> *,
        clone_ptr_t<watchable_t<boost::optional<boost::optional<
            broadcaster_business_card_t<rdb_protocol_t> > > > >,
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
    io_backender_t io_backender(file_direct_io_mode_t::buffered_desired);

    /* Create some structures for the rdb_protocol_t::context_t, warning some
     * boilerplate is about to follow, avert your eyes if you have a weak
     * stomach for such things. */
    extproc_pool_t extproc_pool(2);

    connectivity_cluster_t c;
    semilattice_manager_t<cluster_semilattice_metadata_t> slm(&c, cluster_semilattice_metadata_t());
    connectivity_cluster_t::run_t cr(&c, get_unittest_addresses(), peer_address_t(), ANY_PORT, &slm, 0, NULL);

    connectivity_cluster_t c2;
    directory_read_manager_t<cluster_directory_metadata_t> read_manager(&c2);
    connectivity_cluster_t::run_t cr2(&c2, get_unittest_addresses(), peer_address_t(), ANY_PORT, &read_manager, 0, NULL);

    boost::shared_ptr<semilattice_readwrite_view_t<auth_semilattice_metadata_t> > dummy_auth;
    rdb_protocol_t::context_t ctx(&extproc_pool, NULL, slm.get_root_view(),
                                  dummy_auth, &read_manager, generate_uuid());

    /* Set up a broadcaster and initial listener */
    test_store_t<rdb_protocol_t> initial_store(&io_backender, &order_source, &ctx);
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
                                       &io_backender,
                                       cluster.get_mailbox_manager(),
                                       broadcaster_business_card_watchable_variable.get_watchable(),
                                       &branch_history_manager,
                                       broadcaster.get(),
                                       &get_global_perfmon_collection(),
                                       &interruptor,
                                       &order_source));

    fun(std::make_pair(&io_backender, &cluster),
        &branch_history_manager,
        broadcaster_business_card_watchable_variable.get_watchable(),
        &broadcaster,
        &initial_store,
        &initial_listener,
        &ctx,
        &order_source);
}

void run_in_thread_pool_with_broadcaster(
        boost::function< void(std::pair<io_backender_t *, simple_mailbox_cluster_t *>,
                              branch_history_manager_t<rdb_protocol_t> *,
                              clone_ptr_t<watchable_t<boost::optional<boost::optional<broadcaster_business_card_t<rdb_protocol_t> > > > >,
                              scoped_ptr_t<broadcaster_t<rdb_protocol_t> > *,
                              test_store_t<rdb_protocol_t> *,
                              scoped_ptr_t<listener_t<rdb_protocol_t> > *,
                              rdb_protocol_t::context_t *,
                              order_source_t *)> fun)
{
    extproc_spawner_t extproc_spawner;
    run_in_thread_pool(boost::bind(&run_with_broadcaster, fun));
}


/* `PartialBackfill` backfills only in a specific sub-region. */

counted_t<const ql::datum_t> generate_document(size_t value_padding_length, const std::string &value) {
    // This is a kind of hacky way to add an object to a map but I'm not sure
    // anyone really cares.
    return make_counted<ql::datum_t>(scoped_cJSON_t(cJSON_Parse(strprintf("{\"id\" : %s, \"padding\" : \"%s\"}",
                                                                          value.c_str(),
                                                                          std::string(value_padding_length, 'a').c_str()).c_str())));
}

void write_to_broadcaster(size_t value_padding_length,
                          broadcaster_t<rdb_protocol_t> *broadcaster,
                          const std::string &key,
                          const std::string &value,
                          order_token_t otok,
                          signal_t *) {
    rdb_protocol_t::write_t write(rdb_protocol_t::point_write_t(store_key_t(key),
                                                                generate_document(value_padding_length, value),
                                                                true),
                                  DURABILITY_REQUIREMENT_DEFAULT);

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
    } write_callback;
    cond_t non_interruptor;
    spawn_write_fake_ack_checker_t ack_checker;
    broadcaster->spawn_write(write, &exiter, otok, &write_callback, &non_interruptor, &ack_checker);
    write_callback.wait_lazily_unordered();
}

void run_backfill_test(size_t value_padding_length,
                       std::pair<io_backender_t *, simple_mailbox_cluster_t *> io_backender_and_cluster,
                       branch_history_manager_t<rdb_protocol_t> *branch_history_manager,
                       clone_ptr_t<watchable_t<boost::optional<boost::optional<broadcaster_business_card_t<rdb_protocol_t> > > > > broadcaster_metadata_view,
                       scoped_ptr_t<broadcaster_t<rdb_protocol_t> > *broadcaster,
                       test_store_t<rdb_protocol_t> *,
                       scoped_ptr_t<listener_t<rdb_protocol_t> > *initial_listener,
                       rdb_protocol_t::context_t *ctx,
                       order_source_t *order_source) {
    io_backender_t *const io_backender = io_backender_and_cluster.first;
    simple_mailbox_cluster_t *const cluster = io_backender_and_cluster.second;

    recreate_temporary_directory(base_path_t("."));
    /* Set up a replier so the broadcaster can handle operations */
    EXPECT_FALSE((*initial_listener)->get_broadcaster_lost_signal()->is_pulsed());
    replier_t<rdb_protocol_t> replier(initial_listener->get(), cluster->get_mailbox_manager(), branch_history_manager);

    watchable_variable_t<boost::optional<boost::optional<replier_business_card_t<rdb_protocol_t> > > >
        replier_business_card_variable(boost::optional<boost::optional<replier_business_card_t<rdb_protocol_t> > >(boost::optional<replier_business_card_t<rdb_protocol_t> >(replier.get_business_card())));

    /* Start sending operations to the broadcaster */
    std::map<std::string, std::string> inserter_state;
    test_inserter_t inserter(
        boost::bind(&write_to_broadcaster, value_padding_length, broadcaster->get(), _1, _2, _3, _4),
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
        EXPECT_EQ(*generate_document(value_padding_length,
                                     it->second),
                  *get_result.data);
    }
}

TEST(RDBProtocolBackfill, Backfill) {
     run_in_thread_pool_with_broadcaster(boost::bind(&run_backfill_test, 0, _1, _2, _3, _4, _5, _6, _7, _8));
}

TEST(RDBProtocolBackfill, BackfillLargeValues) {
     run_in_thread_pool_with_broadcaster(boost::bind(&run_backfill_test, 300, _1, _2, _3, _4, _5, _6, _7, _8));
}

void run_sindex_backfill_test(std::pair<io_backender_t *, simple_mailbox_cluster_t *> io_backender_and_cluster,
                              branch_history_manager_t<rdb_protocol_t> *branch_history_manager,
                              clone_ptr_t<watchable_t<boost::optional<boost::optional<broadcaster_business_card_t<rdb_protocol_t> > > > > broadcaster_metadata_view,
                              scoped_ptr_t<broadcaster_t<rdb_protocol_t> > *broadcaster,
                              test_store_t<rdb_protocol_t> *,
                              scoped_ptr_t<listener_t<rdb_protocol_t> > *initial_listener,
                              rdb_protocol_t::context_t *ctx,
                              order_source_t *order_source) {
    io_backender_t *const io_backender = io_backender_and_cluster.first;
    simple_mailbox_cluster_t *const cluster = io_backender_and_cluster.second;

    recreate_temporary_directory(base_path_t("."));
    /* Set up a replier so the broadcaster can handle operations */
    EXPECT_FALSE((*initial_listener)->get_broadcaster_lost_signal()->is_pulsed());
    replier_t<rdb_protocol_t> replier(initial_listener->get(), cluster->get_mailbox_manager(), branch_history_manager);

    watchable_variable_t<boost::optional<boost::optional<replier_business_card_t<rdb_protocol_t> > > >
        replier_business_card_variable(boost::optional<boost::optional<replier_business_card_t<rdb_protocol_t> > >(boost::optional<replier_business_card_t<rdb_protocol_t> >(replier.get_business_card())));

    std::string sindex_id("sid");
    {
        /* Create a secondary index object. */
        const ql::sym_t one(1);
        ql::protob_t<Term> twrap = ql::make_counted_term();
        Term *arg = twrap.get();
        N2(GET_FIELD, NVAR(one), NDATUM("id"));

        ql::map_wire_func_t m(twrap, make_vector(one), get_backtrace(twrap));

        rdb_protocol_t::write_t write(rdb_protocol_t::sindex_create_t(sindex_id, m, SINGLE));

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
        } write_callback;
        cond_t non_interruptor;
        spawn_write_fake_ack_checker_t ack_checker;
        broadcaster->get()->spawn_write(write, &exiter, order_token_t::ignore,
                                        &write_callback, &non_interruptor, &ack_checker);
        write_callback.wait_lazily_unordered();
    }

    /* Start sending operations to the broadcaster */
    std::map<std::string, std::string> inserter_state;
    test_inserter_t inserter(
        boost::bind(&write_to_broadcaster, 0, broadcaster->get(), _1, _2, _3, _4),
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

    cond_t dummy_interruptor;
    ql::env_t dummy_env(&dummy_interruptor);

    for (std::map<std::string, std::string>::iterator it = inserter_state.begin();
            it != inserter_state.end(); it++) {
        scoped_cJSON_t sindex_key_json(cJSON_Parse(it->second.c_str()));
        auto sindex_key_literal = make_counted<const ql::datum_t>(sindex_key_json);
        rdb_protocol_t::read_t read(rdb_protocol_t::rget_read_t(
            sindex_id, sindex_range_t(sindex_key_literal, false, sindex_key_literal, false)));
        fake_fifo_enforcement_t enforce;
        fifo_enforcer_sink_t::exit_read_t exiter(&enforce.sink, enforce.source.enter_read());
        cond_t non_interruptor;
        rdb_protocol_t::read_response_t response;
        broadcaster->get()->read(read, &response, &exiter, order_source->check_in("unittest::(rdb)run_partial_backfill_test").with_read_mode(), &non_interruptor);
        rdb_protocol_t::rget_read_response_t get_result = boost::get<rdb_protocol_t::rget_read_response_t>(response.response);
        auto result_stream = boost::get<rdb_protocol_t::rget_read_response_t::stream_t>(&get_result.result);
        guarantee(result_stream);
        ASSERT_EQ(1u, result_stream->size());
        EXPECT_EQ(*generate_document(0, it->second), *result_stream->at(0).data);
    }
}

TEST(RDBProtocolBackfill, SindexBackfill) {
     run_in_thread_pool_with_broadcaster(&run_sindex_backfill_test);
}

}   /* namespace unittest */

