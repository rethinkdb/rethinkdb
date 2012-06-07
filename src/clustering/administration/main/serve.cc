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
#include "rpc/semilattice/view/function.hpp"
#include "rpc/semilattice/view/member.hpp"

// TODO: This doesn't really belong here.
template <class protocol_t>
class file_based_svs_by_namespace_t : public svs_by_namespace_t<protocol_t> {
public:
    file_based_svs_by_namespace_t(const std::string &file_path) : file_path_(file_path) { }

    void get_svs(perfmon_collection_t *perfmon_collection, namespace_id_t namespace_id,
                 boost::scoped_array<boost::scoped_ptr<typename protocol_t::store_t> > *stores_out,
                 boost::scoped_ptr<multistore_ptr_t<protocol_t> > *svs_out) {

        // TODO: If the server gets killed when starting up, we can
        // get a database in an invalid startup state.

        // TODO: Obviously, the hard-coded numeric constant here might
        // be regarded as a problem.
        const int num_stores = 4;
        const std::string file_name_base = file_path_ + "/" + uuid_to_str(namespace_id);

        // TODO: This is quite suspicious in that we check if the file
        // exists and then assume it exists or does not exist when
        // loading or creating it.

        // TODO: Also we only check file 0.

        // TODO: We should use N slices on M serializers, not N slices
        // on N serializers.

        stores_out->reset(new boost::scoped_ptr<typename protocol_t::store_t>[num_stores]);

        int res = access((file_name_base + "_" + strprintf("%d", 0)).c_str(), R_OK | W_OK);
        if (res == 0) {
            // The files already exist, thus we don't create them.
            boost::scoped_array<store_view_t<protocol_t> *> store_views(new store_view_t<protocol_t> *[num_stores]);

            // TODO: This should use pmap.
            for (int i = 0; i < num_stores; ++i) {
                const std::string file_name = file_name_base + "_" + strprintf("%d", i);
                (*stores_out)[i].reset(new typename protocol_t::store_t(file_name, false, perfmon_collection));
                store_views[i] = (*stores_out)[i].get();
            }

            svs_out->reset(new multistore_ptr_t<protocol_t>(store_views.get(), num_stores));
        } else {
            // TODO: How do we specify what the stores' regions are?

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

bool serve(const std::string &filepath, metadata_persistence::persistent_file_t *persistent_file, const std::set<peer_address_t> &joins, int port, int client_port, machine_id_t machine_id, const cluster_semilattice_metadata_t &semilattice_metadata, std::string web_assets, signal_t *stop_cond) {
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

    watchable_variable_t<cluster_directory_metadata_t> our_root_directory_variable(
        cluster_directory_metadata_t(
            machine_id,
            get_ips(),
            stat_manager.get_address(),
            log_server.get_business_card(),
            SERVER_PEER
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

    field_copier_t<std::list<local_issue_t>, cluster_directory_metadata_t> copy_local_issues_to_cluster(
        &cluster_directory_metadata_t::local_issues,
        local_issue_tracker.get_issues_watchable(),
        &our_root_directory_variable
        );

    global_issue_aggregator_t issue_aggregator;

    remote_issue_collector_t remote_issue_tracker(
        directory_read_manager.get_root_view()->subview(
            field_getter_t<std::list<local_issue_t>, cluster_directory_metadata_t>(&cluster_directory_metadata_t::local_issues)),
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

    file_based_svs_by_namespace_t<mock::dummy_protocol_t> dummy_svs_source(filepath);
    reactor_driver_t<mock::dummy_protocol_t> dummy_reactor_driver(&mailbox_manager,
                                                                  directory_read_manager.get_root_view()->subview(
                                                                      field_getter_t<namespaces_directory_metadata_t<mock::dummy_protocol_t>, cluster_directory_metadata_t>(&cluster_directory_metadata_t::dummy_namespaces)),
                                                                  metadata_field(&cluster_semilattice_metadata_t::dummy_namespaces, semilattice_manager_cluster.get_root_view()),
                                                                  metadata_field(&cluster_semilattice_metadata_t::machines, semilattice_manager_cluster.get_root_view()),
                                                                  directory_read_manager.get_root_view()->subview(
                                                                      field_getter_t<machine_id_t, cluster_directory_metadata_t>(&cluster_directory_metadata_t::machine_id)),
                                                                  &dummy_svs_source,
                                                                  &perfmon_repo);
    field_copier_t<namespaces_directory_metadata_t<mock::dummy_protocol_t>, cluster_directory_metadata_t> dummy_reactor_directory_copier(
        &cluster_directory_metadata_t::dummy_namespaces,
        dummy_reactor_driver.get_watchable(),
        &our_root_directory_variable
        );

    file_based_svs_by_namespace_t<memcached_protocol_t> memcached_svs_source(filepath);
    reactor_driver_t<memcached_protocol_t> memcached_reactor_driver(&mailbox_manager,
                                                                    directory_read_manager.get_root_view()->subview(
                                                                        field_getter_t<namespaces_directory_metadata_t<memcached_protocol_t>, cluster_directory_metadata_t>(&cluster_directory_metadata_t::memcached_namespaces)),
                                                                    metadata_field(&cluster_semilattice_metadata_t::memcached_namespaces, semilattice_manager_cluster.get_root_view()),
                                                                    metadata_field(&cluster_semilattice_metadata_t::machines, semilattice_manager_cluster.get_root_view()),
                                                                    directory_read_manager.get_root_view()->subview(
                                                                        field_getter_t<machine_id_t, cluster_directory_metadata_t>(&cluster_directory_metadata_t::machine_id)),
                                                                    &memcached_svs_source,
                                                                    &perfmon_repo);
    field_copier_t<namespaces_directory_metadata_t<memcached_protocol_t>, cluster_directory_metadata_t> memcached_reactor_directory_copier(
        &cluster_directory_metadata_t::memcached_namespaces,
        memcached_reactor_driver.get_watchable(),
        &our_root_directory_variable
        );

    namespace_repo_t<mock::dummy_protocol_t> dummy_namespace_repo(&mailbox_manager,
        directory_read_manager.get_root_view()->subview(
            field_getter_t<namespaces_directory_metadata_t<mock::dummy_protocol_t>, cluster_directory_metadata_t>(&cluster_directory_metadata_t::dummy_namespaces))
        );

    namespace_repo_t<memcached_protocol_t> memcached_namespace_repo(&mailbox_manager,
        directory_read_manager.get_root_view()->subview(
            field_getter_t<namespaces_directory_metadata_t<memcached_protocol_t>, cluster_directory_metadata_t>(&cluster_directory_metadata_t::memcached_namespaces))
        );

#ifndef NDEBUG
    boost::shared_ptr<semilattice_read_view_t<machine_semilattice_metadata_t> > machine_semilattice_view =
        /* TODO: This will crash if we are declared dead. */
        metadata_function<deletable_t<machine_semilattice_metadata_t>, machine_semilattice_metadata_t>(boost::bind(&deletable_getter<machine_semilattice_metadata_t>, _1),
            metadata_member(machine_id,
                metadata_field(&machines_semilattice_metadata_t::machines,
                    metadata_field(&cluster_semilattice_metadata_t::machines,
                        semilattice_manager_cluster.get_root_view()))));
#endif

    parser_maker_t<mock::dummy_protocol_t, mock::dummy_protocol_parser_t> dummy_parser_maker(
        &mailbox_manager,
        metadata_field(&cluster_semilattice_metadata_t::dummy_namespaces, semilattice_manager_cluster.get_root_view()),
#ifndef NDEBUG
        machine_semilattice_view,
#endif
        &dummy_namespace_repo,
        &perfmon_repo);

    parser_maker_t<memcached_protocol_t, memcache_listener_t> memcached_parser_maker(
        &mailbox_manager,
        metadata_field(&cluster_semilattice_metadata_t::memcached_namespaces, semilattice_manager_cluster.get_root_view()),
#ifndef NDEBUG
        machine_semilattice_view,
#endif
        &memcached_namespace_repo,
        &perfmon_repo);

    metadata_persistence::semilattice_watching_persister_t persister(persistent_file, machine_id, semilattice_manager_cluster.get_root_view());

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
    persister.stop_and_flush(&non_interruptor);

    return true;
}
