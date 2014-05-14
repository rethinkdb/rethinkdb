// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/main/serve.hpp"

#include <stdio.h>

#include "arch/arch.hpp"
#include "arch/os_signal.hpp"
#include "arch/io/network.hpp"
#include "clustering/administration/admin_tracker.hpp"
#include "clustering/administration/auto_reconnect.hpp"
#include "clustering/administration/http/server.hpp"
#include "clustering/administration/issues/local.hpp"
#include "clustering/administration/logger.hpp"
#include "clustering/administration/main/file_based_svs_by_namespace.hpp"
#include "clustering/administration/main/initial_join.hpp"
#include "clustering/administration/main/ports.hpp"
#include "clustering/administration/main/watchable_fields.hpp"
#include "containers/incremental_lenses.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/namespace_interface_repository.hpp"
#include "clustering/administration/network_logger.hpp"
#include "clustering/administration/perfmon_collection_repo.hpp"
#include "clustering/administration/persist.hpp"
#include "clustering/administration/proc_stats.hpp"
#include "clustering/administration/reactor_driver.hpp"
#include "clustering/administration/sys_stats.hpp"
#include "extproc/extproc_pool.hpp"
#include "rdb_protocol/query_server.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rpc/connectivity/cluster.hpp"
#include "rpc/connectivity/multiplexer.hpp"
#include "rpc/connectivity/heartbeat.hpp"
#include "rpc/directory/read_manager.hpp"
#include "rpc/directory/write_manager.hpp"
#include "rpc/semilattice/semilattice_manager.hpp"
#include "rpc/semilattice/view/field.hpp"
#include "buffer_cache/alt/cache_balancer.hpp"

peer_address_set_t look_up_peers_addresses(const std::vector<host_and_port_t> &names) {
    peer_address_set_t peers;
    for (size_t i = 0; i < names.size(); ++i) {
        peer_address_t peer(std::set<host_and_port_t>(&names[i], &names[i+1]));
        if (peers.find(peer) != peers.end()) {
            logWRN("Duplicate peer in --join parameters, ignoring: '%s:%d'",
                   names[i].host().c_str(), names[i].port().value());
        } else {
            peers.insert(peer);
        }
    }
    return peers;
}

std::string service_address_ports_t::get_addresses_string() const {
    std::set<ip_address_t> actual_addresses = local_addresses;
    bool first = true;
    std::string result;

    // Get the actual list for printing if we're listening on all addresses.
    if (is_bind_all()) {
        actual_addresses = get_local_ips(std::set<ip_address_t>(), true);
    }

    for (std::set<ip_address_t>::const_iterator i = actual_addresses.begin(); i != actual_addresses.end(); ++i) {
        result += (first ? "" : ", " ) + i->to_string();
        first = false;
    }

    return result;
}

bool service_address_ports_t::is_bind_all() const {
    // If the set is empty, it means we're listening on all addresses.
    return local_addresses.empty();
}

bool do_serve(io_backender_t *io_backender,
              bool i_am_a_server,
              // NB. filepath & persistent_file are used only if i_am_a_server is true.
              const base_path_t &base_path,
              metadata_persistence::cluster_persistent_file_t *cluster_metadata_file,
              metadata_persistence::auth_persistent_file_t *auth_metadata_file,
              uint64_t total_cache_size,
              const serve_info_t &serve_info,
              os_signal_cond_t *stop_cond) {
    try {
        extproc_pool_t extproc_pool(get_num_threads());

        local_issue_tracker_t local_issue_tracker;

        thread_pool_log_writer_t log_writer(&local_issue_tracker);

        cluster_semilattice_metadata_t cluster_metadata;
        auth_semilattice_metadata_t auth_metadata;
        machine_id_t machine_id = generate_uuid();

        if (cluster_metadata_file != NULL) {
            machine_id = cluster_metadata_file->read_machine_id();
            cluster_metadata = cluster_metadata_file->read_metadata();
        }
        if (auth_metadata_file != NULL) {
            auth_metadata = auth_metadata_file->read_metadata();
        }
#ifndef NDEBUG
        logINF("Our machine ID is %s", uuid_to_str(machine_id).c_str());
#endif

        connectivity_cluster_t connectivity_cluster;
        message_multiplexer_t message_multiplexer(&connectivity_cluster);

        message_multiplexer_t::client_t heartbeat_manager_client(&message_multiplexer, 'H', SEMAPHORE_NO_LIMIT);
        heartbeat_manager_t heartbeat_manager(&heartbeat_manager_client);
        message_multiplexer_t::client_t::run_t heartbeat_manager_client_run(&heartbeat_manager_client, &heartbeat_manager);

        message_multiplexer_t::client_t mailbox_manager_client(&message_multiplexer, 'M');
        mailbox_manager_t mailbox_manager(&mailbox_manager_client);
        message_multiplexer_t::client_t::run_t mailbox_manager_client_run(&mailbox_manager_client, &mailbox_manager);

        message_multiplexer_t::client_t semilattice_manager_client(&message_multiplexer, 'S');
        semilattice_manager_t<cluster_semilattice_metadata_t> semilattice_manager_cluster(&semilattice_manager_client, cluster_metadata);
        message_multiplexer_t::client_t::run_t semilattice_manager_client_run(&semilattice_manager_client, &semilattice_manager_cluster);

        message_multiplexer_t::client_t auth_manager_client(&message_multiplexer, 'A');
        semilattice_manager_t<auth_semilattice_metadata_t> auth_manager_cluster(&auth_manager_client, auth_metadata);
        message_multiplexer_t::client_t::run_t auth_manager_client_run(&auth_manager_client, &auth_manager_cluster);

        log_server_t log_server(&mailbox_manager, &log_writer);

        // Initialize the stat manager before the directory manager so that we
        // could initialize the cluster directory metadata with the proper
        // stat_manager mailbox address
        stat_manager_t stat_manager(&mailbox_manager);

        metadata_change_handler_t<cluster_semilattice_metadata_t> metadata_change_handler(&mailbox_manager, semilattice_manager_cluster.get_root_view());
        metadata_change_handler_t<auth_semilattice_metadata_t> auth_change_handler(&mailbox_manager, auth_manager_cluster.get_root_view());

        scoped_ptr_t<cluster_directory_metadata_t> initial_directory(
            new cluster_directory_metadata_t(machine_id,
                                             connectivity_cluster.get_me(),
                                             total_cache_size,
                                             get_ips(),
                                             stat_manager.get_address(),
                                             metadata_change_handler.get_request_mailbox_address(),
                                             auth_change_handler.get_request_mailbox_address(),
                                             log_server.get_business_card(),
                                             i_am_a_server ? SERVER_PEER : PROXY_PEER));

        watchable_variable_t<cluster_directory_metadata_t> our_root_directory_variable(*initial_directory);

        message_multiplexer_t::client_t directory_manager_client(&message_multiplexer, 'D');
        directory_write_manager_t<cluster_directory_metadata_t> directory_write_manager(&directory_manager_client, our_root_directory_variable.get_watchable());
        directory_read_manager_t<cluster_directory_metadata_t> directory_read_manager(connectivity_cluster.get_connectivity_service());
        message_multiplexer_t::client_t::run_t directory_manager_client_run(&directory_manager_client, &directory_read_manager);

        network_logger_t network_logger(
            connectivity_cluster.get_me(),
            directory_read_manager.get_root_view(),
            metadata_field(&cluster_semilattice_metadata_t::machines, semilattice_manager_cluster.get_root_view()));

        message_multiplexer_t::run_t message_multiplexer_run(&message_multiplexer);
        scoped_ptr_t<connectivity_cluster_t::run_t> connectivity_cluster_run;

        try {
            connectivity_cluster_run.init(new connectivity_cluster_t::run_t(
                &connectivity_cluster,
                serve_info.ports.local_addresses,
                serve_info.ports.canonical_addresses,
                serve_info.ports.port,
                &message_multiplexer_run,
                serve_info.ports.client_port,
                &heartbeat_manager));

            // Update the directory with the ip addresses that we are passing to peers
            std::set<ip_and_port_t> ips = connectivity_cluster_run->get_ips();
            initial_directory->ips.clear();
            for (auto it = ips.begin(); it != ips.end(); ++it) {
                initial_directory->ips.push_back(it->ip().to_string());
            }
            our_root_directory_variable.set_value(*initial_directory);
            initial_directory.reset();
        } catch (const address_in_use_exc_t &ex) {
            throw address_in_use_exc_t(strprintf("Could not bind to cluster port: %s", ex.what()));
        }

        // If (0 == port), then we asked the OS to give us a port number.
        if (serve_info.ports.port != 0) {
            guarantee(serve_info.ports.port == connectivity_cluster_run->get_port());
        }
        logINF("Listening for intracluster connections on port %d\n", connectivity_cluster_run->get_port());

        auto_reconnector_t auto_reconnector(
            &connectivity_cluster,
            connectivity_cluster_run.get(),
            directory_read_manager.get_root_view()->incremental_subview(
                incremental_field_getter_t<machine_id_t, cluster_directory_metadata_t>(&cluster_directory_metadata_t::machine_id)),
            metadata_field(&cluster_semilattice_metadata_t::machines, semilattice_manager_cluster.get_root_view()));

        field_copier_t<std::list<local_issue_t>, cluster_directory_metadata_t> copy_local_issues_to_cluster(
            &cluster_directory_metadata_t::local_issues,
            local_issue_tracker.get_issues_watchable(),
            &our_root_directory_variable);

        admin_tracker_t admin_tracker(semilattice_manager_cluster.get_root_view(),
                                      auth_manager_cluster.get_root_view(),
                                      directory_read_manager.get_root_view());

        perfmon_collection_t proc_stats_collection;
        perfmon_membership_t proc_stats_membership(&get_global_perfmon_collection(), &proc_stats_collection, "proc");

        proc_stats_collector_t proc_stats_collector(&proc_stats_collection);

        scoped_ptr_t<perfmon_collection_t> sys_stats_collection;
        scoped_ptr_t<perfmon_membership_t> sys_stats_membership;
        scoped_ptr_t<sys_stats_collector_t> sys_stats_collector;
        if (i_am_a_server) {
            sys_stats_collection.init(new perfmon_collection_t);
            sys_stats_membership.init(new perfmon_membership_t(
                &get_global_perfmon_collection(),
                sys_stats_collection.get(),
                "sys"));
            sys_stats_collector.init(new sys_stats_collector_t(
                base_path, sys_stats_collection.get()));
        }

        scoped_ptr_t<initial_joiner_t> initial_joiner;
        if (!serve_info.peers.empty()) {
            initial_joiner.init(new initial_joiner_t(&connectivity_cluster,
                                                     connectivity_cluster_run.get(),
                                                     serve_info.peers));
            try {
                wait_interruptible(initial_joiner->get_ready_signal(), stop_cond);
            } catch (const interrupted_exc_t &) {
                return false;
            }
        }

        perfmon_collection_repo_t perfmon_repo(&get_global_perfmon_collection());

        // Namespace repo
        rdb_context_t rdb_ctx(&extproc_pool,
                              NULL,
                              semilattice_manager_cluster.get_root_view(),
                              auth_manager_cluster.get_root_view(),
                              &directory_read_manager,
                              machine_id,
                              &get_global_perfmon_collection(),
                              serve_info.reql_http_proxy);

        namespace_repo_t rdb_namespace_repo(&mailbox_manager,
            metadata_field(&cluster_semilattice_metadata_t::rdb_namespaces,
                           semilattice_manager_cluster.get_root_view()),
            directory_read_manager.get_root_view()->incremental_subview(
                incremental_field_getter_t<namespaces_directory_metadata_t, cluster_directory_metadata_t>(&cluster_directory_metadata_t::rdb_namespaces)),
            &rdb_ctx);

        //This is an annoying chicken and egg problem here
        rdb_ctx.ns_repo = &rdb_namespace_repo;

        {
            scoped_ptr_t<cache_balancer_t> cache_balancer;

            if (i_am_a_server) {
                // Proxies do not have caches to balance
                cache_balancer.init(new alt_cache_balancer_t(total_cache_size));
            }

            // Reactor drivers

            // RDB
            scoped_ptr_t<file_based_svs_by_namespace_t> rdb_svs_source;
            scoped_ptr_t<reactor_driver_t> rdb_reactor_driver;
            scoped_ptr_t<field_copier_t<namespaces_directory_metadata_t, cluster_directory_metadata_t> >
                rdb_reactor_directory_copier;

            if (i_am_a_server) {
                rdb_svs_source.init(new file_based_svs_by_namespace_t(
                    io_backender, cache_balancer.get(), base_path));
                rdb_reactor_driver.init(new reactor_driver_t(
                        base_path,
                        io_backender,
                        &mailbox_manager,
                        directory_read_manager.get_root_view()->incremental_subview(
                            incremental_field_getter_t<namespaces_directory_metadata_t,
                                                       cluster_directory_metadata_t>(&cluster_directory_metadata_t::rdb_namespaces)),
                        cluster_metadata_file->get_rdb_branch_history_manager(),
                        metadata_field(&cluster_semilattice_metadata_t::rdb_namespaces,
                                       semilattice_manager_cluster.get_root_view()),
                        metadata_field(&cluster_semilattice_metadata_t::machines,
                                       semilattice_manager_cluster.get_root_view()),
                        directory_read_manager.get_root_view()->incremental_subview(
                            incremental_field_getter_t<machine_id_t,
                                                       cluster_directory_metadata_t>(&cluster_directory_metadata_t::machine_id)),
                        rdb_svs_source.get(),
                        &perfmon_repo,
                        &rdb_ctx));
                rdb_reactor_directory_copier.init(new field_copier_t<namespaces_directory_metadata_t, cluster_directory_metadata_t>(
                    &cluster_directory_metadata_t::rdb_namespaces,
                    rdb_reactor_driver->get_watchable(),
                    &our_root_directory_variable));
            }

            {
                rdb_query_server_t rdb_query_server(serve_info.ports.local_addresses,
                                                    serve_info.ports.reql_port, &rdb_ctx);
                logINF("Listening for client driver connections on port %d\n",
                       rdb_query_server.get_port());

                scoped_ptr_t<metadata_persistence::semilattice_watching_persister_t<cluster_semilattice_metadata_t> >
                    cluster_metadata_persister;
                scoped_ptr_t<metadata_persistence::semilattice_watching_persister_t<auth_semilattice_metadata_t> >
                    auth_metadata_persister;

                if (i_am_a_server) {
                    cluster_metadata_persister.init(new metadata_persistence::semilattice_watching_persister_t<cluster_semilattice_metadata_t>(
                        cluster_metadata_file,
                        semilattice_manager_cluster.get_root_view()));
                    auth_metadata_persister.init(new metadata_persistence::semilattice_watching_persister_t<auth_semilattice_metadata_t>(
                        auth_metadata_file,
                        auth_manager_cluster.get_root_view()));
                }

                {
                    scoped_ptr_t<administrative_http_server_manager_t> admin_server_ptr;
                    if (serve_info.ports.http_admin_is_disabled) {
                        logINF("Administrative HTTP connections are disabled.\n");
                    } else {
                        // TODO: Pardon me what, but is this how we fail here?
                        guarantee(serve_info.ports.http_port < 65536);
                        admin_server_ptr.init(
                            new administrative_http_server_manager_t(
                                serve_info.ports.local_addresses,
                                serve_info.ports.http_port,
                                &mailbox_manager,
                                &metadata_change_handler,
                                &auth_change_handler,
                                semilattice_manager_cluster.get_root_view(),
                                directory_read_manager.get_root_view(),
                                &rdb_namespace_repo,
                                &admin_tracker,
                                rdb_query_server.get_http_app(),
                                machine_id,
                                serve_info.web_assets));
                        logINF("Listening for administrative HTTP connections on port %d\n",
                               admin_server_ptr->get_port());
                    }

                    const std::string addresses_string = serve_info.ports.get_addresses_string();
                    logINF("Listening on addresses: %s\n", addresses_string.c_str());

                    if (!serve_info.ports.is_bind_all()) {
                        logINF("To fully expose RethinkDB on the network, bind to all addresses");
                        if(serve_info.config_file) {
                            logINF("by adding `bind=all' to the config file (%s).",
                                   (*serve_info.config_file).c_str());
                        } else {
                            logINF("by running rethinkdb with the `--bind all` command line option.");
                        }
                    }

                    logINF("Server ready\n");

                    stop_cond->wait_lazily_unordered();


                    if (stop_cond->get_source_signo() == SIGINT) {
                        logINF("Server got SIGINT from pid %d, uid %d; shutting down...\n",
                               stop_cond->get_source_pid(), stop_cond->get_source_uid());
                    } else if (stop_cond->get_source_signo() == SIGTERM) {
                        logINF("Server got SIGTERM from pid %d, uid %d; shutting down...\n",
                               stop_cond->get_source_pid(), stop_cond->get_source_uid());

                    } else {
                        logINF("Server got signal %d from pid %d, uid %d; shutting down...\n",
                               stop_cond->get_source_signo(),
                               stop_cond->get_source_pid(), stop_cond->get_source_uid());
                    }

                }

                cond_t non_interruptor;
                if (i_am_a_server) {
                    cluster_metadata_persister->stop_and_flush(&non_interruptor);
                    auth_metadata_persister->stop_and_flush(&non_interruptor);
                }

                logINF("Shutting down client connections...\n");
            }
            logINF("All client connections closed.\n");

            logINF("Shutting down storage engine... (This may take a while if you had a lot of unflushed data in the writeback cache.)\n");
        }
        logINF("Storage engine shut down.\n");

    } catch (const address_in_use_exc_t &ex) {
        logERR("%s.\n", ex.what());
        return false;
    } catch (const tcp_socket_exc_t &ex) {
        logERR("%s.\n", ex.what());
        return false;
    }

    return true;
}

bool serve(io_backender_t *io_backender,
           const base_path_t &base_path,
           metadata_persistence::cluster_persistent_file_t *cluster_persistent_file,
           metadata_persistence::auth_persistent_file_t *auth_persistent_file,
           uint64_t total_cache_size,
           const serve_info_t &serve_info,
           os_signal_cond_t *stop_cond) {
    return do_serve(io_backender,
                    true,
                    base_path,
                    cluster_persistent_file,
                    auth_persistent_file,
                    total_cache_size,
                    serve_info,
                    stop_cond);
}

bool serve_proxy(const serve_info_t &serve_info,
                 os_signal_cond_t *stop_cond) {
    // TODO: filepath doesn't _seem_ ignored.
    // filepath and persistent_file are ignored for proxies, so we use the empty string & NULL respectively.
    return do_serve(NULL,
                    false,
                    base_path_t(""),
                    NULL,
                    NULL,
                    0,
                    serve_info,
                    stop_cond);
}
