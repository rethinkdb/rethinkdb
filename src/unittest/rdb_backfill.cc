// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "clustering/administration/metadata.hpp"
#include "clustering/immediate_consistency/backfill_throttler.hpp"
#include "clustering/immediate_consistency/local_replicator.hpp"
#include "clustering/immediate_consistency/primary_dispatcher.hpp"
#include "clustering/immediate_consistency/remote_replicator_client.hpp"
#include "clustering/immediate_consistency/remote_replicator_server.hpp"
#include "extproc/extproc_pool.hpp"
#include "extproc/extproc_spawner.hpp"
#include "rdb_protocol/minidriver.hpp"
#include "rdb_protocol/pb_utils.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/store.hpp"
#include "rdb_protocol/sym.hpp"
#include "rpc/directory/read_manager.hpp"
#include "rpc/semilattice/semilattice_manager.hpp"
#include "stl_utils.hpp"
#include "unittest/branch_history_manager.hpp"
#include "unittest/clustering_utils.hpp"
#include "unittest/dummy_metadata_controller.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

ql::datum_t generate_document(size_t value_padding_length, const std::string &value) {
    ql::configured_limits_t limits;
    // This is a kind of hacky way to add an object to a map but I'm not sure
    // anyone really cares.
    return ql::to_datum(scoped_cJSON_t(cJSON_Parse(strprintf("{\"id\" : %s, \"padding\" : \"%s\"}",
                                                             value.c_str(),
                                                             std::string(value_padding_length, 'a').c_str()).c_str())).get(),
                        limits, reql_version_t::LATEST);
}

void write_to_dispatcher(size_t value_padding_length,
                         primary_dispatcher_t *dispatcher,
                         const std::string &key,
                         const std::string &value,
                         order_token_t otok,
                         signal_t *) {
    write_t write(
            point_write_t(
                store_key_t(key),
                generate_document(value_padding_length, value),
                true),
            DURABILITY_REQUIREMENT_DEFAULT,
            profile_bool_t::PROFILE,
            ql::configured_limits_t());
    simple_write_callback_t write_callback;
    dispatcher->spawn_write(write, otok, &write_callback);
    write_callback.wait_lazily_unordered();
}

void run_backfill_test(size_t value_padding_length) {

    order_source_t order_source;
    simple_mailbox_cluster_t cluster;
    in_memory_branch_history_manager_t branch_history_manager;
    io_backender_t io_backender(file_direct_io_mode_t::buffered_desired);
    extproc_pool_t extproc_pool(2);
    rdb_context_t ctx(&extproc_pool, NULL);
    cond_t non_interruptor;

    primary_dispatcher_t dispatcher(
        &get_global_perfmon_collection(),
        region_map_t<version_t>(region_t::universe(), version_t::zero()));

    test_store_t store1(&io_backender, &order_source, &ctx);
    in_memory_branch_history_manager_t bhm1;
    local_replicator_t local_replicator(
        cluster.get_mailbox_manager(),
        generate_uuid(),
        &dispatcher,
        &store1.store,
        &bhm1,
        &non_interruptor);

    /* Start sending operations to the broadcaster */
    std::map<std::string, std::string> inserter_state;
    test_inserter_t inserter(
        std::bind(&write_to_dispatcher, value_padding_length, &dispatcher,
                  ph::_1, ph::_2, ph::_3, ph::_4),
        std::function<std::string(const std::string &, order_token_t, signal_t *)>(),
        &mc_key_gen,
        &order_source,
        "rdb_backfill run_partial_backfill_test inserter",
        &inserter_state);
    nap(10000);

    remote_replicator_server_t remote_replicator_server(
        cluster.get_mailbox_manager(),
        &dispatcher);

    backfill_throttler_t backfill_throttler;

    /* Set up a second mirror */
    test_store_t store2(&io_backender, &order_source, &ctx);
    in_memory_branch_history_manager_t bhm2;
    cond_t interruptor;
    remote_replicator_client_t remote_replicator_client(
        base_path_t("."),
        &io_backender,
        &backfill_throttler,
        cluster.get_mailbox_manager(),
        generate_uuid(),
        dispatcher.get_branch_id(),
        remote_replicator_server.get_bcard(),
        local_replicator.get_replica_bcard(),
        &store2.store,
        &bhm2,
        &get_global_perfmon_collection(),
        &order_source,
        &interruptor,
        nullptr);

    nap(10000);

    /* Stop the inserter, then let any lingering writes finish */
    inserter.stop();
    /* Let any lingering writes finish */
    // TODO: 100 seconds?
    nap(100000);

    for (std::map<std::string, std::string>::iterator it = inserter_state.begin();
            it != inserter_state.end(); it++) {
        read_t read(point_read_t(store_key_t(it->first)), profile_bool_t::PROFILE);
        fifo_enforcer_source_t fifo_source;
        fifo_enforcer_sink_t fifo_sink;
        fifo_enforcer_sink_t::exit_read_t exiter(&fifo_sink, fifo_source.enter_read());
        read_response_t response;
        dispatcher.read(
            read, &exiter,
            order_source.check_in("(rdb)run_partial_backfill_test").with_read_mode(),
            &non_interruptor, &response);
        point_read_response_t get_result =
            boost::get<point_read_response_t>(response.response);
        EXPECT_TRUE(get_result.data.has());
        EXPECT_EQ(generate_document(value_padding_length,
                                     it->second),
                  get_result.data);
    }
}

TPTEST(RDBProtocolBackfill, Backfill) {
    run_backfill_test(0);
}

TPTEST(RDBProtocolBackfill, BackfillLargeValues) {
     run_backfill_test(300);
}

}   /* namespace unittest */
