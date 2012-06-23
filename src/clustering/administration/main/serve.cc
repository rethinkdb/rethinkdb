#include "arch/arch.hpp"
#include "arch/os_signal.hpp"
#include "clustering/administration/admin_tracker.hpp"
#include "clustering/administration/auto_reconnect.hpp"
#include "clustering/administration/http/server.hpp"
#include "clustering/administration/issues/local.hpp"
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
// TODO: the multistore header doesn't really belong.
#include "clustering/immediate_consistency/branch/multistore.hpp"
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

// TODO: This doesn't really belong here.
template <class protocol_t>
class file_based_svs_by_namespace_t : public svs_by_namespace_t<protocol_t> {
public:
    explicit file_based_svs_by_namespace_t(const std::string &file_path) : file_path_(file_path) { }

    void get_svs(perfmon_collection_t *perfmon_collection, namespace_id_t namespace_id,
                 boost::scoped_array<boost::scoped_ptr<typename protocol_t::store_t> > *stores_out,
                 boost::scoped_ptr<multistore_ptr_t<protocol_t> > *svs_out) {

        // TODO: If the server gets killed when starting up, we can
        // get a database in an invalid startup state.

        // TODO: Obviously, the hard-coded numeric constant here might
        // be regarded as a problem.  Randomly choosing between 4 or 5
        // is pretty cool though.
        const std::string file_name_base = file_path_ + "/" + uuid_to_str(namespace_id);

        // TODO: This is quite suspicious in that we check if the file
        // exists and then assume it exists or does not exist when
        // loading or creating it.

        // TODO: Also we only check file 0.

        // TODO: We should use N slices on M serializers, not N slices
        // on N serializers.

        //        stores_out->reset(new boost::scoped_ptr<typename protocol_t::store_t>[num_stores]);

        int res = access((file_name_base + "_" + strprintf("%d", 0)).c_str(), R_OK | W_OK);
        if (res == 0) {
            int num_stores = 1;
            while (0 == access((file_name_base + "_" + strprintf("%d", num_stores)).c_str(), R_OK | W_OK)) {
                ++num_stores;
            }

            // The files already exist, thus we don't create them.
            boost::scoped_array<store_view_t<protocol_t> *> store_views(new store_view_t<protocol_t> *[num_stores]);
            stores_out->reset(new boost::scoped_ptr<typename protocol_t::store_t>[num_stores]);

            debugf("loading %d hash-sharded stores\n", num_stores);

            // TODO: Exceptions?  Can exceptions happen, and then store_views' values would leak.

            // TODO: This should use pmap.
            for (int i = 0; i < num_stores; ++i) {
                const std::string file_name = file_name_base + "_" + strprintf("%d", i);
                (*stores_out)[i].reset(new typename protocol_t::store_t(file_name, false, perfmon_collection));
                store_views[i] = (*stores_out)[i].get();
            }

            svs_out->reset(new multistore_ptr_t<protocol_t>(store_views.get(), num_stores));
        } else {
            // num_stores randomization is commented out to simplify experiments with figuring out what's wrong with rebalance.
            const int num_stores = 4 + randint(4);
            debugf("creating %d hash-sharded stores\n", num_stores);
            stores_out->reset(new boost::scoped_ptr<typename protocol_t::store_t>[num_stores]);

            // TODO: How do we specify what the stores' regions are?

            // TODO: Exceptions?  Can exceptions happen, and then store_views' values would leak.

            // The files do not exist, create them.
            // TODO: This should use pmap.
            boost::scoped_array<store_view_t<protocol_t> *> store_views(new store_view_t<protocol_t> *[num_stores]);
            for (int i = 0; i < num_stores; ++i) {
                const std::string file_name = file_name_base + "_" + strprintf("%d", i);
                (*stores_out)[i].reset(new typename protocol_t::store_t(file_name, true, perfmon_collection));
                store_views[i] = (*stores_out)[i].get();
            }

            svs_out->reset(new multistore_ptr_t<protocol_t>(store_views.get(), num_stores));

            // Initialize the metadata in the underlying stores.
            boost::scoped_array<boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> > write_tokens(new boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t>[num_stores]);

            (*svs_out)->new_write_tokens(write_tokens.get(), num_stores);

            cond_t dummy_interruptor;

            (*svs_out)->set_all_metainfos(region_map_t<protocol_t, binary_blob_t>((*svs_out)->get_multistore_joined_region(),
                                                                                  binary_blob_t(version_range_t(version_t::zero()))),
                                          order_token_t::ignore,  // TODO
                                          write_tokens.get(),
                                          num_stores,
                                          &dummy_interruptor);
        }
    }


private:
    const std::string file_path_;

    DISABLE_COPYING(file_based_svs_by_namespace_t);
};

bool do_serve(
    bool i_am_a_server,
    const std::string &logfilepath,
    // NB. filepath & persistent_file are used iff i_am_a_server is true.
    const std::string &filepath, metadata_persistence::persistent_file_t *persistent_file,
    const std::set<peer_address_t> &joins,
    int port, int client_port, int http_port, DEBUG_ONLY(int port_offset, )
    machine_id_t machine_id, const cluster_semilattice_metadata_t &semilattice_metadata,
    std::string web_assets, signal_t *stop_cond)
{
    local_issue_tracker_t local_issue_tracker;

    log_writer_t log_writer(logfilepath, &local_issue_tracker);

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

    metadata_change_handler_t<cluster_semilattice_metadata_t> metadata_change_handler(&mailbox_manager, semilattice_manager_cluster.get_root_view());

    watchable_variable_t<cluster_directory_metadata_t> our_root_directory_variable(
        cluster_directory_metadata_t(
            machine_id,
            get_ips(),
            stat_manager.get_address(),
            metadata_change_handler.get_request_mailbox_address(),
            log_server.get_business_card(),
            i_am_a_server ? SERVER_PEER : PROXY_PEER
        ));

    message_multiplexer_t::client_t directory_manager_client(&message_multiplexer, 'D');
    directory_write_manager_t<cluster_directory_metadata_t> directory_write_manager(&directory_manager_client, our_root_directory_variable.get_watchable());
    directory_read_manager_t<cluster_directory_metadata_t> directory_read_manager(connectivity_cluster.get_connectivity_service());
    message_multiplexer_t::client_t::run_t directory_manager_client_run(&directory_manager_client, &directory_read_manager);

    message_multiplexer_t::run_t message_multiplexer_run(&message_multiplexer);
    connectivity_cluster_t::run_t connectivity_cluster_run(&connectivity_cluster, port, &message_multiplexer_run, client_port);

    // If (0 == port), then we asked the OS to give us a port number.
    if (0 == port) {
        port = connectivity_cluster_run.get_port();
    } else {
        rassert(port == connectivity_cluster_run.get_port());
    }
    printf("Listening for intracluster traffic on port %d...\n", port);

    auto_reconnector_t auto_reconnector(
        &connectivity_cluster,
        &connectivity_cluster_run,
        directory_read_manager.get_root_view()->subview(
            field_getter_t<machine_id_t, cluster_directory_metadata_t>(&cluster_directory_metadata_t::machine_id)),
        metadata_field(&cluster_semilattice_metadata_t::machines, semilattice_manager_cluster.get_root_view())
        );

    field_copier_t<std::list<local_issue_t>, cluster_directory_metadata_t> copy_local_issues_to_cluster(
        &cluster_directory_metadata_t::local_issues,
        local_issue_tracker.get_issues_watchable(),
        &our_root_directory_variable
        );

    admin_tracker_t admin_tracker(
        semilattice_manager_cluster.get_root_view(), directory_read_manager.get_root_view());

    perfmon_collection_t proc_stats_collection("proc", &get_global_perfmon_collection(), true, true);
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

    perfmon_collection_repo_t perfmon_repo(&get_global_perfmon_collection());

    file_based_svs_by_namespace_t<mock::dummy_protocol_t> dummy_svs_source(filepath);
    // Reactor drivers
    boost::scoped_ptr<reactor_driver_t<mock::dummy_protocol_t> > dummy_reactor_driver(!i_am_a_server ? NULL :
        new reactor_driver_t<mock::dummy_protocol_t>(
            &mailbox_manager,
            directory_read_manager.get_root_view()->subview(
                field_getter_t<namespaces_directory_metadata_t<mock::dummy_protocol_t>, cluster_directory_metadata_t>(&cluster_directory_metadata_t::dummy_namespaces)),
            metadata_field(&cluster_semilattice_metadata_t::dummy_namespaces, semilattice_manager_cluster.get_root_view()),
            metadata_field(&cluster_semilattice_metadata_t::machines, semilattice_manager_cluster.get_root_view()),
            directory_read_manager.get_root_view()->subview(
                field_getter_t<machine_id_t, cluster_directory_metadata_t>(&cluster_directory_metadata_t::machine_id)),
            &dummy_svs_source,
            &perfmon_repo));
    boost::scoped_ptr<field_copier_t<namespaces_directory_metadata_t<mock::dummy_protocol_t>, cluster_directory_metadata_t> >
        dummy_reactor_directory_copier(!i_am_a_server ? NULL :
            new field_copier_t<namespaces_directory_metadata_t<mock::dummy_protocol_t>, cluster_directory_metadata_t>(
                &cluster_directory_metadata_t::dummy_namespaces,
                dummy_reactor_driver->get_watchable(),
                &our_root_directory_variable));

    file_based_svs_by_namespace_t<memcached_protocol_t> memcached_svs_source(filepath);
    boost::scoped_ptr<reactor_driver_t<memcached_protocol_t> > memcached_reactor_driver(!i_am_a_server ? NULL :
        new reactor_driver_t<memcached_protocol_t>(
            &mailbox_manager,
            directory_read_manager.get_root_view()->subview(
                field_getter_t<namespaces_directory_metadata_t<memcached_protocol_t>, cluster_directory_metadata_t>(&cluster_directory_metadata_t::memcached_namespaces)),
            metadata_field(&cluster_semilattice_metadata_t::memcached_namespaces, semilattice_manager_cluster.get_root_view()),
            metadata_field(&cluster_semilattice_metadata_t::machines, semilattice_manager_cluster.get_root_view()),
            directory_read_manager.get_root_view()->subview(
                field_getter_t<machine_id_t, cluster_directory_metadata_t>(&cluster_directory_metadata_t::machine_id)),
            &memcached_svs_source,
            &perfmon_repo));
    boost::scoped_ptr<field_copier_t<namespaces_directory_metadata_t<memcached_protocol_t>, cluster_directory_metadata_t> >
        memcached_reactor_directory_copier(!i_am_a_server ? NULL :
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
        DEBUG_ONLY(port_offset, )
        &dummy_namespace_repo,
        &perfmon_repo);

    parser_maker_t<memcached_protocol_t, memcache_listener_t> memcached_parser_maker(
        &mailbox_manager,
        metadata_field(&cluster_semilattice_metadata_t::memcached_namespaces, semilattice_manager_cluster.get_root_view()),
        DEBUG_ONLY(port_offset, )
        &memcached_namespace_repo,
        &perfmon_repo);

    boost::scoped_ptr<metadata_persistence::semilattice_watching_persister_t> persister(!i_am_a_server ? NULL :
        new metadata_persistence::semilattice_watching_persister_t(
            persistent_file, machine_id, semilattice_manager_cluster.get_root_view()));

    {
        if (0 == http_port)
            http_port = port + 1000;

        guarantee(http_port < 65536);

        printf("Starting up administrative HTTP server on port %d...\n", http_port);
        administrative_http_server_manager_t administrative_http_interface(
            http_port,
            &mailbox_manager,
            &metadata_change_handler,
            semilattice_manager_cluster.get_root_view(),
            directory_read_manager.get_root_view(),
            &memcached_namespace_repo,
            &admin_tracker,
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

bool serve(const std::string &filepath, metadata_persistence::persistent_file_t *persistent_file, const std::set<peer_address_t> &joins, int port, int client_port, int http_port, DEBUG_ONLY(int port_offset, ) machine_id_t machine_id, const cluster_semilattice_metadata_t &semilattice_metadata, std::string web_assets, signal_t *stop_cond) {
    std::string logfilepath = filepath + "/log_file";
    return do_serve(true, logfilepath, filepath, persistent_file, joins, port, client_port, http_port, DEBUG_ONLY(port_offset, ) machine_id, semilattice_metadata, web_assets, stop_cond);
}

bool serve_proxy(const std::string &logfilepath, const std::set<peer_address_t> &joins, int port, int client_port, int http_port, DEBUG_ONLY(int port_offset, ) machine_id_t machine_id, const cluster_semilattice_metadata_t &semilattice_metadata, std::string web_assets, signal_t *stop_cond) {
    // filepath and persistent_file are ignored for proxies, so we use the empty string & NULL respectively.
    return do_serve(false, logfilepath, "", NULL, joins, port, client_port, http_port, DEBUG_ONLY(port_offset, ) machine_id, semilattice_metadata, web_assets, stop_cond);
}
