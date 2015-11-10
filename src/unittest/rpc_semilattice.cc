// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "clustering/administration/metadata.hpp"
#include "containers/archive/archive.hpp"
#include "rpc/semilattice/semilattice_manager.hpp"
#include "rpc/semilattice/joins/map.hpp"
#include "rpc/semilattice/view/field.hpp"
#include "rpc/semilattice/view/member.hpp"
#include "unittest/clustering_utils.hpp"
#include "unittest/dummy_metadata_controller.hpp"
#include "unittest/gtest.hpp"
#include "unittest/unittest_utils.hpp"


namespace unittest {

class sl_int_t {
public:
    sl_int_t() { }
    explicit sl_int_t(uint64_t initial) : i(initial) { }
    uint64_t i;
};

RDB_MAKE_SERIALIZABLE_1(sl_int_t, i);

inline void semilattice_join(sl_int_t *a, sl_int_t b) {
    a->i |= b.i;
}

class sl_pair_t {
public:
    sl_pair_t(sl_int_t _x, sl_int_t _y) : x(_x), y(_y) { }
    sl_int_t x, y;
};

void semilattice_join(sl_pair_t *a, sl_pair_t b) {
    semilattice_join(&a->x, b.x);
    semilattice_join(&a->y, b.y);
}

template<class T>
void assign(T *target, T value) {
    *target = value;
}

/* `SingleMetadata` tests metadata's properties on a single node. */
TPTEST(RPCSemilatticeTest, SingleMetadata, 2) {
    heartbeat_semilattice_metadata_t heartbeat_semilattice_metadata;
    dummy_semilattice_controller_t<heartbeat_semilattice_metadata_t>
        heartbeat_manager(heartbeat_semilattice_metadata);

    connectivity_cluster_t c;
    semilattice_manager_t<sl_int_t> slm(&c, 'S', sl_int_t(2));
    connectivity_cluster_t::run_t cr(&c, generate_uuid(), get_unittest_addresses(),
        peer_address_t(), ANY_PORT, 0, heartbeat_manager.get_view());

    /* Make sure that metadata works properly when passed to the constructor */
    EXPECT_EQ(2u, slm.get_root_view()->get().i);

    /* Make sure `join()` works properly */
    slm.get_root_view()->join(sl_int_t(1));

    EXPECT_EQ(3u, slm.get_root_view()->get().i);
}

/* `MetadataExchange` makes sure that metadata is correctly exchanged between
nodes. */
TPTEST(RPCSemilatticeTest, MetadataExchange, 2) {
    heartbeat_semilattice_metadata_t heartbeat_semilattice_metadata;
    dummy_semilattice_controller_t<heartbeat_semilattice_metadata_t>
        heartbeat_manager(heartbeat_semilattice_metadata);

    connectivity_cluster_t cluster1, cluster2;
    semilattice_manager_t<sl_int_t> slm1(&cluster1, 'S', sl_int_t(1)),
                                    slm2(&cluster2, 'S', sl_int_t(2));
    connectivity_cluster_t::run_t run1(&cluster1, generate_uuid(),
        get_unittest_addresses(), peer_address_t(), ANY_PORT, 0,
        heartbeat_manager.get_view());
    connectivity_cluster_t::run_t run2(&cluster2, generate_uuid(),
        get_unittest_addresses(), peer_address_t(), ANY_PORT, 0,
        heartbeat_manager.get_view());

    EXPECT_EQ(1u, slm1.get_root_view()->get().i);
    EXPECT_EQ(2u, slm2.get_root_view()->get().i);

    run1.join(get_cluster_local_address(&cluster2));

    /* Block until the connection is established */
    signal_timer_t timeout;
    timeout.start(1000);
    cluster1.get_connections()->run_all_until_satisfied(
        [](watchable_map_t<peer_id_t, connectivity_cluster_t::connection_pair_t> *map) {
            return map->get_all().size() == 2;
        }, &timeout);

    cond_t non_interruptor;
    slm1.get_root_view()->sync_from(cluster2.get_me(), &non_interruptor);
    EXPECT_EQ(3u, slm1.get_root_view()->get().i);

    slm1.get_root_view()->sync_to(cluster2.get_me(), &non_interruptor);
    EXPECT_EQ(3u, slm2.get_root_view()->get().i);

    slm1.get_root_view()->join(sl_int_t(4));
    EXPECT_EQ(7u, slm1.get_root_view()->get().i);

    slm1.get_root_view()->sync_to(cluster2.get_me(), &non_interruptor);
    EXPECT_EQ(7u, slm2.get_root_view()->get().i);
}

TPTEST(RPCSemilatticeTest, SyncFrom, 2) {
    heartbeat_semilattice_metadata_t heartbeat_semilattice_metadata;
    dummy_semilattice_controller_t<heartbeat_semilattice_metadata_t>
        heartbeat_manager(heartbeat_semilattice_metadata);

    connectivity_cluster_t cluster1, cluster2;
    semilattice_manager_t<sl_int_t> slm1(&cluster1, 'S', sl_int_t(1)),
                                    slm2(&cluster2, 'S', sl_int_t(2));
    connectivity_cluster_t::run_t run1(&cluster1, generate_uuid(),
        get_unittest_addresses(), peer_address_t(), ANY_PORT, 0,
        heartbeat_manager.get_view());
    connectivity_cluster_t::run_t run2(&cluster2, generate_uuid(),
        get_unittest_addresses(), peer_address_t(), ANY_PORT, 0,
        heartbeat_manager.get_view());

    EXPECT_EQ(1u, slm1.get_root_view()->get().i);
    EXPECT_EQ(2u, slm2.get_root_view()->get().i);

    cond_t non_interruptor;

    EXPECT_THROW(slm1.get_root_view()->sync_from(cluster2.get_me(), &non_interruptor), sync_failed_exc_t);
    EXPECT_THROW(slm1.get_root_view()->sync_to(cluster2.get_me(), &non_interruptor), sync_failed_exc_t);

    run1.join(get_cluster_local_address(&cluster2));

    /* Block until the connection is established */
    signal_timer_t timeout;
    timeout.start(1000);
    cluster1.get_connections()->run_all_until_satisfied(
        [](watchable_map_t<peer_id_t, connectivity_cluster_t::connection_pair_t> *map) {
            return map->get_all().size() == 2;
        }, &timeout);

    slm1.get_root_view()->sync_from(cluster2.get_me(), &non_interruptor);
    slm2.get_root_view()->sync_from(cluster1.get_me(), &non_interruptor);

    EXPECT_EQ(3u, slm1.get_root_view()->get().i);
    EXPECT_EQ(3u, slm2.get_root_view()->get().i);
}

/* `Watcher` makes sure that metadata watchers get notified when metadata
changes. */

TPTEST(RPCSemilatticeTest, Watcher, 2) {
    heartbeat_semilattice_metadata_t heartbeat_semilattice_metadata;
    dummy_semilattice_controller_t<heartbeat_semilattice_metadata_t>
        heartbeat_manager(heartbeat_semilattice_metadata);

    connectivity_cluster_t cluster;
    semilattice_manager_t<sl_int_t> slm(&cluster, 'S', sl_int_t(2));
    connectivity_cluster_t::run_t run(&cluster, generate_uuid(),
        get_unittest_addresses(), peer_address_t(), ANY_PORT, 0,
        heartbeat_manager.get_view());

    bool have_been_notified = false;
    semilattice_read_view_t<sl_int_t>::subscription_t watcher(
        std::bind(&assign<bool>, &have_been_notified, true),
        slm.get_root_view());

    slm.get_root_view()->join(sl_int_t(1));

    EXPECT_TRUE(have_been_notified);
}

/* `ViewController` tests `dummy_semilattice_controller_t`. */

TPTEST_MULTITHREAD(RPCSemilatticeTest, ViewController, 3) {
    dummy_semilattice_controller_t<sl_int_t> controller(sl_int_t(16));
    EXPECT_EQ(16u, controller.get_view()->get().i);

    controller.get_view()->join(sl_int_t(2));
    EXPECT_EQ(18u, controller.get_view()->get().i);

    bool have_been_notified = false;
    semilattice_read_view_t<sl_int_t>::subscription_t watcher(
        std::bind(&assign<bool>, &have_been_notified, true),
        controller.get_view());

    EXPECT_FALSE(have_been_notified);

    controller.get_view()->join(sl_int_t(1));
    EXPECT_EQ(19u, controller.get_view()->get().i);

    EXPECT_TRUE(have_been_notified);
}

/* `FieldView` tests `metadata_field()`. */
TPTEST_MULTITHREAD(RPCSemilatticeTest, FieldView, 3) {
    dummy_semilattice_controller_t<sl_pair_t> controller(
        sl_pair_t(sl_int_t(8), sl_int_t(4)));

    boost::shared_ptr<semilattice_read_view_t<sl_int_t> > x_view =
        metadata_field(&sl_pair_t::x, controller.get_view());

    EXPECT_EQ(8u, x_view->get().i);

    bool have_been_notified = false;
    semilattice_read_view_t<sl_int_t>::subscription_t watcher(
        std::bind(&assign<bool>, &have_been_notified, true),
        x_view);

    EXPECT_FALSE(have_been_notified);
    controller.get_view()->join(sl_pair_t(sl_int_t(1), sl_int_t(0)));
    EXPECT_TRUE(have_been_notified);

    EXPECT_EQ(9u, x_view->get().i);
}

/* `MemberView` tests `metadata_member()`. */

TPTEST_MULTITHREAD(RPCSemilatticeTest, MemberView, 3) {
    std::map<std::string, sl_int_t> initial_value;
    initial_value["foo"] = sl_int_t(8);
    dummy_semilattice_controller_t<std::map<std::string, sl_int_t> > controller(
        initial_value);

    boost::shared_ptr<semilattice_read_view_t<sl_int_t> > foo_view =
        metadata_member(std::string("foo"), controller.get_view());

    EXPECT_EQ(8u, foo_view->get().i);

    bool have_been_notified = false;
    semilattice_read_view_t<sl_int_t>::subscription_t watcher(
        std::bind(&assign<bool>, &have_been_notified, true),
        foo_view);

    EXPECT_FALSE(have_been_notified);
    std::map<std::string, sl_int_t> new_value;
    new_value["foo"] = sl_int_t(1);
    controller.get_view()->join(new_value);
    EXPECT_TRUE(have_been_notified);

    EXPECT_EQ(9u, foo_view->get().i);
}

}   /* namespace unittest */

#include "rpc/semilattice/semilattice_manager.tcc"
template class semilattice_manager_t<unittest::sl_int_t>;
