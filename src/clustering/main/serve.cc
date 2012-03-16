#include "arch/arch.hpp"
#include "arch/os_signal.hpp"
#include "clustering/administration/http/server.hpp"
#include "clustering/administration/issues/global.hpp"
#include "clustering/administration/issues/local_to_global.hpp"
#include "clustering/administration/issues/machine_down.hpp"
#include "clustering/administration/issues/name_conflict.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/persist.hpp"
#include "clustering/administration/reactor_driver.hpp"
#include "clustering/main/serve.hpp"
#include "memcached/clustering.hpp"
#include "mock/dummy_protocol.hpp"
#include "mock/dummy_protocol_parser.hpp"
#include "rpc/connectivity/cluster.hpp"
#include "rpc/connectivity/cluster.hpp"
#include "rpc/connectivity/multiplexer.hpp"
#include "rpc/directory/manager.hpp"
#include "rpc/directory/watchable_copier.hpp"
#include "rpc/mailbox/mailbox.hpp"
#include "rpc/semilattice/semilattice_manager.hpp"

bool serve(const std::string &filepath, const std::vector<peer_address_t> &joins, int port, machine_id_t machine_id, const cluster_semilattice_metadata_t &semilattice_metadata) {

    local_issue_tracker_t local_issue_tracker;

    std::cout << "Establishing cluster node on port " << port << "..." << std::endl;

    connectivity_cluster_t connectivity_cluster; 
    message_multiplexer_t message_multiplexer(&connectivity_cluster);

    message_multiplexer_t::client_t mailbox_manager_client(&message_multiplexer, 'M');
    mailbox_manager_t mailbox_manager(&mailbox_manager_client);
    message_multiplexer_t::client_t::run_t mailbox_manager_client_run(&mailbox_manager_client, &mailbox_manager);

    message_multiplexer_t::client_t semilattice_manager_client(&message_multiplexer, 'S');
    semilattice_manager_t<cluster_semilattice_metadata_t> semilattice_manager_cluster(&semilattice_manager_client, semilattice_metadata);
    message_multiplexer_t::client_t::run_t semilattice_manager_client_run(&semilattice_manager_client, &semilattice_manager_cluster);

    message_multiplexer_t::client_t directory_manager_client(&message_multiplexer, 'D');
    directory_readwrite_manager_t<cluster_directory_metadata_t> directory_manager(&directory_manager_client, cluster_directory_metadata_t(machine_id));
    message_multiplexer_t::client_t::run_t directory_manager_client_run(&directory_manager_client, &directory_manager);

    message_multiplexer_t::run_t message_multiplexer_run(&message_multiplexer);
    connectivity_cluster_t::run_t connectivity_cluster_run(&connectivity_cluster, port, &message_multiplexer_run);

    watchable_write_copier_t<std::list<clone_ptr_t<local_issue_t> > > copy_local_issues_to_cluster(
        local_issue_tracker.get_issues_watchable(),
        directory_manager.get_root_view()->subview(field_lens(&cluster_directory_metadata_t::local_issues))
        );

    global_issue_aggregator_t issue_aggregator;

    remote_issue_collector_t remote_issue_tracker(
        translate_into_watchable(directory_manager.get_root_view()->subview(field_lens(&cluster_directory_metadata_t::local_issues))),
        directory_manager.get_root_view()->subview(field_lens(&cluster_directory_metadata_t::machine_id))
        );
    global_issue_aggregator_t::source_t remote_issue_tracker_feed(&issue_aggregator, &remote_issue_tracker);

    machine_down_issue_tracker_t machine_down_issue_tracker(
        semilattice_manager_cluster.get_root_view(),
        directory_manager.get_root_view()->subview(field_lens(&cluster_directory_metadata_t::machine_id))
        );
    global_issue_aggregator_t::source_t machine_down_issue_tracker_feed(&issue_aggregator, &machine_down_issue_tracker);

    name_conflict_issue_tracker_t name_conflict_issue_tracker(
        semilattice_manager_cluster.get_root_view()
        );
    global_issue_aggregator_t::source_t name_conflict_issue_tracker_feed(&issue_aggregator, &name_conflict_issue_tracker);

    for (int i = 0; i < (int)joins.size(); i++) {
        connectivity_cluster_run.join(joins[i]);
    }

    /* TODO: Warn if the `join()` didn't go through. */

    metadata_persistence::semilattice_watching_persister_t persister(filepath, machine_id, semilattice_manager_cluster.get_root_view(), &local_issue_tracker);

    reactor_driver_t<mock::dummy_protocol_t> dummy_reactor_driver(&mailbox_manager,
                                                                  directory_manager.get_root_view()->subview(field_lens(&cluster_directory_metadata_t::dummy_namespaces)), 
                                                                  metadata_field(&cluster_semilattice_metadata_t::dummy_namespaces, semilattice_manager_cluster.get_root_view()),
                                                                  directory_manager.get_root_view()->subview(field_lens(&cluster_directory_metadata_t::machine_id)),
                                                                  filepath);

    reactor_driver_t<memcached_protocol_t> memcached_reactor_driver(&mailbox_manager,
                                                                    directory_manager.get_root_view()->subview(field_lens(&cluster_directory_metadata_t::memcached_namespaces)), 
                                                                    metadata_field(&cluster_semilattice_metadata_t::memcached_namespaces, semilattice_manager_cluster.get_root_view()),
                                                                    directory_manager.get_root_view()->subview(field_lens(&cluster_directory_metadata_t::machine_id)),
                                                                    filepath);

    mock::dummy_protocol_parser_maker_t dummy_parser_maker(&mailbox_manager, 
                                                           metadata_field(&cluster_semilattice_metadata_t::dummy_namespaces, semilattice_manager_cluster.get_root_view()),
                                                           directory_manager.get_root_view()->subview(field_lens(&cluster_directory_metadata_t::dummy_namespaces)));

    memcached_parser_maker_t mc_parser_maker(&mailbox_manager, 
                                             metadata_field(&cluster_semilattice_metadata_t::memcached_namespaces, semilattice_manager_cluster.get_root_view()),
                                             directory_manager.get_root_view()->subview(field_lens(&cluster_directory_metadata_t::memcached_namespaces)));

    administrative_http_server_manager_t administrative_http_interface(
        port + 1000,
        semilattice_manager_cluster.get_root_view(),
        directory_manager.get_root_view(),
        &issue_aggregator,
        machine_id);

    std::cout << "Server started; send SIGINT to stop." << std::endl;

    wait_for_sigint();

    return true;
}
