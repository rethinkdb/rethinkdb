// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/main/serve.hpp"

#include <stdio.h>

#include "arch/arch.hpp"
#include "arch/io/network.hpp"
#include "arch/os_signal.hpp"
#include "buffer_cache/cache_balancer.hpp"
#include "clustering/administration/artificial_reql_cluster_interface.hpp"
#include "clustering/administration/http/server.hpp"
#include "clustering/administration/issues/local.hpp"
#include "clustering/administration/jobs/manager.hpp"
#include "clustering/administration/logs/log_writer.hpp"
#include "clustering/administration/main/initial_join.hpp"
#include "clustering/administration/main/ports.hpp"
#include "clustering/administration/main/memory_checker.hpp"
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
        peer_address_t peer(std::set<host_and_port_t>{names[i]});
        if (peers.find(peer) != peers.end()) {
            logWRN("Duplicate peer in --join parameters, ignoring: '%s:%d'",
                   names[i].host().c_str(), names[i].port().value());
        } else {
            peers.insert(peer);
        }
    }
    return peers;
}

std::string service_address_ports_t::get_addresses_string(
    std::set<ip_address_t> actual_addresses) const {

    bool first = true;
    std::string result;

    // Get the actual list for printing if we're listening on all addresses.
    if (is_bind_all(actual_addresses)) {
        actual_addresses = get_local_ips(std::set<ip_address_t>(),
                                         local_ip_filter_t::ALL);
    }

    for (std::set<ip_address_t>::const_iterator i = actual_addresses.begin(); i != actual_addresses.end(); ++i) {
        result += (first ? "" : ", " ) + i->to_string();
        first = false;
    }

    return result;
}

bool service_address_ports_t::is_bind_all(
    std::set<ip_address_t> addresses) const {
    // If the set is empty, it means we're listening on all addresses.
    return addresses.empty();
}

#ifdef _WIN32
std::string windows_version_string();
#else
// Defined in command_line.cc; not in any header, because it is not
// safe to run in general.
std::string run_uname(const std::string &flags);
#endif

bool do_serve(io_backender_t *io_backender,
              bool i_am_a_server,
              // NB. filepath & persistent_file are used only if i_am_a_server is true.
              const base_path_t &base_path,
              metadata_file_t *metadata_file,
              const serve_info_t &serve_info,
              os_signal_cond_t *stop_cond,
              /* For regular servers, the initial password is already set in the
              metadata and this will be empty. */
              const std::string &proxy_initial_password = "") {
    /* This coroutine is responsible for creating and destroying most of the important
    components of the server. */

    // Do this here so we don't block on popen while pretending to serve.
#ifdef _WIN32
    std::string uname = windows_version_string();
#else
    std::string uname = run_uname("ms");
#endif
    try {
        /* `extproc_pool` spawns several subprocesses that can be used to run tasks that
        we don't want to run in the main RethinkDB process, such as Javascript
        evaluations. */
        extproc_pool_t extproc_pool(get_num_threads());

        /* `thread_pool_log_writer_t` automatically registers itself. While it exists,
        log messages will be written using the event loop instead of blocking. */
        thread_pool_log_writer_t log_writer;

        cluster_semilattice_metadata_t cluster_metadata;
        auth_semilattice_metadata_t auth_metadata;
        heartbeat_semilattice_metadata_t heartbeat_metadata;
        server_id_t server_id;
        if (metadata_file != nullptr) {
            guarantee(proxy_initial_password.empty());
            cond_t non_interruptor;
            metadata_file_t::read_txn_t txn(metadata_file, &non_interruptor);
            cluster_metadata = txn.read(mdkey_cluster_semilattices(), &non_interruptor);
            auth_metadata = txn.read(mdkey_auth_semilattices(), &non_interruptor);
            heartbeat_metadata = txn.read(
                mdkey_heartbeat_semilattices(), &non_interruptor);
            server_id = txn.read(mdkey_server_id(), &non_interruptor);
        } else {
            // We are a proxy, generate a temporary proxy server id.
            server_id = server_id_t::generate_proxy_id();
            auth_metadata.m_users.insert(
                auth_semilattice_metadata_t::create_initial_admin_pair(
                    proxy_initial_password));
        }

#ifndef NDEBUG
        logNTC("Our server ID is %s", server_id.print().c_str());
#endif

        /* The `connectivity_cluster_t` maintains TCP connections to other servers in the
        cluster. */
        connectivity_cluster_t connectivity_cluster;

        /* The `mailbox_manager_t` maintains a local index of mailboxes that exist on
        this server, and routes mailbox messages received from other servers. */
        mailbox_manager_t mailbox_manager(&connectivity_cluster, 'M');

        /* `semilattice_manager_cluster`, `semilattice_manager_auth`, and
        `semilattice_manager_heartbeat` are responsible for syncing the semilattice
        metadata between servers over the network. */
        semilattice_manager_t<cluster_semilattice_metadata_t>
            semilattice_manager_cluster(&connectivity_cluster, 'S', cluster_metadata);
        semilattice_manager_t<auth_semilattice_metadata_t>
            semilattice_manager_auth(&connectivity_cluster, 'A', auth_metadata);
        semilattice_manager_t<heartbeat_semilattice_metadata_t>
            semilattice_manager_heartbeat(
                &connectivity_cluster, 'Y', heartbeat_metadata);

        /* The `directory_*_read_manager_t`s are responsible for receiving directory
        updates over the network from other servers. */

        /* The `cluster_directory_metadata_t` contains basic information about each
        server (such as its name, server ID, version, PID, command line arguments, etc.)
        and also many singleton mailboxes for various purposes. */
        directory_read_manager_t<cluster_directory_metadata_t>
            directory_read_manager(&connectivity_cluster, 'D');

        /* The `table_manager_bcard_t`s contain the mailboxes that allow the Raft members
        on different servers to communicate with each other. */
        directory_map_read_manager_t<namespace_id_t, table_manager_bcard_t>
            table_directory_read_manager(&connectivity_cluster, 'T');

        /* The `table_query_bcard_t`s contain mailboxes that execute `read_t`s or
        `write_t`s. The `table_query_client_t` reads this directory to know how to route
        queries. */
        directory_map_read_manager_t<
                std::pair<namespace_id_t, branch_id_t>,
                table_query_bcard_t>
            table_query_directory_read_manager(&connectivity_cluster, 'Q');

        /* `server_connection_read_manager` gives us second-order connectivity
        information; we can figure out which of the servers we're connected to are
        connected to which other servers. */
        directory_map_read_manager_t<server_id_t, empty_value_t>
            server_connection_read_manager(&connectivity_cluster, 'C');

        /* `log_server` retrieves pieces of our local log file and sends them out over
        the network via its mailbox. */
        log_server_t log_server(&mailbox_manager, &log_writer);

        /* `server_config_server` tracks this server's name, tags, etc. It takes care of
        loading and persisting that information to disk, and also distributing it over
        the network via the `cluster_directory_metadata_t`. */
        scoped_ptr_t<server_config_server_t> server_config_server;
        if (i_am_a_server) {
            server_config_server.init(new server_config_server_t(
                &mailbox_manager, metadata_file));
        }

        /* `server_config_client` is used to get a list of all connected servers and
        request information about their names and tags. It can also be used to change
        servers' names and tags over the network by sending messages to the servers'
        `server_config_server_t`s. */
        server_config_client_t server_config_client(
            &mailbox_manager,
            directory_read_manager.get_root_map_view(),
            server_connection_read_manager.get_root_view());

        /* `network_logger` writes to the log file when another server connects or
        disconnects. */
        network_logger_t network_logger(
            connectivity_cluster.get_me(),
            directory_read_manager.get_root_map_view());

        /* `connectivity_cluster_run` is the other half of the `connectivity_cluster_t`.
        Before it's created, the `connectivity_cluster_t` won't process any connections
        or messages. So it's only safe to create now that we've set up all of our message
        handlers. */
        scoped_ptr_t<connectivity_cluster_t::run_t> connectivity_cluster_run;
        try {
            connectivity_cluster_run.init(new connectivity_cluster_t::run_t(
                &connectivity_cluster,
                server_id,
                serve_info.ports.local_addresses_cluster,
                serve_info.ports.canonical_addresses,
                serve_info.join_delay_secs,
                serve_info.ports.port,
                serve_info.ports.client_port,
                semilattice_manager_heartbeat.get_root_view(),
                semilattice_manager_auth.get_root_view(),
                serve_info.tls_configs.cluster.get()));
        } catch (const address_in_use_exc_t &ex) {
            throw address_in_use_exc_t(strprintf("Could not bind to cluster port: %s", ex.what()));
        }

        // If (0 == port), then we asked the OS to give us a port number.
        if (serve_info.ports.port != 0) {
            guarantee(serve_info.ports.port == connectivity_cluster_run->get_port());
        }
        logNTC("Listening for intracluster connections on port %d\n",
            connectivity_cluster_run->get_port());

        /* `auto_reconnector` tries to reconnect to other servers if we lose the
        connection to them. */
        auto_reconnector_t auto_reconnector(
            &connectivity_cluster,
            connectivity_cluster_run.get(),
            &server_config_client,
            serve_info.join_delay_secs,
            serve_info.node_reconnect_timeout_secs * 1000); // in ms

        /* `initial_joiner` sets up the initial connections to the peers that were
        specified with the `--join` flag on the command line. */
        scoped_ptr_t<initial_joiner_t> initial_joiner;
        if (!serve_info.peers.empty()) {
            initial_joiner.init(new initial_joiner_t(&connectivity_cluster,
                                                     connectivity_cluster_run.get(),
                                                     serve_info.peers,
                                                     serve_info.join_delay_secs));
            try {
                wait_interruptible(initial_joiner->get_ready_signal(), stop_cond);
            } catch (const interrupted_exc_t &) {
                return false;
            }
        }

        perfmon_collection_repo_t perfmon_collection_repo(
            &get_global_perfmon_collection());

        /* We thread the `rdb_context_t` through every function that evaluates ReQL
        terms. It contains pointers to all the things that the ReQL term evaluation code
        needs. */
        rdb_context_t rdb_ctx(&extproc_pool,
                              &mailbox_manager,
                              nullptr,   /* we'll fill this in later */
                              semilattice_manager_auth.get_root_view(),
                              &get_global_perfmon_collection(),
                              serve_info.reql_http_proxy);
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

            /* The `multi_table_manager_t` takes care of the actual business of setting
            up tables and handling queries for them. The `table_persistence_interface_t`
            helps it by constructing the B-trees and serializers, and also persisting
            table-related metadata to disk. */
            scoped_ptr_t<cache_balancer_t> cache_balancer;
            scoped_ptr_t<real_table_persistence_interface_t>
                table_persistence_interface;
            scoped_ptr_t<multi_table_manager_t> multi_table_manager;
            if (i_am_a_server) {
                cache_balancer.init(new alt_cache_balancer_t(
                    server_config_server->get_actual_cache_size_bytes()));
                table_persistence_interface.init(
                    new real_table_persistence_interface_t(
                        io_backender,
                        cache_balancer.get(),
                        base_path,
                        &rdb_ctx,
                        metadata_file));
                multi_table_manager.init(new multi_table_manager_t(
                    server_id,
                    &mailbox_manager,
                    &server_config_client,
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

            /* The `table_meta_client_t` sends messages to the `multi_table_manager_t`s
            on the other servers in the cluster to create, drop, and reconfigure tables,
            as well as request information about them. */
            table_meta_client_t table_meta_client(
                &mailbox_manager,
                multi_table_manager.get(),
                &multi_table_manager_directory,
                table_directory_read_manager.get_root_view(),
                &server_config_client);

            /* The `real_reql_cluster_interface_t` is the interface that the ReQL logic
            uses to create, destroy, and reconfigure databases and tables. */
            real_reql_cluster_interface_t real_reql_cluster_interface(
                &mailbox_manager,
                semilattice_manager_auth.get_root_view(),
                semilattice_manager_cluster.get_root_view(),
                &rdb_ctx,
                &server_config_client,
                &table_meta_client,
                multi_table_manager.get(),
                table_query_directory_read_manager.get_root_view());

            /* `admin_artificial_tables_t` is a container for all of the tables in the
            `rethinkdb` system database. */
            admin_artificial_tables_t admin_tables(
                &real_reql_cluster_interface,
                semilattice_manager_auth.get_root_view(),
                semilattice_manager_cluster.get_root_view(),
                semilattice_manager_heartbeat.get_root_view(),
                directory_read_manager.get_root_view(),
                directory_read_manager.get_root_map_view(),
                &table_meta_client,
                &server_config_client,
                real_reql_cluster_interface.get_namespace_repo(),
                &mailbox_manager);

            /* Kick off a coroutine to log any outdated indexes. */
            outdated_index_issue_tracker_t::log_outdated_indexes(
                multi_table_manager.get(),
                semilattice_manager_cluster.get_root_view()->get(),
                stop_cond);

            /* `jobs_manager_t` keeps track of all of the running jobs on this server.
            When the user reads the `rethinkdb.jobs` table, it sends messages to the
            `jobs_manager_t` on each server to get information about running jobs. */
            jobs_manager_t jobs_manager(
                &mailbox_manager,
                server_id,
                &rdb_ctx,
                /* A `table_persistence_interface` is only instantiated when
                `i_am_a_server` is true, and a `nullptr` otherwise. */
                table_persistence_interface.get_or_null(),
                multi_table_manager.get());

            /* When the user reads the `rethinkdb.stats` table, it sends messages to the
            `stat_manager_t` on each server to get the stats information. */
            stat_manager_t stat_manager(&mailbox_manager, server_id);

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

            /* `memory_checker` periodically checks to see if we are using swap
                    memory, and will log a warning. */
            scoped_ptr_t<memory_checker_t> memory_checker;
            if (i_am_a_server) {
                memory_checker.init(new memory_checker_t());
            }

            /* When the user reads the `rethinkdb.current_issues` table, it sends
            messages to the `local_issue_server` on each server to get the issues
            information. */
            scoped_ptr_t<local_issue_server_t> local_issue_server;
            if (i_am_a_server) {
                local_issue_server.init(new local_issue_server_t(
                    &mailbox_manager,
                    log_writer.get_log_write_issue_tracker(),
                    memory_checker->get_memory_issue_tracker()));
            }

            proc_directory_metadata_t initial_proc_directory {
                RETHINKDB_VERSION_STR,
                current_microtime(),
                getpid(),
                str_gethostname(),
                /* Note we'll update `reql_port` and `http_port` later, once final values
                are available */
                static_cast<uint16_t>(connectivity_cluster_run->get_port()),
                static_cast<uint16_t>(serve_info.ports.reql_port),
                serve_info.ports.http_admin_is_disabled
                    ? boost::optional<uint16_t>()
                    : boost::optional<uint16_t>(serve_info.ports.http_port),
                connectivity_cluster_run->get_canonical_addresses(),
                serve_info.argv };
            cluster_directory_metadata_t initial_directory(
                server_id,
                connectivity_cluster.get_me(),
                initial_proc_directory,
                0,   /* we'll fill `actual_cache_size_bytes` in later */
                multi_table_manager->get_multi_table_manager_bcard(),
                jobs_manager.get_business_card(),
                stat_manager.get_address(),
                log_server.get_business_card(),
                i_am_a_server
                    ? local_issue_server->get_bcard()
                    : local_issue_bcard_t(),
                i_am_a_server
                    ? server_config_server->get_config()->get()
                    : server_config_versioned_t(),
                i_am_a_server
                    ? boost::make_optional(server_config_server->get_business_card())
                    : boost::optional<server_config_business_card_t>(),
                i_am_a_server ? SERVER_PEER : PROXY_PEER);

            /* `our_root_directory_variable` is the value we'll send out over the network
            in our directory to all the other servers. */
            watchable_variable_t<cluster_directory_metadata_t>
                our_root_directory_variable(initial_directory);

            /* These will take care of updating the directory every time our cache size
            or server config changes. They also fill in the initial values. */
            scoped_ptr_t<watchable_field_copier_t<
                    uint64_t, cluster_directory_metadata_t> >
                actual_cache_size_directory_copier;
            scoped_ptr_t<watchable_field_copier_t<
                    server_config_versioned_t, cluster_directory_metadata_t> >
                server_config_directory_copier;
            if (i_am_a_server) {
                actual_cache_size_directory_copier.init(
                    new watchable_field_copier_t<
                            uint64_t, cluster_directory_metadata_t>(
                        &cluster_directory_metadata_t::actual_cache_size_bytes,
                        server_config_server->get_actual_cache_size_bytes(),
                        &our_root_directory_variable));
                server_config_directory_copier.init(
                    new watchable_field_copier_t<
                            server_config_versioned_t, cluster_directory_metadata_t>(
                        &cluster_directory_metadata_t::server_config,
                        server_config_server->get_config(),
                        &our_root_directory_variable));
            }

            /* These `directory_*_write_manager_t`s are the counterparts to the
            `directory_*_read_manager_t`s earlier in this file. These are responsible for
            sending directory information over the network; the `read_manager_t`s are
            responsible for receiving the transmissions. */

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
                /* The `rdb_query_server_t` listens for client requests and processes the
                queries it receives. */
                rdb_query_server_t rdb_query_server(
                    serve_info.ports.local_addresses_driver,
                    serve_info.ports.reql_port,
                    &rdb_ctx,
                    &server_config_client,
                    server_id,
                    serve_info.tls_configs.driver.get());
                logNTC("Listening for client driver connections on port %d\n",
                       rdb_query_server.get_port());
                /* If `serve_info.ports.reql_port` was zero then the OS assigned us a
                port, so we need to update the directory. */
                our_root_directory_variable.apply_atomic_op(
                    [&](cluster_directory_metadata_t *md) -> bool {
                        md->proc.reql_port = rdb_query_server.get_port();
                        return (md->proc.reql_port != serve_info.ports.reql_port);
                    });

                /* `cluster_metadata_persister`, `auth_metadata_persister`, and
                `heartbeat_semilattice_metadata_t` are responsible for syncing the
                semilattice metadata to disk. */
                scoped_ptr_t<semilattice_persister_t<cluster_semilattice_metadata_t> >
                    cluster_metadata_persister;
                scoped_ptr_t<semilattice_persister_t<auth_semilattice_metadata_t> >
                    auth_metadata_persister;
                scoped_ptr_t<semilattice_persister_t<heartbeat_semilattice_metadata_t> >
                    heartbeat_metadata_persister;

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
                    heartbeat_metadata_persister.init(
                        new semilattice_persister_t<heartbeat_semilattice_metadata_t>(
                            metadata_file,
                            mdkey_heartbeat_semilattices(),
                            semilattice_manager_heartbeat.get_root_view()));
                }

                {
                    /* The `administrative_http_server_manager_t` serves the web UI. */
                    scoped_ptr_t<administrative_http_server_manager_t> admin_server_ptr;
                    if (serve_info.ports.http_admin_is_disabled) {
                        logNTC("Administrative HTTP connections are disabled.\n");
                    } else {
                        // TODO: Pardon me what, but is this how we fail here?
                        guarantee(serve_info.ports.http_port < 65536);
                        admin_server_ptr.init(
                            new administrative_http_server_manager_t(
                                serve_info.ports.local_addresses_http,
                                serve_info.ports.http_port,
                                rdb_query_server.get_http_app(),
                                serve_info.web_assets,
                                serve_info.tls_configs.web.get()));
                        logNTC("Listening for administrative HTTP connections on port %d\n",
                               admin_server_ptr->get_port());
                        /* If `serve_info.ports.http_port` was zero then the OS assigned
                        us a port, so we need to update the directory. */
                        our_root_directory_variable.apply_atomic_op(
                            [&](cluster_directory_metadata_t *md) -> bool {
                                *md->proc.http_admin_port = admin_server_ptr->get_port();
                                return (*md->proc.http_admin_port !=
                                    serve_info.ports.http_port);
                            });
                    }

                    std::string addresses_string =
                        serve_info.ports.get_addresses_string(
                            serve_info.ports.local_addresses_cluster);
                    logNTC("Listening on cluster address%s: %s\n",
                           serve_info.ports.local_addresses_cluster.size() == 1 ? "" : "es",
                           addresses_string.c_str());

                    addresses_string =
                        serve_info.ports.get_addresses_string(
                            serve_info.ports.local_addresses_driver);
                    logNTC("Listening on driver address%s: %s\n",
                           serve_info.ports.local_addresses_driver.size() == 1 ? "" : "es",
                           addresses_string.c_str());

                    addresses_string =
                        serve_info.ports.get_addresses_string(
                            serve_info.ports.local_addresses_http);
                    logNTC("Listening on http address%s: %s\n",
                           serve_info.ports.local_addresses_http.size() == 1 ? "" : "es",
                           addresses_string.c_str());

                    if (!serve_info.ports.is_bind_all(serve_info.ports.local_addresses)) {
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
                        logNTC("Server ready, \"%s\" %s\n",
                               server_config_server->get_config()
                                   ->get().config.name.c_str(),
                               server_id.print().c_str());
                    } else {
                        logNTC("Proxy ready, %s", server_id.print().c_str());
                    }

                    /* `checker` periodically phones home to RethinkDB HQ to check if
                    there are later versions of RethinkDB available. */
                    scoped_ptr_t<version_checker_t> checker;
                    if (i_am_a_server
                        && serve_info.do_version_checking == update_check_t::perform) {
                        checker.init(new version_checker_t(
                            &rdb_ctx, uname, &table_meta_client, &server_config_client));
                    }

                    /* This is the end of the startup process. `stop_cond` will be pulsed
                    when it's time for the server to shut down. */
                    stop_cond->wait_lazily_unordered();
                    logNTC("Server got %s; shutting down...", stop_cond->format().c_str());
                }

                cond_t non_interruptor;
                if (i_am_a_server) {
                    cluster_metadata_persister->stop_and_flush(&non_interruptor);
                    auth_metadata_persister->stop_and_flush(&non_interruptor);
                }

                logNTC("Shutting down client connections...\n");
            }
            logNTC("All client connections closed.\n");
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
                 const std::string &initial_password,
                 os_signal_cond_t *stop_cond) {
    // TODO: filepath doesn't _seem_ ignored.
    // filepath and persistent_file are ignored for proxies, so we use the empty string & NULL respectively.
    return do_serve(nullptr,
                    false,
                    base_path_t(""),
                    nullptr,
                    serve_info,
                    stop_cond,
                    initial_password);
}
