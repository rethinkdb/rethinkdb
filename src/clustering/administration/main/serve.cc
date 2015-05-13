// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/main/serve.hpp"

#include <stdio.h>

#include "arch/arch.hpp"
#include "arch/io/network.hpp"
#include "arch/os_signal.hpp"
#include "buffer_cache/cache_balancer.hpp"
#include "clustering/administration/artificial_reql_cluster_interface.hpp"
#include "clustering/administration/http/server.hpp"
#include "clustering/administration/issues/local.hpp"
#include "clustering/administration/issues/outdated_index.hpp"
#include "clustering/administration/issues/server.hpp"
#include "clustering/administration/jobs/manager.hpp"
#include "clustering/administration/logs/log_writer.hpp"
#include "clustering/administration/main/initial_join.hpp"
#include "clustering/administration/main/ports.hpp"
#include "clustering/administration/main/watchable_fields.hpp"
#include "clustering/administration/main/version_check.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/perfmon_collection_repo.hpp"
#include "clustering/administration/persist/file_keys.hpp"
#include "clustering/administration/persist/semilattice.hpp"
#include "clustering/administration/persist/table_interface.hpp"
#include "clustering/administration/real_reql_cluster_interface.hpp"
#include "clustering/administration/servers/auto_reconnect.hpp"
#include "clustering/administration/servers/config_server.hpp"
#include "clustering/administration/servers/config_client.hpp"
#include "clustering/administration/servers/network_logger.hpp"
#include "clustering/table_manager/table_meta_client.hpp"
#include "clustering/table_manager/multi_table_manager.hpp"
#include "containers/incremental_lenses.hpp"
#include "extproc/extproc_pool.hpp"
#include "rdb_protocol/query_server.hpp"
#include "rpc/connectivity/cluster.hpp"
#include "rpc/directory/map_read_manager.hpp"
#include "rpc/directory/map_write_manager.hpp"
#include "rpc/directory/read_manager.hpp"
#include "rpc/directory/write_manager.hpp"
#include "rpc/semilattice/semilattice_manager.hpp"
#include "rpc/semilattice/view/field.hpp"

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
        actual_addresses = get_local_ips(std::set<ip_address_t>(),
                                         local_ip_filter_t::ALL);
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

// Defined in command_line.cc; not in any header, because it is not
// safe to run in general.
std::string run_uname(const std::string &flags);

bool do_serve(io_backender_t *io_backender,
              bool i_am_a_server,
              // NB. filepath & persistent_file are used only if i_am_a_server is true.
              const base_path_t &base_path,
              metadata_file_t *metadata_file,
              const serve_info_t &serve_info,
              os_signal_cond_t *stop_cond) {
    // Do this here so we don't block on popen while pretending to serve.
    std::string uname = run_uname("ms");
    try {
        extproc_pool_t extproc_pool(get_num_threads());

        local_issue_aggregator_t local_issue_aggregator;

        thread_pool_log_writer_t log_writer(&local_issue_aggregator);

        cluster_semilattice_metadata_t cluster_metadata;
        auth_semilattice_metadata_t auth_metadata;
        server_id_t server_id = generate_uuid();
        if (metadata_file != NULL) {
            cond_t non_interruptor;
            metadata_file_t::read_txn_t txn(metadata_file, &non_interruptor);
            cluster_metadata = txn.read(mdkey_cluster_semilattices(), &non_interruptor);
            auth_metadata = txn.read(mdkey_auth_semilattices(), &non_interruptor);
            server_id = txn.read(mdkey_server_id(), &non_interruptor);
        }

#ifndef NDEBUG
        logNTC("Our server ID is %s", uuid_to_str(server_id).c_str());
#endif

        connectivity_cluster_t connectivity_cluster;

        mailbox_manager_t mailbox_manager(&connectivity_cluster, 'M');

        semilattice_manager_t<cluster_semilattice_metadata_t>
            semilattice_manager_cluster(&connectivity_cluster, 'S', cluster_metadata);

        semilattice_manager_t<auth_semilattice_metadata_t>
            semilattice_manager_auth(&connectivity_cluster, 'A', auth_metadata);

        directory_read_manager_t<cluster_directory_metadata_t>
            directory_read_manager(&connectivity_cluster, 'D');

        directory_map_read_manager_t<namespace_id_t, table_manager_bcard_t>
            table_directory_read_manager(&connectivity_cluster, 'T');

        directory_map_read_manager_t<
                std::pair<namespace_id_t, branch_id_t>,
                table_query_bcard_t>
            table_query_directory_read_manager(&connectivity_cluster, 'Q');

        directory_map_read_manager_t<server_id_t, empty_value_t>
            server_connection_read_manager(&connectivity_cluster, 'C');

        log_server_t log_server(&mailbox_manager, &log_writer);

        scoped_ptr_t<server_config_server_t> server_config_server;
        if (i_am_a_server) {
            server_config_server.init(new server_config_server_t(
                &mailbox_manager,
                server_id,
                directory_read_manager.get_root_view(),
                metadata_field(
                    &cluster_semilattice_metadata_t::servers,
                    semilattice_manager_cluster.get_root_view())));
        }

        server_config_client_t server_config_client(
            &mailbox_manager,
            directory_read_manager.get_root_map_view(),
            metadata_field(
                &cluster_semilattice_metadata_t::servers,
                semilattice_manager_cluster.get_root_view()),
            server_connection_read_manager.get_root_view());

        jobs_manager_t jobs_manager(&mailbox_manager, server_id, &server_config_client);
        stat_manager_t stat_manager(&mailbox_manager, server_id);

        network_logger_t network_logger(
            connectivity_cluster.get_me(),
            directory_read_manager.get_root_map_view(),
            metadata_field(&cluster_semilattice_metadata_t::servers,
                           semilattice_manager_cluster.get_root_view()));

        scoped_ptr_t<server_issue_tracker_t> server_issue_tracker;
        if (i_am_a_server) {
            server_issue_tracker.init(new server_issue_tracker_t(
                &local_issue_aggregator,
                semilattice_manager_cluster.get_root_view(),
                directory_read_manager.get_root_map_view(),
                &server_config_client,
                server_config_server.get()));
        }

        scoped_ptr_t<connectivity_cluster_t::run_t> connectivity_cluster_run;

        try {
            connectivity_cluster_run.init(new connectivity_cluster_t::run_t(
                &connectivity_cluster,
                serve_info.ports.local_addresses,
                serve_info.ports.canonical_addresses,
                serve_info.ports.port,
                serve_info.ports.client_port));
        } catch (const address_in_use_exc_t &ex) {
            throw address_in_use_exc_t(strprintf("Could not bind to cluster port: %s", ex.what()));
        }

        // If (0 == port), then we asked the OS to give us a port number.
        if (serve_info.ports.port != 0) {
            guarantee(serve_info.ports.port == connectivity_cluster_run->get_port());
        }
        logNTC("Listening for intracluster connections on port %d\n",
            connectivity_cluster_run->get_port());

        auto_reconnector_t auto_reconnector(
            &connectivity_cluster,
            connectivity_cluster_run.get(),
            directory_read_manager.get_root_view()->incremental_subview(
                incremental_field_getter_t<server_id_t, cluster_directory_metadata_t>(&cluster_directory_metadata_t::server_id)),
            metadata_field(&cluster_semilattice_metadata_t::servers, semilattice_manager_cluster.get_root_view()));

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

        perfmon_collection_repo_t perfmon_collection_repo(
            &get_global_perfmon_collection());

        // ReQL evaluation context and supporting structures
        rdb_context_t rdb_ctx(&extproc_pool,
                              &mailbox_manager,
                              NULL,   /* we'll fill this in later */
                              semilattice_manager_auth.get_root_view(),
                              &get_global_perfmon_collection(),
                              serve_info.reql_http_proxy);
        jobs_manager.set_rdb_context(&rdb_ctx);

        {
            /* Extract a subview of the directory with all the table meta manager
            business cards. */
            watchable_map_value_transform_t<peer_id_t, cluster_directory_metadata_t,
                    multi_table_manager_bcard_t>
                multi_table_manager_directory(
                    directory_read_manager.get_root_map_view(),
                    [](const cluster_directory_metadata_t *cluster_md) {
                        return &cluster_md->multi_table_manager_bcard;
                    });

            scoped_ptr_t<cache_balancer_t> cache_balancer;
            scoped_ptr_t<outdated_index_issue_tracker_t> outdated_index_issue_tracker;
            scoped_ptr_t<real_table_persistence_interface_t>
                table_persistence_interface;
            scoped_ptr_t<multi_table_manager_t> multi_table_manager;
            if (i_am_a_server) {
                cache_balancer.init(new alt_cache_balancer_t(
                    server_config_server->get_actual_cache_size_bytes()));
                outdated_index_issue_tracker.init(new outdated_index_issue_tracker_t(
                    &local_issue_aggregator));
                table_persistence_interface.init(
                    new real_table_persistence_interface_t(
                        io_backender,
                        cache_balancer.get(),
                        base_path,
                        outdated_index_issue_tracker.get(),
                        &rdb_ctx,
                        metadata_file));
                multi_table_manager.init(new multi_table_manager_t(
                    server_id,
                    &mailbox_manager,
                    &multi_table_manager_directory,
                    table_directory_read_manager.get_root_view(),
                    server_config_client.get_connections_map(),
                    table_persistence_interface.get(),
                    base_path,
                    io_backender,
                    &perfmon_collection_repo));
            } else {
                /* Proxies still need a `multi_table_manager_t` because it takes care of
                receiving table names, databases, and primary keys from other servers and
                providing them to the `table_meta_client_t`. */
                multi_table_manager.init(new multi_table_manager_t(
                    &mailbox_manager,
                    &multi_table_manager_directory,
                    table_directory_read_manager.get_root_view()));
            }

            table_meta_client_t table_meta_client(
                &mailbox_manager,
                multi_table_manager.get(),
                &multi_table_manager_directory,
                table_directory_read_manager.get_root_view());

            real_reql_cluster_interface_t real_reql_cluster_interface(
                &mailbox_manager,
                semilattice_manager_cluster.get_root_view(),
                &rdb_ctx,
                &server_config_client,
                &table_meta_client,
                table_query_directory_read_manager.get_root_view());

            admin_artificial_tables_t admin_tables(
                &real_reql_cluster_interface,
                semilattice_manager_cluster.get_root_view(),
                semilattice_manager_auth.get_root_view(),
                directory_read_manager.get_root_view(),
                directory_read_manager.get_root_map_view(),
                &table_meta_client,
                &server_config_client,
                &mailbox_manager);

            /* `real_reql_cluster_interface_t` needs access to the admin tables so that
            it can return rows from the `table_status` and `table_config` artificial
            tables when the user calls the corresponding porcelains. But
            `admin_artificial_tables_t` needs access to the
            `real_reql_cluster_interface_t` because `table_config` needs to be able to
            run distribution queries. The simplest solution is for them to have
            references to each other. This is the place where we "close the loop". */
            real_reql_cluster_interface.admin_tables = &admin_tables;

            /* `rdb_context_t` needs access to the `reql_cluster_interface_t` so that it
            can find tables and run meta-queries, but the `real_reql_cluster_interface_t`
            needs access to the `rdb_context_t` so that it can construct instances of
            `cluster_namespace_interface_t`. Again, we solve this problem by having a
            circular reference. */
            rdb_ctx.cluster_interface = admin_tables.get_reql_cluster_interface();

            cluster_directory_metadata_t initial_directory(
                server_id,
                connectivity_cluster.get_me(),
                RETHINKDB_VERSION_STR,
                current_microtime(),
                getpid(),
                str_gethostname(),
                /* Note we'll update `reql_port` and `http_port` later, once final values
                are available */
                connectivity_cluster_run->get_port(),
                serve_info.ports.reql_port,
                serve_info.ports.http_admin_is_disabled
                    ? boost::optional<uint16_t>()
                    : boost::optional<uint16_t>(serve_info.ports.http_port),
                connectivity_cluster_run->get_canonical_addresses(),
                serve_info.argv,
                0,   /* we'll fill `actual_cache_size_bytes` in later */
                multi_table_manager->get_multi_table_manager_bcard(),
                jobs_manager.get_business_card(),
                stat_manager.get_address(),
                log_server.get_business_card(),
                i_am_a_server
                    ? boost::make_optional(server_config_server->get_business_card())
                    : boost::optional<server_config_business_card_t>(),
                i_am_a_server ? SERVER_PEER : PROXY_PEER);

            watchable_variable_t<cluster_directory_metadata_t>
                our_root_directory_variable(initial_directory);

            field_copier_t<local_issues_t, cluster_directory_metadata_t>
                copy_local_issues_to_cluster(
                    &cluster_directory_metadata_t::local_issues,
                    local_issue_aggregator.get_issues_watchable(),
                    &our_root_directory_variable);

            /* This will take care of updating the directory every time our cache size
            changes. It also fills in the initial value. */
            scoped_ptr_t<field_copier_t<uint64_t, cluster_directory_metadata_t> >
                actual_cache_size_directory_copier;
            if (i_am_a_server) {
                actual_cache_size_directory_copier.init(
                    new field_copier_t<uint64_t, cluster_directory_metadata_t>(
                        &cluster_directory_metadata_t::actual_cache_size_bytes,
                        server_config_server->get_actual_cache_size_bytes(),
                        &our_root_directory_variable));
            }

            directory_write_manager_t<cluster_directory_metadata_t> directory_write_manager(
                &connectivity_cluster, 'D', our_root_directory_variable.get_watchable());

            directory_map_write_manager_t<server_id_t, empty_value_t> server_connection_write_manager(
                &connectivity_cluster, 'C', network_logger.get_local_connections_map());

            scoped_ptr_t<directory_map_write_manager_t<
                    namespace_id_t, table_manager_bcard_t> >
                table_directory_write_manager;
            scoped_ptr_t<directory_map_write_manager_t<
                    std::pair<namespace_id_t, uuid_u>, table_query_bcard_t> >
                table_query_directory_write_manager;
            if (i_am_a_server) {
                table_directory_write_manager.init(
                    new directory_map_write_manager_t<
                            namespace_id_t, table_manager_bcard_t>(
                        &connectivity_cluster, 'T',
                        multi_table_manager->get_table_manager_bcards()));
                table_query_directory_write_manager.init(
                    new directory_map_write_manager_t<
                            std::pair<namespace_id_t, uuid_u>, table_query_bcard_t>(
                        &connectivity_cluster, 'Q',
                        multi_table_manager->get_table_query_bcards()));
            }

            {
                rdb_query_server_t rdb_query_server(
                    serve_info.ports.local_addresses,
                    serve_info.ports.reql_port,
                    &rdb_ctx);
                logNTC("Listening for client driver connections on port %d\n",
                       rdb_query_server.get_port());
                /* If `serve_info.ports.reql_port` was zero then the OS assigned us a
                port, so we need to update the directory. */
                our_root_directory_variable.apply_atomic_op(
                    [&](cluster_directory_metadata_t *md) -> bool {
                        md->reql_port = rdb_query_server.get_port();
                        return (md->reql_port != serve_info.ports.reql_port);
                    });

                scoped_ptr_t<semilattice_persister_t<cluster_semilattice_metadata_t> >
                    cluster_metadata_persister;
                scoped_ptr_t<semilattice_persister_t<auth_semilattice_metadata_t> >
                    auth_metadata_persister;

                if (i_am_a_server) {
                    cluster_metadata_persister.init(
                        new semilattice_persister_t<cluster_semilattice_metadata_t>(
                            metadata_file,
                            mdkey_cluster_semilattices(),
                            semilattice_manager_cluster.get_root_view()));
                    auth_metadata_persister.init(
                        new semilattice_persister_t<auth_semilattice_metadata_t>(
                            metadata_file,
                            mdkey_auth_semilattices(),
                            semilattice_manager_auth.get_root_view()));
                }

                {
                    scoped_ptr_t<administrative_http_server_manager_t> admin_server_ptr;
                    if (serve_info.ports.http_admin_is_disabled) {
                        logNTC("Administrative HTTP connections are disabled.\n");
                    } else {
                        // TODO: Pardon me what, but is this how we fail here?
                        guarantee(serve_info.ports.http_port < 65536);
                        admin_server_ptr.init(
                            new administrative_http_server_manager_t(
                                serve_info.ports.local_addresses,
                                serve_info.ports.http_port,
                                server_id,
                                rdb_query_server.get_http_app(),
                                serve_info.web_assets));
                        logNTC("Listening for administrative HTTP connections on port %d\n",
                               admin_server_ptr->get_port());
                        /* If `serve_info.ports.http_port` was zero then the OS assigned
                        us a port, so we need to update the directory. */
                        our_root_directory_variable.apply_atomic_op(
                            [&](cluster_directory_metadata_t *md) -> bool {
                                *md->http_admin_port = admin_server_ptr->get_port();
                                return (*md->http_admin_port !=
                                    serve_info.ports.http_port);
                            });
                    }

                    const std::string addresses_string = serve_info.ports.get_addresses_string();
                    logNTC("Listening on address%s: %s\n",
                           serve_info.ports.local_addresses.size() == 1 ? "" : "es",
                           addresses_string.c_str());

                    if (!serve_info.ports.is_bind_all()) {
                        if(serve_info.config_file) {
                            logNTC("To fully expose RethinkDB on the network, bind to "
                                   "all addresses by adding `bind=all' to the config "
                                   "file (%s).", (*serve_info.config_file).c_str());
                        } else {
                            logNTC("To fully expose RethinkDB on the network, bind to "
                                   "all addresses by running rethinkdb with the `--bind "
                                   "all` command line option.");
                        }
                    }

                    if (i_am_a_server) {
                        // TODO: This duplicates part of network_logger_t::pretty_print_server, refactor
                        servers_semilattice_metadata_t::server_map_t::const_iterator
                            server_it = cluster_metadata.servers.servers.find(server_id);
                        guarantee(server_it != cluster_metadata.servers.servers.end());
                        std::string server_name;
                        if (server_it->second.is_deleted()) {
                            server_name = "__deleted_server__";
                        } else {
                            server_name =
                                server_it->second.get_ref().name.get_ref().str();
                        }
                        logNTC("Server ready, \"%s\" %s\n",
                               server_name.c_str(),
                               uuid_to_str(server_id).c_str());
                    } else {
                        logNTC("Proxy ready");
                    }

                    scoped_ptr_t<version_checker_t> checker;
                    if (i_am_a_server
                        && serve_info.do_version_checking == update_check_t::perform) {
                        checker.init(new version_checker_t(&rdb_ctx,
                            semilattice_manager_cluster.get_root_view(), uname));
                    }

                    stop_cond->wait_lazily_unordered();

                    if (stop_cond->get_source_signo() == SIGINT) {
                        logNTC("Server got SIGINT from pid %d, uid %d; shutting down...\n",
                               stop_cond->get_source_pid(), stop_cond->get_source_uid());
                    } else if (stop_cond->get_source_signo() == SIGTERM) {
                        logNTC("Server got SIGTERM from pid %d, uid %d; shutting down...\n",
                               stop_cond->get_source_pid(), stop_cond->get_source_uid());

                    } else {
                        logNTC("Server got signal %d from pid %d, uid %d; shutting down...\n",
                               stop_cond->get_source_signo(),
                               stop_cond->get_source_pid(), stop_cond->get_source_uid());
                    }
                }

                cond_t non_interruptor;
                if (i_am_a_server) {
                    cluster_metadata_persister->stop_and_flush(&non_interruptor);
                    auth_metadata_persister->stop_and_flush(&non_interruptor);
                }

                logNTC("Shutting down client connections...\n");
            }
            logNTC("All client connections closed.\n");

            jobs_manager.unset_rdb_context_and_reactor_driver();

            logNTC("Shutting down storage engine... (This may take a while if you had a lot of unflushed data in the writeback cache.)\n");
        }
        logNTC("Storage engine shut down.\n");

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
           metadata_file_t *metadata_file,
           const serve_info_t &serve_info,
           os_signal_cond_t *stop_cond) {
    return do_serve(io_backender,
                    true,
                    base_path,
                    metadata_file,
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
                    nullptr,
                    serve_info,
                    stop_cond);
}
