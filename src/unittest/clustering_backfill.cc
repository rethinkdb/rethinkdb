// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/backfiller.hpp"
#include "clustering/immediate_consistency/backfillee.hpp"
#include "containers/uuid.hpp"
#include "rpc/semilattice/view/field.hpp"
#include "unittest/branch_history_manager.hpp"
#include "unittest/clustering_utils.hpp"
#include "unittest/gtest.hpp"
#include "unittest/mock_store.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

TPTEST(ClusteringBackfill, BackfillTest) {
    order_source_t order_source;
    cond_t non_interruptor;

    /* Set up two stores */

    region_t region = region_t::universe();

    mock_store_t backfiller_store;
    mock_store_t backfillee_store;

    in_memory_branch_history_manager_t branch_history_manager;
    branch_id_t dummy_branch_id = generate_uuid();
    {
        branch_birth_certificate_t dummy_branch;
        dummy_branch.region = region;
        dummy_branch.initial_timestamp = state_timestamp_t::zero();
        dummy_branch.origin = region_map_t<version_t>(region, version_t::zero());
        branch_history_manager.create_branch(dummy_branch_id, dummy_branch, &non_interruptor);
    }

    state_timestamp_t timestamp = state_timestamp_t::zero();

    // initialize the metainfo in a store
    store_view_t *stores[] = { &backfiller_store, &backfillee_store };
    for (size_t i = 0; i < sizeof(stores) / sizeof(stores[0]); i++) {
        write_token_t token;
        stores[i]->new_write_token(&token);
        stores[i]->set_metainfo(
            region_map_t<binary_blob_t>(
                region,
                binary_blob_t(version_t(dummy_branch_id, timestamp))),
            order_source.check_in(strprintf("set_metainfo(i=%zu)", i)),
            &token,
            &non_interruptor);
    }

    // Insert 10 values into both stores, then another 10 into only `backfiller_store` and not `backfillee_store`
    for (int i = 0; i < 20; i++) {
        write_response_t response;
        const std::string key = std::string(1, 'a' + randint(26));
        write_t w = mock_overwrite(key, strprintf("%d", i));

        for (int j = 0; j < (i < 10 ? 2 : 1); j++) {
            timestamp = timestamp.next();

            write_token_t token;
            backfiller_store.new_write_token(&token);

#ifndef NDEBUG
            equality_metainfo_checker_callback_t
                metainfo_checker_callback(
                    binary_blob_t(version_t(dummy_branch_id, timestamp.pred())));
            metainfo_checker_t metainfo_checker(&metainfo_checker_callback, region);
#endif

            backfiller_store.write(
                DEBUG_ONLY(metainfo_checker, )
                region_map_t<binary_blob_t>(
                    region,
                    binary_blob_t(version_t(dummy_branch_id, timestamp))
                ),
                w,
                &response, write_durability_t::SOFT,
                timestamp,
                order_source.check_in(strprintf("backfiller_store.write(j=%d)", j)),
                &token,
                &non_interruptor);
        }
    }

    // Set up a cluster so mailboxes can be created

    simple_mailbox_cluster_t cluster;

    /* Expose the backfiller to the cluster */

    backfiller_t backfiller(
        cluster.get_mailbox_manager(),
        &branch_history_manager,
        &backfiller_store);

    /* Run a backfill */

    {
        backfillee_t backfillee(
            cluster.get_mailbox_manager(),
            &branch_history_manager,
            &backfillee_store,
            backfiller.get_business_card(),
            backfill_config_t(),
            &non_interruptor);
        class callback_t : public backfillee_t::callback_t {
        public:
            bool on_progress(const region_map_t<version_t> &) THROWS_NOTHING {
                return true;
            }
        } callback;
        backfillee.go(
            &callback,
            key_range_t::right_bound_t(backfillee_store.get_region().inner.left),
            &non_interruptor);
    }

    /* Make sure everything got transferred properly */

    for (char c = 'a'; c <= 'z'; c++) {
        std::string key(1, c);
        EXPECT_EQ(backfiller_store.values(key), backfillee_store.values(key));
        EXPECT_TRUE(backfiller_store.timestamps(key) == backfillee_store.timestamps(key));
    }

    read_token_t token1;
    backfillee_store.new_read_token(&token1);

    region_map_t<binary_blob_t> blob_backfillee_metadata;
    backfillee_store.do_get_metainfo(order_source.check_in("backfillee_store.do_get_metainfo").with_read_mode(),
                                     &token1, &non_interruptor, &blob_backfillee_metadata);

    region_map_t<version_t> backfillee_metadata =
        to_version_map(blob_backfillee_metadata);

    read_token_t token2;
    backfiller_store.new_read_token(&token2);

    region_map_t<binary_blob_t> blob_backfiller_metadata;
    backfiller_store.do_get_metainfo(order_source.check_in("backfiller_store.do_get_metainfo").with_read_mode(),
                                     &token2, &non_interruptor, &blob_backfiller_metadata);

    region_map_t<version_t> backfiller_metadata =
        to_version_map(blob_backfiller_metadata);

    EXPECT_TRUE(backfillee_metadata == backfiller_metadata);

    //EXPECT_EQ(1, backfillee_metadata.size());
    //EXPECT_EQ(timestamp, backfillee_metadata[0].second.timestamp);
}

}   /* namespace unittest */
