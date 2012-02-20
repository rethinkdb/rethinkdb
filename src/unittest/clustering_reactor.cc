#include "unittest/gtest.hpp"
#include "unittest/clustering_utils.hpp"
#include "unittest/dummy_metadata_controller.hpp"
#include "unittest/dummy_protocol.hpp"
#include "unittest/unittest_utils.hpp"
#include "rpc/connectivity/multiplexer.hpp"
#include "rpc/semilattice/semilattice_manager.hpp"
#include "rpc/directory/manager.hpp"
#include "clustering/reactor/directory_echo.hpp"
#include "clustering/reactor/metadata.hpp"
#include "clustering/reactor/blueprint.hpp"
#include "concurrency/watchable.hpp"
#include "clustering/reactor/reactor.hpp"
#include "clustering/immediate_consistency/query/namespace_interface.hpp"

namespace unittest {

namespace {

/* `let_stuff_happen()` delays for some time to let events occur */
void let_stuff_happen() {
    nap(1000);

}


class test_cluster_directory_t {
public:
    boost::optional<directory_echo_wrapper_t<reactor_business_card_t<dummy_protocol_t> > >  reactor_directory;
    std::map<master_id_t, master_business_card_t<dummy_protocol_t> > master_directory;

    RDB_MAKE_ME_SERIALIZABLE_2(reactor_directory, master_directory);
};

/* This is a cluster that is useful for reactor testing... but doesn't actually
 * have a reactor due to the annoyance of needing the peer ids to create a
 * correct blueprint. */
class reactor_test_cluster_t {
public:
    reactor_test_cluster_t(int port, dummy_underlying_store_t *dummy_underlying_store) :
        connectivity_cluster(),
        message_multiplexer(&connectivity_cluster),

        mailbox_manager_client(&message_multiplexer, 'M'),
        mailbox_manager(&mailbox_manager_client),
        mailbox_manager_client_run(&mailbox_manager_client, &mailbox_manager),

        semilattice_manager_client(&message_multiplexer, 'S'),
        semilattice_manager_branch_history(&semilattice_manager_client, branch_history_t<dummy_protocol_t>()),
        semilattice_manager_client_run(&semilattice_manager_client, &semilattice_manager_branch_history),

        directory_manager_client(&message_multiplexer, 'D'),
        directory_manager(&directory_manager_client, test_cluster_directory_t()),
        directory_manager_client_run(&directory_manager_client, &directory_manager),

        message_multiplexer_run(&message_multiplexer),
        connectivity_cluster_run(&connectivity_cluster, port, &message_multiplexer_run),
        dummy_store_view(dummy_underlying_store, a_thru_z_region())
        { }

    peer_id_t get_me() {
        return connectivity_cluster.get_me();
    }

    connectivity_cluster_t connectivity_cluster;
    message_multiplexer_t message_multiplexer;

    message_multiplexer_t::client_t mailbox_manager_client;
    mailbox_manager_t mailbox_manager;
    message_multiplexer_t::client_t::run_t mailbox_manager_client_run;

    message_multiplexer_t::client_t semilattice_manager_client;
    semilattice_manager_t<branch_history_t<dummy_protocol_t> > semilattice_manager_branch_history;
    message_multiplexer_t::client_t::run_t semilattice_manager_client_run;

    message_multiplexer_t::client_t directory_manager_client;
    directory_readwrite_manager_t<test_cluster_directory_t> directory_manager;
    message_multiplexer_t::client_t::run_t directory_manager_client_run;

    message_multiplexer_t::run_t message_multiplexer_run;
    connectivity_cluster_t::run_t connectivity_cluster_run;

    dummy_store_view_t dummy_store_view;
};

class test_reactor_t {
public:
    test_reactor_t(reactor_test_cluster_t *r, const blueprint_t<dummy_protocol_t> &initial_blueprint)
        : blueprint_watchable(initial_blueprint),
          reactor(&r->mailbox_manager, r->directory_manager.get_root_view()->subview(field_lens(&test_cluster_directory_t::reactor_directory)), 
                  r->directory_manager.get_root_view()->subview(field_lens(&test_cluster_directory_t::master_directory)),
                  r->semilattice_manager_branch_history.get_root_view(), &blueprint_watchable, &r->dummy_store_view)
    { }

    watchable_impl_t<blueprint_t<dummy_protocol_t> > blueprint_watchable;
    reactor_t<dummy_protocol_t> reactor;
};


void run_queries(reactor_test_cluster_t *reactor_test_cluster) {
    cluster_namespace_interface_t<dummy_protocol_t> namespace_if(&reactor_test_cluster->mailbox_manager, 
                                                                 reactor_test_cluster->directory_manager.get_root_view()->subview(field_lens(&test_cluster_directory_t::master_directory)));

    order_source_t order_source;
    inserter_t inserter(&namespace_if, &order_source);
    let_stuff_happen();
    inserter.stop();
    inserter.validate();
}

class test_cluster_group_t {
public:
    boost::ptr_vector<dummy_underlying_store_t> stores;
    boost::ptr_vector<reactor_test_cluster_t> test_clusters;

    boost::ptr_vector<test_reactor_t> test_reactors;

    test_cluster_group_t(int n_machines) {
        int port = 10000 + rand() % 20000;
        for (int i = 0; i < n_machines; i++) {
            stores.push_back(new dummy_underlying_store_t(a_thru_z_region()));
            stores.back().metainfo.set(a_thru_z_region(), binary_blob_t(version_range_t(version_t::zero())));

            test_clusters.push_back(new reactor_test_cluster_t(port + i, &stores[i]));
            if (i > 0) {
                test_clusters[0].connectivity_cluster_run.join(test_clusters[i].connectivity_cluster.get_peer_address(test_clusters[i].connectivity_cluster.get_me()));
            }
        }
    }

    void construct_all_reactors(const blueprint_t<dummy_protocol_t> &bp) {
        for (unsigned i = 0; i < test_clusters.size(); i++) {
            test_reactors.push_back(new test_reactor_t(&test_clusters[i], bp));
        }
    }

    peer_id_t get_peer_id(unsigned i) {
        rassert(i < test_clusters.size());
        return test_clusters[i].get_me();
    }
};

}   /* anonymous namespace */

void runOneShardOnePrimaryOneNodeStartupShutdowntest() {
    test_cluster_group_t cluster_group(1);

    blueprint_t<dummy_protocol_t> blueprint;
    blueprint.add_peer(cluster_group.get_peer_id(0));
    blueprint.add_role(cluster_group.get_peer_id(0), a_thru_z_region(), blueprint_t<dummy_protocol_t>::role_primary);

    cluster_group.construct_all_reactors(blueprint);
    let_stuff_happen();
    run_queries(&cluster_group.test_clusters[0]);
}

TEST(ClusteringReactor, OneShardOnePrimaryOneNodeStartupShutdown) {
    run_in_thread_pool(&runOneShardOnePrimaryOneNodeStartupShutdowntest);
}

void runOneShardOnePrimaryOneSecondaryStartupShutdowntest() {
    int port = 10000 + rand() % 20000;

    dummy_underlying_store_t primary_store(a_thru_z_region()), secondary_store(a_thru_z_region());
    primary_store.metainfo.set(a_thru_z_region(), binary_blob_t(version_range_t(version_t::zero())));
    secondary_store.metainfo.set(a_thru_z_region(), binary_blob_t(version_range_t(version_t::zero())));

    reactor_test_cluster_t primary_cluster(port, &primary_store), secondary_cluster(port+1, &secondary_store);
    primary_cluster.connectivity_cluster_run.join(secondary_cluster.connectivity_cluster.get_peer_address(secondary_cluster.connectivity_cluster.get_me()));


    blueprint_t<dummy_protocol_t> blueprint;
    blueprint.add_peer(primary_cluster.get_me());
    blueprint.add_role(primary_cluster.get_me(), a_thru_z_region(), blueprint_t<dummy_protocol_t>::role_primary);

    blueprint.add_peer(secondary_cluster.get_me());
    blueprint.add_role(secondary_cluster.get_me(), a_thru_z_region(), blueprint_t<dummy_protocol_t>::role_secondary);

    test_reactor_t primary_reactor(&primary_cluster, blueprint);
    test_reactor_t secondary_reactor(&secondary_cluster, blueprint);

    let_stuff_happen();

    run_queries(&primary_cluster);
}

TEST(ClusteringReactor, runOneShardOnePrimaryOneSecondaryStartupShutdowntest) {
    run_in_thread_pool(&runOneShardOnePrimaryOneSecondaryStartupShutdowntest);
}

void runTwoShardsTwoNodes() {
}

TEST(ClusteringReactor, runTwoShardsTwoNodes) {
    run_in_thread_pool(&runTwoShardsTwoNodes);
}


} // namespace unittest
