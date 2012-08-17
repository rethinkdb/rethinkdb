#include "unittest/gtest.hpp"

#include "clustering/immediate_consistency/branch/broadcaster.hpp"
#include "clustering/immediate_consistency/branch/listener.hpp"
#include "clustering/immediate_consistency/branch/replier.hpp"
#include "memcached/protocol.hpp"
#include "mock/branch_history_manager.hpp"
#include "mock/clustering_utils.hpp"
#include "mock/unittest_utils.hpp"
#include "unittest/dummy_metadata_controller.hpp"

namespace unittest {

void run_with_broadcaster(
        boost::function< void(io_backender_t *,
                              mock::simple_mailbox_cluster_t *,
                              branch_history_manager_t<memcached_protocol_t> *,
                              clone_ptr_t<watchable_t<boost::optional<boost::optional<broadcaster_business_card_t<memcached_protocol_t> > > > >,
                              scoped_ptr_t<broadcaster_t<memcached_protocol_t> > *,
                              mock::test_store_t<memcached_protocol_t> *,
                              scoped_ptr_t<listener_t<memcached_protocol_t> > *,
                              order_source_t *)> fun) {
    order_source_t order_source;

    /* Set up a cluster so mailboxes can be created */
    mock::simple_mailbox_cluster_t cluster;

    /* Set up branch history manager */
    mock::in_memory_branch_history_manager_t<memcached_protocol_t> branch_history_manager;

    // io backender
    scoped_ptr_t<io_backender_t> io_backender;
    make_io_backender(aio_default, &io_backender);

    /* Set up a broadcaster and initial listener */
    mock::test_store_t<memcached_protocol_t> initial_store(io_backender.get(), &order_source);
    store_view_t<memcached_protocol_t> *store_ptr = &initial_store.store;
    memcached_protocol_t::context_t ctx;
    multistore_ptr_t<memcached_protocol_t> multi_store(&store_ptr, 1, &ctx);
    cond_t interruptor;

    scoped_ptr_t<broadcaster_t<memcached_protocol_t> > broadcaster(
        new broadcaster_t<memcached_protocol_t>(
            cluster.get_mailbox_manager(),
            &branch_history_manager,
            &multi_store,
            &get_global_perfmon_collection(),
            &order_source,
            &interruptor
        ));

    // TODO: visit a psychiatrist
    watchable_variable_t<boost::optional<boost::optional<broadcaster_business_card_t<memcached_protocol_t> > > > broadcaster_business_card_watchable_variable(boost::optional<boost::optional<broadcaster_business_card_t<memcached_protocol_t> > >(boost::optional<broadcaster_business_card_t<memcached_protocol_t> >(broadcaster->get_business_card())));

    scoped_ptr_t<listener_t<memcached_protocol_t> > initial_listener(
        new listener_t<memcached_protocol_t>(io_backender.get(),
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
        &order_source);
}

void run_in_thread_pool_with_broadcaster(
        boost::function< void(io_backender_t *,
                              mock::simple_mailbox_cluster_t *,
                              branch_history_manager_t<memcached_protocol_t> *,
                              clone_ptr_t<watchable_t<boost::optional<boost::optional<broadcaster_business_card_t<memcached_protocol_t> > > > >,
                              scoped_ptr_t<broadcaster_t<memcached_protocol_t> > *,
                              mock::test_store_t<memcached_protocol_t> *,
                              scoped_ptr_t<listener_t<memcached_protocol_t> > *,
                              order_source_t *)> fun)
{
    mock::run_in_thread_pool(boost::bind(&run_with_broadcaster, fun));
}


/* `PartialBackfill` backfills only in a specific sub-region. */

void write_to_broadcaster(broadcaster_t<memcached_protocol_t> *broadcaster, const std::string& key, const std::string& value, order_token_t otok, signal_t *) {
    sarc_mutation_t set;
    set.key = store_key_t(key);
    set.data = data_buffer_t::create(value.size());
    memcpy(set.data->buf(), value.c_str(), value.size());
    set.flags = 123;
    set.exptime = 0;
    set.add_policy = add_policy_yes;
    set.replace_policy = replace_policy_yes;
    mock::fake_fifo_enforcement_t enforce;
    memcached_protocol_t::write_t write(set, time(NULL), 12345);
    fifo_enforcer_sink_t::exit_write_t exiter(&enforce.sink, enforce.source.enter_write());
    class : public broadcaster_t<memcached_protocol_t>::write_callback_t, public cond_t {
    public:
        void on_response(peer_id_t, const memcached_protocol_t::write_response_t &) {
            /* ignore */
        }
        void on_done() {
            pulse();
        }
    } write_callback;
    cond_t non_interruptor;
    broadcaster->spawn_write(write, &exiter, otok, &write_callback, &non_interruptor);
    write_callback.wait_lazily_unordered();
}

void run_partial_backfill_test(io_backender_t *io_backender,
                               mock::simple_mailbox_cluster_t *cluster,
                               branch_history_manager_t<memcached_protocol_t> *branch_history_manager,
                               clone_ptr_t<watchable_t<boost::optional<boost::optional<broadcaster_business_card_t<memcached_protocol_t> > > > > broadcaster_metadata_view,
                               scoped_ptr_t<broadcaster_t<memcached_protocol_t> > *broadcaster,
                               mock::test_store_t<memcached_protocol_t> *,
                               scoped_ptr_t<listener_t<memcached_protocol_t> > *initial_listener,
                               order_source_t *order_source) {
    /* Set up a replier so the broadcaster can handle operations */
    EXPECT_FALSE((*initial_listener)->get_broadcaster_lost_signal()->is_pulsed());
    replier_t<memcached_protocol_t> replier(initial_listener->get());

    watchable_variable_t<boost::optional<boost::optional<replier_business_card_t<memcached_protocol_t> > > >
        replier_business_card_variable(boost::optional<boost::optional<replier_business_card_t<memcached_protocol_t> > >(boost::optional<replier_business_card_t<memcached_protocol_t> >(replier.get_business_card())));

    /* Start sending operations to the broadcaster */
    std::map<std::string, std::string> inserter_state;
    mock::test_inserter_t inserter(
        boost::bind(&write_to_broadcaster, broadcaster->get(), _1, _2, _3, _4),
        NULL,
        &mock::mc_key_gen,
        order_source,
        "memcached_backfill run_partial_backfill_test inserter",
        &inserter_state);
    nap(10000);

    /* Set up a second mirror */
    mock::test_store_t<memcached_protocol_t> store2(io_backender, order_source);
    store_view_t<memcached_protocol_t> *store2_ptr = &store2.store;

    memcached_protocol_t::context_t ctx;
    multistore_ptr_t<memcached_protocol_t> multi_store2(&store2_ptr, 1, &ctx, memcached_protocol_t::region_t::universe());
    cond_t interruptor;
    listener_t<memcached_protocol_t> listener2(
        io_backender,
        cluster->get_mailbox_manager(),
        broadcaster_metadata_view,
        branch_history_manager,
        &multi_store2,
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
        get_query_t get;
        get.key = store_key_t(it->first);
        memcached_protocol_t::read_t read(get, time(NULL));
        mock::fake_fifo_enforcement_t enforce;
        fifo_enforcer_sink_t::exit_read_t exiter(&enforce.sink, enforce.source.enter_read());
        cond_t non_interruptor;
        memcached_protocol_t::read_response_t response =
            broadcaster->get()->read(read, &exiter, order_source->check_in("unittest::(memcached)run_partial_backfill_test").with_read_mode(), &non_interruptor);
        get_result_t get_result = boost::get<get_result_t>(response.result);
        EXPECT_TRUE(get_result.value.get() != NULL);
        EXPECT_EQ(it->second.size(), get_result.value->size());
        if (static_cast<size_t>(get_result.value->size()) == it->second.size()) {
            EXPECT_EQ(it->second, std::string(get_result.value->buf(), get_result.value->size()));
        }
    }
}

TEST(MemcachedBackfill, Backfill) {
     run_in_thread_pool_with_broadcaster(&run_partial_backfill_test);
}

}   /* namespace unittest */

