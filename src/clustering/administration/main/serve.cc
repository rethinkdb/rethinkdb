
#include "arch/arch.hpp"
#include "arch/os_signal.hpp"
#include "clustering/administration/auto_reconnect.hpp"
#include "clustering/administration/http/server.hpp"
#include "clustering/administration/issues/global.hpp"
#include "clustering/administration/issues/local_to_global.hpp"
#include "clustering/administration/issues/machine_down.hpp"
#include "clustering/administration/issues/name_conflict.hpp"
#include "clustering/administration/issues/pinnings_shards_mismatch.hpp"
#include "clustering/administration/issues/unsatisfiable_goals.hpp"
#include "clustering/administration/issues/vector_clock_conflict.hpp"
#include "clustering/administration/logger.hpp"
#include "clustering/administration/main/initial_join.hpp"
#include "clustering/administration/main/serve.hpp"
#include "clustering/administration/main/watchable_fields.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/namespace_interface_repository.hpp"
#include "clustering/administration/parser_maker.hpp"
#include "clustering/administration/perfmon_collection_repo.hpp"
#include "clustering/administration/persist.hpp"
#include "clustering/administration/proc_stats.hpp"
#include "clustering/administration/reactor_driver.hpp"
#include <stdio.h>
#include "memcached/tcp_conn.hpp"
#include "mock/dummy_protocol.hpp"
#include "mock/dummy_protocol_parser.hpp"
#include "rpc/connectivity/cluster.hpp"
#include "rpc/connectivity/cluster.hpp"
#include "rpc/connectivity/multiplexer.hpp"
#include "rpc/directory/read_manager.hpp"
#include "rpc/directory/write_manager.hpp"
#include "rpc/mailbox/mailbox.hpp"
#include "rpc/semilattice/semilattice_manager.hpp"
#include "rpc/semilattice/view/field.hpp"
#include "rpc/semilattice/view/function.hpp"
#include "rpc/semilattice/view/member.hpp"

bool serve_(
    bool Iamaserver,
    // NB. For ordinary servers, filepath is the path to the database directory. For proxies, it is
    // the path of a log file.
    const std::string &filepath,
    const std::set<peer_address_t> &joins,
    int port, DEBUG_ONLY(int port_offset,) int client_port,
    machine_id_t machine_id, const cluster_semilattice_metadata_t &semilattice_metadata,
    std::string web_assets, signal_t *stop_cond)
{
    local_issue_tracker_t local_issue_tracker;

    log_writer_t log_writer(Iamaserver ? filepath + "/log_file" : filepath, &local_issue_tracker);

    printf("Establishing %s node on port %d...\n", Iamaserver ? "cluster" : "proxy", port);

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

    watchable_variable_t<cluster_directory_metadata_t> our_root_directory_variable(
        cluster_directory_metadata_t(
            machine_id,
            get_ips(),
            stat_manager.get_address(),
            log_server.get_business_card(),
            Iamaserver ? SERVER_PEER : PROXY_PEER
        ));

    message_multiplexer_t::client_t directory_manager_client(&message_multiplexer, 'D');
    directory_write_manager_t<cluster_directory_metadata_t> directory_write_manager(&directory_manager_client, our_root_directory_variable.get_watchable());
    directory_read_manager_t<cluster_directory_metadata_t> directory_read_manager(connectivity_cluster.get_connectivity_service());
    message_multiplexer_t::client_t::run_t directory_manager_client_run(&directory_manager_client, &directory_read_manager);

    message_multiplexer_t::run_t message_multiplexer_run(&message_multiplexer);
    connectivity_cluster_t::run_t connectivity_cluster_run(&connectivity_cluster, port, &message_multiplexer_run, client_port);

    auto_reconnector_t auto_reconnector(
        &connectivity_cluster,
        &connectivity_cluster_run,
        directory_read_manager.get_root_view()->subview(
            field_getter_t<machine_id_t, cluster_directory_metadata_t>(&cluster_directory_metadata_t::machine_id)),
        metadata_field(&cluster_semilattice_metadata_t::machines, semilattice_manager_cluster.get_root_view())
        );

    field_copier_t<std::list<clone_ptr_t<local_issue_t> >, cluster_directory_metadata_t> copy_local_issues_to_cluster(
        &cluster_directory_metadata_t::local_issues,
        local_issue_tracker.get_issues_watchable(),
        &our_root_directory_variable
        );

    global_issue_aggregator_t issue_aggregator;

    remote_issue_collector_t remote_issue_tracker(
        directory_read_manager.get_root_view()->subview(
            field_getter_t<std::list<clone_ptr_t<local_issue_t> >, cluster_directory_metadata_t>(&cluster_directory_metadata_t::local_issues)),
        directory_read_manager.get_root_view()->subview(
            field_getter_t<machine_id_t, cluster_directory_metadata_t>(&cluster_directory_metadata_t::machine_id))
        );
    global_issue_aggregator_t::source_t remote_issue_tracker_feed(&issue_aggregator, &remote_issue_tracker);

    machine_down_issue_tracker_t machine_down_issue_tracker(
        semilattice_manager_cluster.get_root_view(),
        directory_read_manager.get_root_view()->subview(
            field_getter_t<machine_id_t, cluster_directory_metadata_t>(&cluster_directory_metadata_t::machine_id))
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
    global_issue_aggregator_t::source_t dummy_pinnings_shards_mismatch_issue_tracker_feed(&issue_aggregator, &dummy_pinnings_shards_mismatch_issue_tracker);

    unsatisfiable_goals_issue_tracker_t unsatisfiable_goals_issue_tracker(
        semilattice_manager_cluster.get_root_view());
    global_issue_aggregator_t::source_t unsatisfiable_goals_issue_tracker_feed(&issue_aggregator, &unsatisfiable_goals_issue_tracker);

    last_seen_tracker_t last_seen_tracker(
        metadata_field(&cluster_semilattice_metadata_t::machines, semilattice_manager_cluster.get_root_view()),
        directory_read_manager.get_root_view()->subview(
            field_getter_t<machine_id_t, cluster_directory_metadata_t>(&cluster_directory_metadata_t::machine_id))
        );

    perfmon_collection_t proc_stats_collection("proc", NULL, true, true);
    proc_stats_collector_t proc_stats_collector(&proc_stats_collection);

    boost::scoped_ptr<initial_joiner_t> initial_joiner;
    if (!joins.empty()) {
        initial_joiner.reset(new initial_joiner_t(&connectivity_cluster, &connectivity_cluster_run, joins));
        try {
            wait_interruptible(initial_joiner->get_ready_signal(), stop_cond);
        } catch (interrupted_exc_t) {
            return false;
        }
    }

    perfmon_collection_repo_t perfmon_repo(NULL);

    // Reactor drivers
    boost::scoped_ptr<reactor_driver_t<mock::dummy_protocol_t> > dummy_reactor_driver(!Iamaserver ? 0 :
        new reactor_driver_t<mock::dummy_protocol_t>(
            &mailbox_manager,
            directory_read_manager.get_root_view()->subview(
                field_getter_t<namespaces_directory_metadata_t<mock::dummy_protocol_t>, cluster_directory_metadata_t>(&cluster_directory_metadata_t::dummy_namespaces)),
            metadata_field(&cluster_semilattice_metadata_t::dummy_namespaces, semilattice_manager_cluster.get_root_view()),
            metadata_field(&cluster_semilattice_metadata_t::machines, semilattice_manager_cluster.get_root_view()),
            directory_read_manager.get_root_view()->subview(
                field_getter_t<machine_id_t, cluster_directory_metadata_t>(&cluster_directory_metadata_t::machine_id)),
            filepath,
            &perfmon_repo));
    boost::scoped_ptr<field_copier_t<namespaces_directory_metadata_t<mock::dummy_protocol_t>, cluster_directory_metadata_t> >
        dummy_reactor_directory_copier(!Iamaserver ? 0 :
            new field_copier_t<namespaces_directory_metadata_t<mock::dummy_protocol_t>, cluster_directory_metadata_t>(
                &cluster_directory_metadata_t::dummy_namespaces,
                dummy_reactor_driver->get_watchable(),
                &our_root_directory_variable));

    boost::scoped_ptr<reactor_driver_t<memcached_protocol_t> > memcached_reactor_driver(!Iamaserver ? 0 :
        new reactor_driver_t<memcached_protocol_t>(
            &mailbox_manager,
            directory_read_manager.get_root_view()->subview(
                field_getter_t<namespaces_directory_metadata_t<memcached_protocol_t>, cluster_directory_metadata_t>(&cluster_directory_metadata_t::memcached_namespaces)),
            metadata_field(&cluster_semilattice_metadata_t::memcached_namespaces, semilattice_manager_cluster.get_root_view()),
            metadata_field(&cluster_semilattice_metadata_t::machines, semilattice_manager_cluster.get_root_view()),
            directory_read_manager.get_root_view()->subview(
                field_getter_t<machine_id_t, cluster_directory_metadata_t>(&cluster_directory_metadata_t::machine_id)),
            filepath,
            &perfmon_repo));
    boost::scoped_ptr<field_copier_t<namespaces_directory_metadata_t<memcached_protocol_t>, cluster_directory_metadata_t> >
        memcached_reactor_directory_copier(!Iamaserver ? 0 :
            new field_copier_t<namespaces_directory_metadata_t<memcached_protocol_t>, cluster_directory_metadata_t>(
                &cluster_directory_metadata_t::memcached_namespaces,
                memcached_reactor_driver->get_watchable(),
                &our_root_directory_variable));

    // Namespace repos
    namespace_repo_t<mock::dummy_protocol_t> dummy_namespace_repo(&mailbox_manager,
        directory_read_manager.get_root_view()->subview(
            field_getter_t<namespaces_directory_metadata_t<mock::dummy_protocol_t>, cluster_directory_metadata_t>(&cluster_directory_metadata_t::dummy_namespaces))
        );

    namespace_repo_t<memcached_protocol_t> memcached_namespace_repo(&mailbox_manager,
        directory_read_manager.get_root_view()->subview(
            field_getter_t<namespaces_directory_metadata_t<memcached_protocol_t>, cluster_directory_metadata_t>(&cluster_directory_metadata_t::memcached_namespaces))
        );

    parser_maker_t<mock::dummy_protocol_t, mock::dummy_protocol_parser_t> dummy_parser_maker(
        &mailbox_manager,
        metadata_field(&cluster_semilattice_metadata_t::dummy_namespaces, semilattice_manager_cluster.get_root_view()),
        DEBUG_ONLY(port_offset,)
        &dummy_namespace_repo,
        &perfmon_repo);

    parser_maker_t<memcached_protocol_t, memcache_listener_t> memcached_parser_maker(
        &mailbox_manager,
        metadata_field(&cluster_semilattice_metadata_t::memcached_namespaces, semilattice_manager_cluster.get_root_view()),
        DEBUG_ONLY(port_offset,)
        &memcached_namespace_repo,
        &perfmon_repo);

    boost::scoped_ptr<metadata_persistence::semilattice_watching_persister_t> persister(!Iamaserver ? 0 :
        new metadata_persistence::semilattice_watching_persister_t(
            filepath, machine_id, semilattice_manager_cluster.get_root_view(), &local_issue_tracker));

    {
        int http_port = port + 1000;
        guarantee(http_port < 65536);

        printf("Starting up administrative HTTP server on port %d...\n", http_port);
        administrative_http_server_manager_t administrative_http_interface(
            http_port,
            &mailbox_manager,
            semilattice_manager_cluster.get_root_view(),
            directory_read_manager.get_root_view(),
            &memcached_namespace_repo,
            &issue_aggregator,
            &last_seen_tracker,
            machine_id,
            web_assets);

        printf("Server started; send SIGINT to stop.\n");

        stop_cond->wait_lazily_unordered();

        printf("Server got SIGINT; shutting down...\n");
    }

    cond_t non_interruptor;
    if (persister)
        persister->stop_and_flush(&non_interruptor);

    return true;
}

bool serve(const std::string &filepath, const std::set<peer_address_t> &joins, int port, DEBUG_ONLY(int port_offset,) int client_port, machine_id_t machine_id, const cluster_semilattice_metadata_t &semilattice_metadata, std::string web_assets, signal_t *stop_cond) {
    return serve_(true, filepath, joins, port, DEBUG_ONLY(port_offset,) client_port, machine_id, semilattice_metadata, web_assets, stop_cond);
}

bool serve_proxy(const std::string &filepath, const std::set<peer_address_t> &joins, int port, DEBUG_ONLY(int port_offset,) int client_port, machine_id_t machine_id, const cluster_semilattice_metadata_t &semilattice_metadata, std::string web_assets, signal_t *stop_cond) {
    return serve_(false, filepath, joins, port, DEBUG_ONLY(port_offset,) client_port, machine_id, semilattice_metadata, web_assets, stop_cond);
}
