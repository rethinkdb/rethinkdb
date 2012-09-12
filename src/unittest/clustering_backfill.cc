#include "unittest/gtest.hpp"
#include "clustering/immediate_consistency/branch/backfiller.hpp"
#include "clustering/immediate_consistency/branch/backfillee.hpp"
#include "clustering/immediate_consistency/branch/multistore.hpp"
#include "containers/uuid.hpp"
#include "rpc/semilattice/view/field.hpp"
#include "mock/branch_history_manager.hpp"
#include "mock/clustering_utils.hpp"
#include "mock/dummy_protocol.hpp"
#include "mock/unittest_utils.hpp"

using mock::dummy_protocol_t;

namespace unittest {

namespace {

boost::optional<boost::optional<backfiller_business_card_t<dummy_protocol_t> > > wrap_in_optional(
        const boost::optional<backfiller_business_card_t<dummy_protocol_t> > &inner) {
    return boost::optional<boost::optional<backfiller_business_card_t<dummy_protocol_t> > >(inner);
}

}   /* anonymous namespace */

void run_backfill_test() {

    order_source_t order_source;

    /* Set up two stores */

    dummy_protocol_t::region_t region;
    for (char c = 'a'; c <= 'z'; c++) {
        region.keys.insert(std::string(&c, 1));
    }

    dummy_protocol_t::store_t backfiller_store;
    dummy_protocol_t::store_t backfillee_store;

    mock::in_memory_branch_history_manager_t<mock::dummy_protocol_t> branch_history_manager;
    branch_id_t dummy_branch_id = generate_uuid();
    {
        branch_birth_certificate_t<dummy_protocol_t> dummy_branch;
        dummy_branch.region = region;
        dummy_branch.initial_timestamp = state_timestamp_t::zero();
        dummy_branch.origin = region_map_t<dummy_protocol_t, version_range_t>(
            region, version_range_t(version_t(nil_uuid(), state_timestamp_t::zero())));
        cond_t non_interruptor;
        branch_history_manager.create_branch(dummy_branch_id, dummy_branch, &non_interruptor);
    }

    state_timestamp_t timestamp = state_timestamp_t::zero();

    // initialize the metainfo in a store
    store_view_t<dummy_protocol_t> *stores[] = { &backfiller_store, &backfillee_store };
    for (size_t i = 0; i < sizeof(stores) / sizeof(stores[0]); i++) {
        cond_t non_interruptor;
        object_buffer_t<fifo_enforcer_sink_t::exit_write_t> token;
        stores[i]->new_write_token(&token);
        stores[i]->set_metainfo(
            region_map_t<dummy_protocol_t, binary_blob_t>(region,
                                                          binary_blob_t(version_range_t(version_t(dummy_branch_id, timestamp)))),
            order_source.check_in(strprintf("set_metainfo(i=%zu)", i)),
            &token,
            &non_interruptor);
    }

    // Insert 10 values into both stores, then another 10 into only `backfiller_store` and not `backfillee_store`
    for (int i = 0; i < 20; i++) {
        dummy_protocol_t::write_t w;
        dummy_protocol_t::write_response_t response;
        std::string key = std::string(1, 'a' + randint(26));
        w.values[key] = strprintf("%d", i);

        for (int j = 0; j < (i < 10 ? 2 : 1); j++) {
            transition_timestamp_t ts = transition_timestamp_t::starting_from(timestamp);
            timestamp = ts.timestamp_after();

            cond_t non_interruptor;
            object_buffer_t<fifo_enforcer_sink_t::exit_write_t> token;
            backfiller_store.new_write_token(&token);

#ifndef NDEBUG
            mock::equality_metainfo_checker_callback_t<dummy_protocol_t>
                metainfo_checker_callback(binary_blob_t(version_range_t(version_t(dummy_branch_id, ts.timestamp_before()))));
            metainfo_checker_t<dummy_protocol_t> metainfo_checker(&metainfo_checker_callback, region);
#endif

            backfiller_store.write(
                DEBUG_ONLY(metainfo_checker, )
                region_map_t<dummy_protocol_t, binary_blob_t>(
                    region,
                    binary_blob_t(version_range_t(version_t(dummy_branch_id, timestamp)))
                ),
                w, &response, ts,
                order_source.check_in(strprintf("backfiller_store.write(j=%d)", j)),
                &token,
                &non_interruptor);
        }
    }

    // Set up a cluster so mailboxes can be created

    mock::simple_mailbox_cluster_t cluster;

    /* Expose the backfiller to the cluster */

    store_view_t<dummy_protocol_t> *backfiller_store_ptr = &backfiller_store;

    dummy_protocol_t::context_t ctx;
    multistore_ptr_t<dummy_protocol_t> backfiller_multistore(&backfiller_store_ptr, 1, &ctx);

    backfiller_t<dummy_protocol_t> backfiller(
        cluster.get_mailbox_manager(),
        &branch_history_manager,
        &backfiller_multistore);

    watchable_variable_t<boost::optional<backfiller_business_card_t<dummy_protocol_t> > > pseudo_directory(
        boost::optional<backfiller_business_card_t<dummy_protocol_t> >(backfiller.get_business_card()));

    /* Run a backfill */

    // Uhh.. hehhehheh... this might be wrong.
    dummy_protocol_t::context_t ctx2;
    store_view_t<dummy_protocol_t> *backfillee_store_ptr = &backfillee_store;
    multistore_ptr_t<dummy_protocol_t> backfillee_multistore(&backfillee_store_ptr, 1, &ctx2);

    cond_t interruptor;
    backfillee<dummy_protocol_t>(
        cluster.get_mailbox_manager(),
        &branch_history_manager,
        &backfillee_multistore,
        backfillee_store.get_region(),
        pseudo_directory.get_watchable()->subview(&wrap_in_optional),
        generate_uuid(),
        &interruptor);

    /* Make sure everything got transferred properly */

    for (char c = 'a'; c <= 'z'; c++) {
        std::string key(1, c);
        EXPECT_EQ(backfiller_store.values[key], backfillee_store.values[key]);
        EXPECT_TRUE(backfiller_store.timestamps[key] == backfillee_store.timestamps[key]);
    }

    object_buffer_t<fifo_enforcer_sink_t::exit_read_t> token1;
    backfillee_store.new_read_token(&token1);

    region_map_t<dummy_protocol_t, binary_blob_t> untransformed_backfillee_metadata;
    backfillee_store.do_get_metainfo(order_source.check_in("backfillee_store.do_get_metainfo").with_read_mode(),
                                     &token1, &interruptor, &untransformed_backfillee_metadata);

    region_map_t<dummy_protocol_t, version_range_t> backfillee_metadata =
        region_map_transform<dummy_protocol_t, binary_blob_t, version_range_t>(
            untransformed_backfillee_metadata,
            &binary_blob_t::get<version_range_t>);

    object_buffer_t<fifo_enforcer_sink_t::exit_read_t> token2;
    backfiller_store.new_read_token(&token2);

    region_map_t<dummy_protocol_t, binary_blob_t> untransformed_backfiller_metadata;
    backfiller_store.do_get_metainfo(order_source.check_in("backfiller_store.do_get_metainfo").with_read_mode(),
                                     &token2, &interruptor, &untransformed_backfiller_metadata);

    region_map_t<dummy_protocol_t, version_range_t> backfiller_metadata =
        region_map_transform<dummy_protocol_t, binary_blob_t, version_range_t>(untransformed_backfiller_metadata, &binary_blob_t::get<version_range_t>);

    EXPECT_TRUE(backfillee_metadata == backfiller_metadata);

    //EXPECT_EQ(1, backfillee_metadata.size());
    //EXPECT_TRUE(backfillee_metadata[0].second.is_coherent());
    //EXPECT_EQ(timestamp, backfillee_metadata[0].second.earliest.timestamp);
}
TEST(ClusteringBackfill, BackfillTest) {
    mock::run_in_thread_pool(&run_backfill_test);
}

}   /* namespace unittest */
