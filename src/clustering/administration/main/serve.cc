// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "clustering/administration/main/serve.hpp"

#include <stdio.h>

#include "arch/arch.hpp"
#include "arch/os_signal.hpp"
#include "clustering/administration/admin_tracker.hpp"
#include "clustering/administration/auto_reconnect.hpp"
#include "clustering/administration/http/server.hpp"
#include "clustering/administration/issues/local.hpp"
#include "clustering/administration/logger.hpp"
#include "clustering/administration/main/file_based_svs_by_namespace.hpp"
#include "clustering/administration/main/initial_join.hpp"
#include "clustering/administration/main/ports.hpp"
#include "clustering/administration/main/watchable_fields.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/namespace_interface_repository.hpp"
#include "clustering/administration/network_logger.hpp"
#include "clustering/administration/parser_maker.hpp"
#include "clustering/administration/perfmon_collection_repo.hpp"
#include "clustering/administration/persist.hpp"
#include "clustering/administration/proc_stats.hpp"
#include "clustering/administration/reactor_driver.hpp"
#include "clustering/administration/sys_stats.hpp"
#include "extproc/pool.hpp"
#include "memcached/tcp_conn.hpp"
#include "mock/dummy_protocol.hpp"
#include "mock/dummy_protocol_parser.hpp"
#include "rdb_protocol/parser.hpp"
#include "rdb_protocol/pb_server.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rpc/connectivity/cluster.hpp"
#include "rpc/connectivity/multiplexer.hpp"
#include "rpc/connectivity/heartbeat.hpp"
#include "rpc/directory/read_manager.hpp"
#include "rpc/directory/write_manager.hpp"
#include "rpc/semilattice/semilattice_manager.hpp"
#include "rpc/semilattice/view/field.hpp"

std::string service_address_ports_t::get_addresses_string() const {
    std::set<ip_address_t> actual_addresses = local_addresses;
    bool first = true;
    std::string result;

    // Get the actual list for printing if we're listening on all addresses.
    if (is_bind_all()) {
        actual_addresses = ip_address_t::get_local_addresses(std::set<ip_address_t>(), true);
    }

    for (std::set<ip_address_t>::const_iterator i = actual_addresses.begin(); i != actual_addresses.end(); ++i) {
        result += (first ? "" : ", " ) + i->as_dotted_decimal();
        first = false;
    }

    return result;
}

bool service_address_ports_t::is_bind_all() const {
    // If the set is empty, it means we're listening on all addresses.
    return local_addresses.empty();
}

bool do_serve(
    extproc::spawner_info_t *spawner_info,
    io_backender_t *io_backender,
    bool i_am_a_server,
    // NB. filepath & persistent_file are used iff i_am_a_server is true.
    const base_path_t &base_path,
    metadata_persistence::persistent_file_t *persistent_file,
    const peer_address_set_t &joins,
    service_address_ports_t address_ports,
    machine_id_t machine_id,
    const cluster_semilattice_metadata_t &semilattice_metadata,
    std::string web_assets,
    signal_t *stop_cond,
    const boost::optional<std::string> &config_file) {
    try {
        guarantee(spawner_info);
        extproc::pool_group_t extproc_pool_group(spawner_info, extproc::pool_group_t::DEFAULTS);

        local_issue_tracker_t local_issue_tracker;

        thread_pool_log_writer_t log_writer(&local_issue_tracker);

#ifndef NDEBUG
        logINF("Our machine ID is %s", uuid_to_str(machine_id).c_str());
#endif

        connectivity_cluster_t connectivity_cluster;
        message_multiplexer_t message_multiplexer(&connectivity_cluster);

        message_multiplexer_t::client_t heartbeat_manager_client(&message_multiplexer, 'H');
        heartbeat_manager_t heartbeat_manager(&heartbeat_manager_client);
        message_multiplexer_t::client_t::run_t heartbeat_manager_client_run(&heartbeat_manager_client, &heartbeat_manager);

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

        metadata_change_handler_t<cluster_semilattice_metadata_t> metadata_change_handler(&mailbox_manager, semilattice_manager_cluster.get_root_view());

        watchable_variable_t<cluster_directory_metadata_t> our_root_directory_variable(
            cluster_directory_metadata_t(
                machine_id,
                connectivity_cluster.get_me(),
                get_ips(),
                stat_manager.get_address(),
                metadata_change_handler.get_request_mailbox_address(),
                log_server.get_business_card(),
                i_am_a_server ? SERVER_PEER : PROXY_PEER));

        message_multiplexer_t::client_t directory_manager_client(&message_multiplexer, 'D');
        directory_write_manager_t<cluster_directory_metadata_t> directory_write_manager(&directory_manager_client, our_root_directory_variable.get_watchable());
        directory_read_manager_t<cluster_directory_metadata_t> directory_read_manager(connectivity_cluster.get_connectivity_service());
        message_multiplexer_t::client_t::run_t directory_manager_client_run(&directory_manager_client, &directory_read_manager);

        network_logger_t network_logger(
            connectivity_cluster.get_me(),
            directory_read_manager.get_root_view(),
            metadata_field(&cluster_semilattice_metadata_t::machines, semilattice_manager_cluster.get_root_view()));

        message_multiplexer_t::run_t message_multiplexer_run(&message_multiplexer);
        connectivity_cluster_t::run_t connectivity_cluster_run(&connectivity_cluster,
                                                               address_ports.local_addresses,
                                                               address_ports.port,
                                                               &message_multiplexer_run,
                                                               address_ports.client_port,
                                                               &heartbeat_manager);

        // If (0 == port), then we asked the OS to give us a port number.
        if (address_ports.port != 0) {
            guarantee(address_ports.port == connectivity_cluster_run.get_port());
        }
        logINF("Listening for intracluster connections on port %d\n", connectivity_cluster_run.get_port());

        auto_reconnector_t auto_reconnector(
            &connectivity_cluster,
            &connectivity_cluster_run,
            directory_read_manager.get_root_view()->subview(
                field_getter_t<machine_id_t, cluster_directory_metadata_t>(&cluster_directory_metadata_t::machine_id)),
            metadata_field(&cluster_semilattice_metadata_t::machines, semilattice_manager_cluster.get_root_view()));

        field_copier_t<std::list<local_issue_t>, cluster_directory_metadata_t> copy_local_issues_to_cluster(
            &cluster_directory_metadata_t::local_issues,
            local_issue_tracker.get_issues_watchable(),
            &our_root_directory_variable);

        admin_tracker_t admin_tracker(
            semilattice_manager_cluster.get_root_view(), directory_read_manager.get_root_view());

        perfmon_collection_t proc_stats_collection;
        perfmon_membership_t proc_stats_membership(&get_global_perfmon_collection(), &proc_stats_collection, "proc");

        proc_stats_collector_t proc_stats_collector(&proc_stats_collection);

        perfmon_collection_t sys_stats_collection;
        perfmon_membership_t sys_stats_membership(&get_global_perfmon_collection(), &sys_stats_collection, "sys");

        sys_stats_collector_t sys_stats_collector(base_path, &sys_stats_collection);

        scoped_ptr_t<initial_joiner_t> initial_joiner;
        if (!joins.empty()) {
            initial_joiner.init(new initial_joiner_t(&connectivity_cluster, &connectivity_cluster_run, joins));
            try {
                wait_interruptible(initial_joiner->get_ready_signal(), stop_cond);
            } catch (const interrupted_exc_t &) {
                return false;
            }
        }

        perfmon_collection_repo_t perfmon_repo(&get_global_perfmon_collection());

        // Namespace repos

        mock::dummy_protocol_t::context_t dummy_ctx;
        namespace_repo_t<mock::dummy_protocol_t> dummy_namespace_repo(&mailbox_manager,
            directory_read_manager.get_root_view()->subview(
                field_getter_t<namespaces_directory_metadata_t<mock::dummy_protocol_t>, cluster_directory_metadata_t>(&cluster_directory_metadata_t::dummy_namespaces)),
            &dummy_ctx);

        memcached_protocol_t::context_t mc_ctx;
        namespace_repo_t<memcached_protocol_t> memcached_namespace_repo(&mailbox_manager,
            directory_read_manager.get_root_view()->subview(
                field_getter_t<namespaces_directory_metadata_t<memcached_protocol_t>, cluster_directory_metadata_t>(&cluster_directory_metadata_t::memcached_namespaces)),
            &mc_ctx);

        rdb_protocol_t::context_t rdb_ctx(&extproc_pool_group,
                                          NULL,
                                          semilattice_manager_cluster.get_root_view(),
                                          &directory_read_manager,
                                          machine_id);

        namespace_repo_t<rdb_protocol_t> rdb_namespace_repo(&mailbox_manager,
            directory_read_manager.get_root_view()->subview(
                field_getter_t<namespaces_directory_metadata_t<rdb_protocol_t>, cluster_directory_metadata_t>(&cluster_directory_metadata_t::rdb_namespaces)),
            &rdb_ctx);

        //This is an annoying chicken and egg problem here
        rdb_ctx.ns_repo = &rdb_namespace_repo;

        {
            // Reactor drivers

            // Dummy
            file_based_svs_by_namespace_t<mock::dummy_protocol_t> dummy_svs_source(io_backender, base_path);
            scoped_ptr_t<reactor_driver_t<mock::dummy_protocol_t> > dummy_reactor_driver(!i_am_a_server ? NULL :
                new reactor_driver_t<mock::dummy_protocol_t>(
                    base_path,
                    io_backender,
                    &mailbox_manager,
                    directory_read_manager.get_root_view()->subview(
                        field_getter_t<namespaces_directory_metadata_t<mock::dummy_protocol_t>, cluster_directory_metadata_t>(&cluster_directory_metadata_t::dummy_namespaces)),
                    persistent_file->get_dummy_branch_history_manager(),
                    metadata_field(&cluster_semilattice_metadata_t::dummy_namespaces, semilattice_manager_cluster.get_root_view()),
                    metadata_field(&cluster_semilattice_metadata_t::machines, semilattice_manager_cluster.get_root_view()),
                    directory_read_manager.get_root_view()->subview(
                        field_getter_t<machine_id_t, cluster_directory_metadata_t>(&cluster_directory_metadata_t::machine_id)),
                    &dummy_svs_source,
                    &perfmon_repo,
                    NULL));
            scoped_ptr_t<field_copier_t<namespaces_directory_metadata_t<mock::dummy_protocol_t>, cluster_directory_metadata_t> >
                dummy_reactor_directory_copier(!i_am_a_server ? NULL :
                    new field_copier_t<namespaces_directory_metadata_t<mock::dummy_protocol_t>, cluster_directory_metadata_t>(
                        &cluster_directory_metadata_t::dummy_namespaces,
                        dummy_reactor_driver->get_watchable(),
                        &our_root_directory_variable));

            // Memcached
            file_based_svs_by_namespace_t<memcached_protocol_t> memcached_svs_source(io_backender, base_path);
            scoped_ptr_t<reactor_driver_t<memcached_protocol_t> > memcached_reactor_driver(!i_am_a_server ? NULL :
                new reactor_driver_t<memcached_protocol_t>(
                    base_path,
                    io_backender,
                    &mailbox_manager,
                    directory_read_manager.get_root_view()->subview(
                        field_getter_t<namespaces_directory_metadata_t<memcached_protocol_t>, cluster_directory_metadata_t>(&cluster_directory_metadata_t::memcached_namespaces)),
                    persistent_file->get_memcached_branch_history_manager(),
                    metadata_field(&cluster_semilattice_metadata_t::memcached_namespaces, semilattice_manager_cluster.get_root_view()),
                    metadata_field(&cluster_semilattice_metadata_t::machines, semilattice_manager_cluster.get_root_view()),
                    directory_read_manager.get_root_view()->subview(
                        field_getter_t<machine_id_t, cluster_directory_metadata_t>(&cluster_directory_metadata_t::machine_id)),
                    &memcached_svs_source,
                    &perfmon_repo,
                    NULL));
            scoped_ptr_t<field_copier_t<namespaces_directory_metadata_t<memcached_protocol_t>, cluster_directory_metadata_t> >
                memcached_reactor_directory_copier(!i_am_a_server ? NULL :
                    new field_copier_t<namespaces_directory_metadata_t<memcached_protocol_t>, cluster_directory_metadata_t>(
                        &cluster_directory_metadata_t::memcached_namespaces,
                        memcached_reactor_driver->get_watchable(),
                        &our_root_directory_variable));

            // RDB
            file_based_svs_by_namespace_t<rdb_protocol_t> rdb_svs_source(io_backender, base_path);
            scoped_ptr_t<reactor_driver_t<rdb_protocol_t> > rdb_reactor_driver(!i_am_a_server ? NULL :
                new reactor_driver_t<rdb_protocol_t>(
                    base_path,
                    io_backender,
                    &mailbox_manager,
                    directory_read_manager.get_root_view()->subview(
                        field_getter_t<namespaces_directory_metadata_t<rdb_protocol_t>, cluster_directory_metadata_t>(&cluster_directory_metadata_t::rdb_namespaces)),
                    persistent_file->get_rdb_branch_history_manager(),
                    metadata_field(&cluster_semilattice_metadata_t::rdb_namespaces, semilattice_manager_cluster.get_root_view()),
                    metadata_field(&cluster_semilattice_metadata_t::machines, semilattice_manager_cluster.get_root_view()),
                    directory_read_manager.get_root_view()->subview(
                        field_getter_t<machine_id_t, cluster_directory_metadata_t>(&cluster_directory_metadata_t::machine_id)),
                    &rdb_svs_source,
                    &perfmon_repo,
                    &rdb_ctx));
            scoped_ptr_t<field_copier_t<namespaces_directory_metadata_t<rdb_protocol_t>, cluster_directory_metadata_t> >
                rdb_reactor_directory_copier(!i_am_a_server ? NULL :
                    new field_copier_t<namespaces_directory_metadata_t<rdb_protocol_t>, cluster_directory_metadata_t>(
                        &cluster_directory_metadata_t::rdb_namespaces,
                        rdb_reactor_driver->get_watchable(),
                        &our_root_directory_variable));

            {
                parser_maker_t<mock::dummy_protocol_t, mock::dummy_protocol_parser_t> dummy_parser_maker(
                    &mailbox_manager,
                    metadata_field(&cluster_semilattice_metadata_t::dummy_namespaces, semilattice_manager_cluster.get_root_view()),
                    address_ports.local_addresses,
                    address_ports.port_offset,
                    &dummy_namespace_repo,
                    &local_issue_tracker,
                    &perfmon_repo);

                parser_maker_t<memcached_protocol_t, memcache_listener_t> memcached_parser_maker(
                    &mailbox_manager,
                    metadata_field(&cluster_semilattice_metadata_t::memcached_namespaces, semilattice_manager_cluster.get_root_view()),
                    address_ports.local_addresses,
                    address_ports.port_offset,
                    &memcached_namespace_repo,
                    &local_issue_tracker,
                    &perfmon_repo);

                rdb_protocol::query_http_app_t rdb_parser(semilattice_manager_cluster.get_root_view(), &rdb_namespace_repo);

                query2_server_t rdb_pb2_server(address_ports.local_addresses,
                                               address_ports.reql_port, &rdb_ctx);
                logINF("Listening for client driver connections on port %d\n",
                       rdb_pb2_server.get_port());

                scoped_ptr_t<metadata_persistence::semilattice_watching_persister_t> persister(!i_am_a_server ? NULL :
                    new metadata_persistence::semilattice_watching_persister_t(
                        persistent_file, semilattice_manager_cluster.get_root_view()));

                {
                    scoped_ptr_t<administrative_http_server_manager_t> admin_server_ptr;
                    if (address_ports.http_admin_is_disabled) {
                        logINF("Administrative HTTP connections are disabled.\n");
                    } else {
                        // TODO: Pardon me what, but is this how we fail here?
                        guarantee(address_ports.http_port < 65536);
                        admin_server_ptr.init(
                            new administrative_http_server_manager_t(
                                address_ports.local_addresses,
                                address_ports.http_port,
                                &mailbox_manager,
                                &metadata_change_handler,
                                semilattice_manager_cluster.get_root_view(),
                                directory_read_manager.get_root_view(),
                                &memcached_namespace_repo,
                                &rdb_namespace_repo,
                                &admin_tracker,
                                rdb_pb2_server.get_http_app(),
                                machine_id,
                                web_assets));
                        logINF("Listening for administrative HTTP connections on port %d\n", admin_server_ptr->get_port());
                    }

                    const std::string addresses_string = address_ports.get_addresses_string();
                    logINF("Listening on addresses: %s\n", addresses_string.c_str());

                    if (!address_ports.is_bind_all()) {
                        logINF("To fully expose RethinkDB on the network, bind to all addresses");
                        if(config_file) {
                            logINF("by adding `bind=all' to the config file (%s).", (*config_file).c_str());
                        } else {
                            logINF("by running rethinkdb with the `--bind all' command line option.");
                        }
                    }

                    logINF("Server ready\n");

                    stop_cond->wait_lazily_unordered();

                    logINF("Server got SIGINT; shutting down...\n");
                }

                cond_t non_interruptor;
                if (persister.has()) {
                    persister->stop_and_flush(&non_interruptor);
                }

                logINF("Shutting down client connections...\n");
            }
            logINF("All client connections closed.\n");

            logINF("Shutting down storage engine... (This may take a while if you had a lot of unflushed data in the writeback cache.)\n");
        }
        logINF("Storage engine shut down.\n");

        return true;

    } catch (const address_in_use_exc_t &e) {
        nice_crash("%s. Cannot bind to cluster port. Exiting.\n", e.what());
    }
}

bool serve(extproc::spawner_info_t *spawner_info,
           io_backender_t *io_backender,
           const base_path_t &base_path,
           metadata_persistence::persistent_file_t *persistent_file,
           const peer_address_set_t &joins,
           service_address_ports_t address_ports,
           machine_id_t machine_id,
           const cluster_semilattice_metadata_t &semilattice_metadata,
           std::string web_assets,
           signal_t *stop_cond,
           const boost::optional<std::string>& config_file) {
    return do_serve(spawner_info,
                    io_backender,
                    true,
                    base_path,
                    persistent_file,
                    joins,
                    address_ports,
                    machine_id,
                    semilattice_metadata,
                    web_assets,
                    stop_cond,
                    config_file);
}

bool serve_proxy(extproc::spawner_info_t *spawner_info,
                 const peer_address_set_t &joins,
                 service_address_ports_t address_ports,
                 machine_id_t machine_id,
                 const cluster_semilattice_metadata_t &semilattice_metadata,
                 std::string web_assets,
                 signal_t *stop_cond,
                 const boost::optional<std::string>& config_file) {
    // TODO: filepath doesn't _seem_ ignored.
    // filepath and persistent_file are ignored for proxies, so we use the empty string & NULL respectively.
    return do_serve(spawner_info,
                    NULL,
                    false,
                    base_path_t(""),
                    NULL,
                    joins,
                    address_ports,
                    machine_id,
                    semilattice_metadata,
                    web_assets,
                    stop_cond,
                    config_file);
}
