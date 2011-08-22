#include "unittest/gtest.hpp"
#include "clustering/backfiller.hpp"
#include "clustering/backfillee.hpp"
#include "unittest/dummy_protocol.hpp"
#include "rpc/metadata/view/controller.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

void run_backfill_test() {

    /* Set up two stores */

    dummy_protocol_t::region_t region;
    for (char c = 'a'; c <= 'z'; c++) region.keys.insert(std::string(1, c));

    dummy_protocol_t::store_t backfiller_store(region), backfillee_store(region);

    /* Insert 10 values into both stores, then another 10 into only
    `backfiller_store` and not `backfillee_store` */

    repli_timestamp_t ts = repli_timestamp_t::distant_past;
    for (int i = 0; i < 20; i++) {
        dummy_protocol_t::write_t w;
        std::string key = std::string(1, 'a' + rand() % 26);
        w.values[key] = strprintf("%d", i);
        backfiller_store.write(w, ts, order_token_t::ignore);
        if (i < 10) backfillee_store.write(w, ts, order_token_t::ignore);
        ts = ts.next();
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
        &backfiller_store,
        backfiller_md_controller.get_view());

    /* Run a backfill */

    cond_t interruptor;
    backfill(&backfillee_store, &cluster, backfiller_md_controller.get_view(), &interruptor);

    /* Make sure everything got transferred properly */

    for (char c = 'a'; c <= 'z'; c++) {
        std::string key(1, c);
        EXPECT_EQ(backfiller_store.values[key], backfillee_store.values[key]);
        EXPECT_EQ(backfiller_store.timestamps[key], backfillee_store.timestamps[key]);
    }

    EXPECT_EQ(backfiller_store.get_timestamp(), backfillee_store.get_timestamp());
    EXPECT_TRUE(backfiller_store.is_coherent());
    EXPECT_TRUE(backfillee_store.is_coherent());
    EXPECT_FALSE(backfiller_store.is_backfilling());
    EXPECT_FALSE(backfillee_store.is_backfilling());
}
TEST(ClusteringBackfill, BackfillTest) {
    run_in_thread_pool(&run_backfill_test);
}

}   /* namespace unittest */
