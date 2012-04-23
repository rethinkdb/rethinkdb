#include <stdio.h>

#include "arch/arch.hpp"
#include "arch/os_signal.hpp"
#include "clustering/administration/auto_reconnect.hpp"
#include "clustering/administration/http/server.hpp"
#include "clustering/administration/issues/global.hpp"
#include "clustering/administration/issues/local_to_global.hpp"
#include "clustering/administration/issues/machine_down.hpp"
#include "clustering/administration/issues/name_conflict.hpp"
#include "clustering/administration/issues/pinnings_shards_mismatch.hpp"
#include "clustering/administration/issues/vector_clock_conflict.hpp"
#include "clustering/administration/logger.hpp"
#include "clustering/administration/main/initial_join.hpp"
#include "clustering/administration/main/serve.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/persist.hpp"
#include "clustering/administration/reactor_driver.hpp"
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
#include "rpc/semilattice/view/field.hpp"
#include "rpc/semilattice/view/member.hpp"
#include "rpc/semilattice/view/function.hpp"

bool serve(const std::string &filepath, const std::set<peer_address_t> &joins, int port, int client_port, machine_id_t machine_id, const cluster_semilattice_metadata_t &semilattice_metadata, std::string web_assets) {

    local_issue_tracker_t local_issue_tracker;

    log_writer_t log_writer(filepath + "/log_file", &local_issue_tracker);

    printf("Establishing cluster node on port %d...\n", port);

    connectivity_cluster_t connectivity_cluster; 
    message_multiplexer_t message_multiplexer(&connectivity_cluster);

    message_multiplexer_t::client_t mailbox_manager_client(&message_multiplexer, 'M');
    mailbox_manager_t mailbox_manager(&mailbox_manager_client);
    message_multiplexer_t::client_t::run_t mailbox_manager_client_run(&mailbox_manager_client, &mailbox_manager);

    message_multiplexer_t::client_t semilattice_manager_client(&message_multiplexer, 'S');
    semilattice_manager_t<cluster_semilattice_metadata_t> semilattice_manager_cluster(&semilattice_manager_client, semilattice_metadata);
    message_multiplexer_t::client_t::run_t semilattice_manager_client_run(&semilattice_manager_client, &semilattice_manager_cluster);

    log_server_t log_server(&mailbox_manager, &log_writer);

    // Initialize the stat manager before the directory manager so that we
    // could initialize the cluster directory metadata with the proper
    // stat_manager mailbox address
    stat_manager_t stat_manager(&mailbox_manager);

    cluster_directory_metadata_t cluster_directory_metadata_iv(
        machine_id,
        stat_manager.get_address(),
        log_server.get_business_card());

    message_multiplexer_t::client_t directory_manager_client(&message_multiplexer, 'D');
    directory_readwrite_manager_t<cluster_directory_metadata_t> directory_manager(&directory_manager_client, cluster_directory_metadata_iv);
    message_multiplexer_t::client_t::run_t directory_manager_client_run(&directory_manager_client, &directory_manager);

    message_multiplexer_t::run_t message_multiplexer_run(&message_multiplexer);
    connectivity_cluster_t::run_t connectivity_cluster_run(&connectivity_cluster, port, &message_multiplexer_run, client_port);

    auto_reconnector_t auto_reconnector(
        &connectivity_cluster,
        &connectivity_cluster_run,
        translate_into_watchable(directory_manager.get_root_view()->subview(field_lens(&cluster_directory_metadata_t::machine_id))),
        metadata_field(&cluster_semilattice_metadata_t::machines, semilattice_manager_cluster.get_root_view())
        );

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

    vector_clock_conflict_issue_tracker_t vector_clock_conflict_issue_tracker(
        semilattice_manager_cluster.get_root_view()
        );
    global_issue_aggregator_t::source_t vector_clock_conflict_issue_tracker_feed(&issue_aggregator, &vector_clock_conflict_issue_tracker);

    pinnings_shards_mismatch_issue_tracker_t<memcached_protocol_t> mc_pinnings_shards_mismatch_issue_tracker(
            metadata_field(&cluster_semilattice_metadata_t::memcached_namespaces, semilattice_manager_cluster.get_root_view())
            );
    global_issue_aggregator_t::source_t mc_pinnings_shards_mismatch_issue_tracker_feed(&issue_aggregator, &mc_pinnings_shards_mismatch_issue_tracker);

    pinnings_shards_mismatch_issue_tracker_t<mock::dummy_protocol_t> dummy_pinnings_shards_mismatch_issue_tracker(
            metadata_field(&cluster_semilattice_metadata_t::dummy_namespaces, semilattice_manager_cluster.get_root_view())
            );
    global_issue_aggregator_t::source_t dummy__pinnings_shards_mismatch_issue_tracker_feed(&issue_aggregator, &dummy_pinnings_shards_mismatch_issue_tracker);

    last_seen_tracker_t last_seen_tracker(
        metadata_field(&cluster_semilattice_metadata_t::machines, semilattice_manager_cluster.get_root_view()),
        translate_into_watchable(directory_manager.get_root_view()->subview(field_lens(&cluster_directory_metadata_t::machine_id)))
        );

    boost::scoped_ptr<initial_joiner_t> initial_joiner;
    if (!joins.empty()) {
        initial_joiner.reset(new initial_joiner_t(&connectivity_cluster, &connectivity_cluster_run, joins));
        initial_joiner->get_ready_signal()->wait_lazily_unordered();   /* TODO: Listen for `SIGINT`? */
    }

    metadata_persistence::semilattice_watching_persister_t persister(filepath, machine_id, semilattice_manager_cluster.get_root_view(), &local_issue_tracker);

    reactor_driver_t<mock::dummy_protocol_t> dummy_reactor_driver(&mailbox_manager,
                                                                  directory_manager.get_root_view()->subview(field_lens(&cluster_directory_metadata_t::dummy_namespaces)), 
                                                                  metadata_field(&cluster_semilattice_metadata_t::dummy_namespaces, semilattice_manager_cluster.get_root_view()),
                                                                  metadata_field(&cluster_semilattice_metadata_t::machines, semilattice_manager_cluster.get_root_view()),
                                                                  directory_manager.get_root_view()->subview(field_lens(&cluster_directory_metadata_t::machine_id)),
                                                                  filepath);

    reactor_driver_t<memcached_protocol_t> memcached_reactor_driver(&mailbox_manager,
                                                                    directory_manager.get_root_view()->subview(field_lens(&cluster_directory_metadata_t::memcached_namespaces)), 
                                                                    metadata_field(&cluster_semilattice_metadata_t::memcached_namespaces, semilattice_manager_cluster.get_root_view()),
                                                                    metadata_field(&cluster_semilattice_metadata_t::machines, semilattice_manager_cluster.get_root_view()),
                                                                    directory_manager.get_root_view()->subview(field_lens(&cluster_directory_metadata_t::machine_id)),
                                                                    filepath);

    mock::dummy_protocol_parser_maker_t dummy_parser_maker(&mailbox_manager, 
                                                           metadata_field(&cluster_semilattice_metadata_t::dummy_namespaces, semilattice_manager_cluster.get_root_view()),
                                                           directory_manager.get_root_view()->subview(field_lens(&cluster_directory_metadata_t::dummy_namespaces)));

    memcached_parser_maker_t mc_parser_maker(&mailbox_manager, 
                                             metadata_field(&cluster_semilattice_metadata_t::memcached_namespaces, semilattice_manager_cluster.get_root_view()),
#ifndef NDEBUG
                                             /* TODO: This will crash if we are declared dead. */
                                             metadata_function<deletable_t<machine_semilattice_metadata_t>, machine_semilattice_metadata_t>(boost::bind(&deletable_getter<machine_semilattice_metadata_t>, _1),
                                                               metadata_member(machine_id, 
                                                                               metadata_field(&machines_semilattice_metadata_t::machines, 
                                                                                              metadata_field(&cluster_semilattice_metadata_t::machines, 
                                                                                                             semilattice_manager_cluster.get_root_view())))),
#endif
                                             directory_manager.get_root_view()->subview(field_lens(&cluster_directory_metadata_t::memcached_namespaces)));

    administrative_http_server_manager_t administrative_http_interface(
        port + 1000,
        &mailbox_manager,
        semilattice_manager_cluster.get_root_view(),
        translate_into_watchable(directory_manager.get_root_view()),
        &issue_aggregator,
        &last_seen_tracker,
        machine_id,
        web_assets);

    printf("Server started; send SIGINT to stop.\n");

    wait_for_sigint();

    printf("Server got SIGINT; shutting down...\n");

    return true;
}
