#include "unittest/gtest.hpp"
#include "clustering/immediate_consistency/branch/backfiller.hpp"
#include "clustering/immediate_consistency/branch/backfillee.hpp"
#include "rpc/semilattice/view/field.hpp"
#include "unittest/clustering_utils.hpp"
#include "unittest/dummy_metadata_controller.hpp"
#include "mock/dummy_protocol.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

void run_backfill_test() {

    /* Set up two stores */

    dummy_protocol_t::region_t region;
    for (char c = 'a'; c <= 'z'; c++) {
        region.keys.insert(std::string(&c, 1));
    }

    dummy_protocol_t::store_t backfiller_store, backfillee_store;

    /* Make a dummy metadata view to hold a branch tree so branch history checks
    can be performed */

    dummy_semilattice_controller_t<branch_history_t<dummy_protocol_t> > branch_history_controller(
        /* Parentheses prevent C++ from interpreting this as a function
        declaration */
        (branch_history_t<dummy_protocol_t>())
        );
    branch_id_t dummy_branch_id = generate_uuid();
    {
        branch_birth_certificate_t<dummy_protocol_t> dummy_branch;
        dummy_branch.region = region;
        dummy_branch.initial_timestamp = state_timestamp_t::zero();
        dummy_branch.origin = region_map_t<dummy_protocol_t, version_range_t>(
            region, version_range_t(version_t(boost::uuids::nil_generator()(), state_timestamp_t::zero()))
            );
        std::map<branch_id_t, branch_birth_certificate_t<dummy_protocol_t> > singleton_map;
        singleton_map[dummy_branch_id] = dummy_branch;
        metadata_field(&branch_history_t<dummy_protocol_t>::branches, branch_history_controller.get_view())->join(singleton_map);
    }

    state_timestamp_t timestamp = state_timestamp_t::zero();

    // initialize the metainfo in a store
    store_view_t<dummy_protocol_t> *stores[] = { &backfiller_store, &backfillee_store };
    for (size_t i = 0; i < sizeof stores / sizeof stores[0]; i++) {
        cond_t non_interruptor;
        boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> token;
        stores[i]->new_write_token(token);
        stores[i]->set_metainfo(
            region_map_t<dummy_protocol_t, binary_blob_t>(
                region,
                binary_blob_t(version_range_t(version_t(dummy_branch_id, timestamp)))
                ),  
                token,
                &non_interruptor
            );
    }

    // Insert 10 values into both stores, then another 10 into only `backfiller_store` and not `backfillee_store`
    for (int i = 0; i < 20; i++) {
        dummy_protocol_t::write_t w;
        std::string key = std::string(1, 'a' + rand() % 26);
        w.values[key] = strprintf("%d", i);

        for (int j = 0; j < (i < 10 ? 2 : 1); j++) {
            transition_timestamp_t ts = transition_timestamp_t::starting_from(timestamp);
            timestamp = ts.timestamp_after();

            cond_t non_interruptor;
            boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> token;
            backfiller_store.new_write_token(token);

            backfiller_store.write(
                DEBUG_ONLY(
                    region_map_t<dummy_protocol_t, binary_blob_t>(
                        region,
                        binary_blob_t(version_range_t(version_t(dummy_branch_id, ts.timestamp_before())))
                    ),
                )
                region_map_t<dummy_protocol_t, binary_blob_t>(
                    region,
                    binary_blob_t(version_range_t(version_t(dummy_branch_id, timestamp)))
                ),
                w, ts,
                token,
                &non_interruptor
            );
        }
    }

    // Set up a cluster so mailboxes can be created

    simple_mailbox_cluster_t cluster;

    /* Expose the backfiller to the cluster */

    backfiller_t<dummy_protocol_t> backfiller(
        cluster.get_mailbox_manager(),
        branch_history_controller.get_view(),
        &backfiller_store);

    simple_directory_manager_t<boost::optional<backfiller_business_card_t<dummy_protocol_t> > > directory_manager(
        &cluster,
        boost::optional<backfiller_business_card_t<dummy_protocol_t> >(backfiller.get_business_card())
        );

    /* Run a backfill */

    cond_t interruptor;
    backfillee<dummy_protocol_t>(
        cluster.get_mailbox_manager(),
        branch_history_controller.get_view(),
        &backfillee_store,
        directory_manager.get_root_view()->get_peer_view(cluster.get_connectivity_service()->get_me()),
        &interruptor);

    /* Make sure everything got transferred properly */

    for (char c = 'a'; c <= 'z'; c++) {
        std::string key(1, c);
        EXPECT_EQ(backfiller_store.values[key], backfillee_store.values[key]);
        EXPECT_TRUE(backfiller_store.timestamps[key] == backfillee_store.timestamps[key]);
    }

    boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> token1;
    backfillee_store.new_read_token(token1);

    region_map_t<dummy_protocol_t, version_range_t> backfillee_metadata = 
        region_map_transform<dummy_protocol_t, binary_blob_t, version_range_t>(
            backfillee_store.get_metainfo(token1, &interruptor),
            &binary_blob_t::get<version_range_t>
        );

    boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> token2;
    backfiller_store.new_read_token(token2);

    region_map_t<dummy_protocol_t, version_range_t> backfiller_metadata = 
        region_map_transform<dummy_protocol_t, binary_blob_t, version_range_t>(
            backfiller_store.get_metainfo(token2, &interruptor),
            &binary_blob_t::get<version_range_t>
        );

    EXPECT_TRUE(backfillee_metadata == backfiller_metadata);

    //EXPECT_EQ(1, backfillee_metadata.size());
    //EXPECT_TRUE(backfillee_metadata[0].second.is_coherent());
    //EXPECT_EQ(timestamp, backfillee_metadata[0].second.earliest.timestamp);
}
TEST(ClusteringBackfill, BackfillTest) {
    run_in_thread_pool(&run_backfill_test);
}

}   /* namespace unittest */
