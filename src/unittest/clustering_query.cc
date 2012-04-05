#include "unittest/gtest.hpp"

#include "clustering/immediate_consistency/branch/broadcaster.hpp"
#include "clustering/immediate_consistency/branch/listener.hpp"
#include "clustering/immediate_consistency/branch/replier.hpp"
#include "clustering/immediate_consistency/query/master.hpp"
#include "clustering/immediate_consistency/query/namespace_interface.hpp"
#include "rpc/mailbox/mailbox.hpp"
#include "unittest/clustering_utils.hpp"
#include "unittest/dummy_metadata_controller.hpp"
#include "mock/dummy_protocol.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

/* The `ReadWrite` test sends some reads and writes to some shards via a
`cluster_namespace_interface_t`. */

static std::map<peer_id_t, std::map<master_id_t, master_business_card_t<dummy_protocol_t> > > wrap_in_peers_map(const std::map<master_id_t, master_business_card_t<dummy_protocol_t> > &peer_value, peer_id_t peer) {
    std::map<peer_id_t, std::map<master_id_t, master_business_card_t<dummy_protocol_t> > > map;
    map.insert(std::make_pair(peer, peer_value));
    return map;
}

static void run_read_write_test() {

    /* Set up a cluster so mailboxes can be created */
    simple_mailbox_cluster_t cluster;

    /* Set up metadata meeting-places */
    branch_history_t<dummy_protocol_t> initial_branch_metadata;
    dummy_semilattice_controller_t<branch_history_t<dummy_protocol_t> > branch_history_controller(initial_branch_metadata);

    /* Set up a branch */
    test_store_t<dummy_protocol_t> initial_store;
    cond_t interruptor;
    broadcaster_t<dummy_protocol_t> broadcaster(
        cluster.get_mailbox_manager(),
        branch_history_controller.get_view(),
        &initial_store.store,
        &interruptor
        );

    simple_directory_manager_t<boost::optional<broadcaster_business_card_t<dummy_protocol_t> > >
        broadcaster_metadata_controller(&cluster, 
            boost::optional<broadcaster_business_card_t<dummy_protocol_t> >(
                broadcaster.get_business_card()
            ));

    listener_t<dummy_protocol_t> initial_listener(
        cluster.get_mailbox_manager(),
        translate_into_watchable(broadcaster_metadata_controller.get_root_view()->
            get_peer_view(cluster.get_connectivity_service()->get_me())),
        branch_history_controller.get_view(),
        &broadcaster,
        &interruptor
        );

    replier_t<dummy_protocol_t> initial_replier(&initial_listener);

    /* Set up a metadata meeting-place for masters */
    std::map<master_id_t, master_business_card_t<dummy_protocol_t> > initial_master_metadata;
    watchable_variable_t<std::map<master_id_t, master_business_card_t<dummy_protocol_t> > > master_directory(initial_master_metadata);
    mutex_assertion_t master_directory_lock;

    /* Set up a master */
    master_t<dummy_protocol_t> master(cluster.get_mailbox_manager(), &master_directory, &master_directory_lock, a_thru_z_region(), &broadcaster);

    /* Set up a namespace dispatcher */
    cluster_namespace_interface_t<dummy_protocol_t> namespace_interface(
        cluster.get_mailbox_manager(),
        master_directory.get_watchable()->subview(boost::bind(&wrap_in_peers_map, _1, cluster.get_connectivity_service()->get_me()))
        );

    nap(100);

    /* Send some writes to the namespace */
    order_source_t order_source;
    std::map<std::string, std::string> inserter_state;
    test_inserter_t inserter(
        &namespace_interface,
        &order_source,
        &inserter_state);
    nap(100);
    inserter.stop();

    /* Now send some reads */
    for (std::map<std::string, std::string>::iterator it = inserter.values_inserted->begin();
            it != inserter.values_inserted->end(); it++) {
        dummy_protocol_t::read_t r;
        r.keys.keys.insert((*it).first);
        cond_t interruptor;
        dummy_protocol_t::read_response_t resp = namespace_interface.read(r, order_source.check_in("unittest"), &interruptor);
        EXPECT_EQ((*it).second, resp.values[(*it).first]);
    }
}

TEST(ClusteringNamespace, ReadWrite) {
    run_in_thread_pool(&run_read_write_test);
}

}   /* namespace unittest */

