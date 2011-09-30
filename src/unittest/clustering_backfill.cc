#include "unittest/gtest.hpp"
#include "clustering/immediate_consistency/branch/backfiller.hpp"
#include "clustering/immediate_consistency/branch/backfillee.hpp"
#include "rpc/metadata/view/controller.hpp"
#include "rpc/metadata/view/field.hpp"
#include "unittest/dummy_protocol.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

void run_backfill_test() {

    /* Set up two stores */

    dummy_protocol_t::region_t region;
    for (char c = 'a'; c <= 'z'; c++) {
        region.keys.insert(std::string(&c, 1));
    }

    dummy_underlying_store_t backfiller_underlying_store(region);
    dummy_store_view_t backfiller_store(&backfiller_underlying_store, region);
    dummy_underlying_store_t backfillee_underlying_store(region);
    dummy_store_view_t backfillee_store(&backfillee_underlying_store, region);

    /* Make a dummy metadata view to hold a branch tree so branch history checks
    can be performed */

    metadata_view_controller_t<namespace_branch_metadata_t<dummy_protocol_t> > namespace_metadata_controller(
        /* Parentheses prevent C++ from interpreting this as a function
        declaration */
        (namespace_branch_metadata_t<dummy_protocol_t>()));
    branch_id_t dummy_branch_id = generate_uuid();
    {
        branch_metadata_t<dummy_protocol_t> dummy_branch;
        dummy_branch.region = region;
        dummy_branch.initial_timestamp = state_timestamp_t::zero();
        dummy_branch.origin = region_map_t<dummy_protocol_t, version_range_t>(
            region, version_range_t(version_t(boost::uuids::nil_generator()(), state_timestamp_t::zero()))
            );
        std::map<branch_id_t, branch_metadata_t<dummy_protocol_t> > singleton_map;
        singleton_map[dummy_branch_id] = dummy_branch;
        metadata_field(&namespace_branch_metadata_t<dummy_protocol_t>::branches, namespace_metadata_controller.get_view())->join(singleton_map);
    }

    /* Insert 10 values into both stores, then another 10 into only
    `backfiller_store` and not `backfillee_store` */

    state_timestamp_t timestamp = state_timestamp_t::zero();
    for (int i = 0; i < 20; i++) {

        dummy_protocol_t::write_t w;
        std::string key = std::string(1, 'a' + rand() % 26);
        w.values[key] = strprintf("%d", i);

        transition_timestamp_t ts = transition_timestamp_t::starting_from(timestamp);
        timestamp = ts.timestamp_after();

        {
            cond_t interruptor;
            boost::shared_ptr<store_view_t<dummy_protocol_t>::write_transaction_t> txn =
                backfiller_store.begin_write_transaction(&interruptor);
            txn->set_metadata(region_map_t<dummy_protocol_t, binary_blob_t>(
                region,
                binary_blob_t(version_range_t(version_t(dummy_branch_id, timestamp)))
                ));
            txn->write(w, ts);
        }

        if (i < 10) {
            cond_t interruptor;
            boost::shared_ptr<store_view_t<dummy_protocol_t>::write_transaction_t> txn =
                backfillee_store.begin_write_transaction(&interruptor);
            txn->set_metadata(region_map_t<dummy_protocol_t, binary_blob_t>(
                region,
                binary_blob_t(version_range_t(version_t(dummy_branch_id, timestamp)))
                ));
            txn->write(w, ts);
        }
    }

    /* Set up a cluster so mailboxes can be created */

    class dummy_cluster_t : public mailbox_cluster_t {
    public:
        dummy_cluster_t(int port) : mailbox_cluster_t(port) { }
    private:
        void on_utility_message(peer_id_t, std::istream&, boost::function<void()>&) {
            ADD_FAILURE() << "no utility messages should be sent. WTF?";
        }
    } cluster(10000 + rand() % 20000);

    /* Expose the backfiller to the cluster */

    metadata_view_controller_t<resource_metadata_t<backfiller_metadata_t<dummy_protocol_t> > > backfiller_md_controller(
        (resource_metadata_t<backfiller_metadata_t<dummy_protocol_t> >()));

    backfiller_t<dummy_protocol_t> backfiller(
        &cluster,
        namespace_metadata_controller.get_view(),
        &backfiller_store,
        backfiller_md_controller.get_view());

    /* Run a backfill */

    cond_t interruptor;
    backfillee<dummy_protocol_t>(&cluster, namespace_metadata_controller.get_view(), &backfillee_store, backfiller_md_controller.get_view(), &interruptor);

    /* Make sure everything got transferred properly */

    for (char c = 'a'; c <= 'z'; c++) {
        std::string key(1, c);
        EXPECT_EQ(backfiller_underlying_store.values[key], backfillee_underlying_store.values[key]);
        EXPECT_TRUE(backfiller_underlying_store.timestamps[key] == backfillee_underlying_store.timestamps[key]);
    }

    std::vector<std::pair<dummy_protocol_t::region_t, version_range_t> > backfillee_metadata =
        region_map_transform<dummy_protocol_t, binary_blob_t, version_range_t>(
            backfillee_store.begin_read_transaction(&interruptor)->get_metadata(&interruptor),
            static_cast<const version_range_t &(*)(const binary_blob_t &)>(&binary_blob_t::get<version_range_t>)
            ).get_as_pairs();
    EXPECT_EQ(1, backfillee_metadata.size());
    EXPECT_TRUE(backfillee_metadata[0].second.is_coherent());
    EXPECT_EQ(timestamp, backfillee_metadata[0].second.earliest.timestamp);
}
TEST(ClusteringBackfill, BackfillTest) {
    run_in_thread_pool(&run_backfill_test);
}

}   /* namespace unittest */
