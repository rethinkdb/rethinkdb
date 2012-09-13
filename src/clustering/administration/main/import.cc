#include "clustering/administration/main/import.hpp"

#include "arch/io/network.hpp"
#include "clustering/administration/admin_tracker.hpp"
#include "clustering/administration/auto_reconnect.hpp"
#include "clustering/administration/issues/local.hpp"
#include "clustering/administration/logger.hpp"
#include "clustering/administration/main/initial_join.hpp"
#include "clustering/administration/main/json_import.hpp"
#include "clustering/administration/main/watchable_fields.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/network_logger.hpp"
#include "clustering/administration/perfmon_collection_repo.hpp"
#include "clustering/administration/proc_stats.hpp"
#include "clustering/administration/sys_stats.hpp"
#include "extproc/pool.hpp"
#include "http/json.hpp"
#include "rpc/connectivity/multiplexer.hpp"
#include "rpc/directory/read_manager.hpp"
#include "rpc/directory/write_manager.hpp"
#include "rpc/semilattice/semilattice_manager.hpp"
#include "rpc/semilattice/view/field.hpp"
#include "utils.hpp"

bool do_json_importation(const boost::shared_ptr<semilattice_readwrite_view_t<databases_semilattice_metadata_t> > &databases,
                         const boost::shared_ptr<semilattice_readwrite_view_t<cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> > > > &namespaces,
                         namespace_repo_t<rdb_protocol_t> *repo, json_importer_t *importer,
                         std::string db_name, std::string table_name, signal_t *interruptor);


bool run_json_import(extproc::spawner_t::info_t *spawner_info, UNUSED io_backender_t *backender, std::set<peer_address_t> joins, int ports_port, int ports_client_port, std::string db_name, std::string table_name, json_importer_t *importer, signal_t *stop_cond) {

    guarantee(spawner_info);
    extproc::pool_group_t extproc_pool_group(spawner_info, extproc::pool_group_t::DEFAULTS);

    machine_id_t machine_id = generate_uuid();

    local_issue_tracker_t local_issue_tracker;
    thread_pool_log_writer_t log_writer(&local_issue_tracker);

    connectivity_cluster_t connectivity_cluster;
    message_multiplexer_t message_multiplexer(&connectivity_cluster);
    message_multiplexer_t::client_t mailbox_manager_client(&message_multiplexer, 'M');
    mailbox_manager_t mailbox_manager(&mailbox_manager_client);
    message_multiplexer_t::client_t::run_t mailbox_manager_client_run(&mailbox_manager_client, &mailbox_manager);

    message_multiplexer_t::client_t semilattice_manager_client(&message_multiplexer, 'S');
    semilattice_manager_t<cluster_semilattice_metadata_t> semilattice_manager_cluster(&semilattice_manager_client, cluster_semilattice_metadata_t());
    message_multiplexer_t::client_t::run_t semilattice_manager_client_run(&semilattice_manager_client, &semilattice_manager_cluster);

    log_server_t log_server(&mailbox_manager, &log_writer);

    stat_manager_t stat_manager(&mailbox_manager);

    metadata_change_handler_t<cluster_semilattice_metadata_t> metadata_change_handler(&mailbox_manager, semilattice_manager_cluster.get_root_view());

    watchable_variable_t<cluster_directory_metadata_t> our_root_directory_variable(
        cluster_directory_metadata_t(
            machine_id,
            get_ips(),
            stat_manager.get_address(),
            metadata_change_handler.get_request_mailbox_address(),
            log_server.get_business_card(),
            PROXY_PEER));

    message_multiplexer_t::client_t directory_manager_client(&message_multiplexer, 'D');
    // TODO(sam): Are we going to use the write manager at all?
    directory_write_manager_t<cluster_directory_metadata_t> directory_write_manager(&directory_manager_client, our_root_directory_variable.get_watchable());
    directory_read_manager_t<cluster_directory_metadata_t> directory_read_manager(connectivity_cluster.get_connectivity_service());
    message_multiplexer_t::client_t::run_t directory_manager_client_run(&directory_manager_client, &directory_read_manager);

    network_logger_t network_logger(
        connectivity_cluster.get_me(),
        directory_read_manager.get_root_view(),
        metadata_field(&cluster_semilattice_metadata_t::machines, semilattice_manager_cluster.get_root_view()));

    message_multiplexer_t::run_t message_multiplexer_run(&message_multiplexer);
    connectivity_cluster_t::run_t connectivity_cluster_run(&connectivity_cluster, ports_port, &message_multiplexer_run, ports_client_port);

    if (0 == ports_port) {
        ports_port = connectivity_cluster_run.get_port();
    } else {
        guarantee(ports_port == connectivity_cluster_run.get_port());
    }
    logINF("Listening for intracluster traffic on port %d.\n", ports_port);

    auto_reconnector_t auto_reconnector(
        &connectivity_cluster,
        &connectivity_cluster_run,
        directory_read_manager.get_root_view()->subview(
            field_getter_t<machine_id_t, cluster_directory_metadata_t>(&cluster_directory_metadata_t::machine_id)),
        metadata_field(&cluster_semilattice_metadata_t::machines, semilattice_manager_cluster.get_root_view()));

    // Skipped field_copier_t, for fun.

    admin_tracker_t admin_tracker(
        semilattice_manager_cluster.get_root_view(), directory_read_manager.get_root_view());

    perfmon_collection_t proc_stats_collection;
    perfmon_membership_t proc_stats_membership(&get_global_perfmon_collection(), &proc_stats_collection, "proc");

    proc_stats_collector_t proc_stats_collector(&proc_stats_collection);

    perfmon_collection_t sys_stats_collection;
    perfmon_membership_t sys_stats_membership(&get_global_perfmon_collection(), &sys_stats_collection, "sys");

    const char *bs_filepath = "";
    sys_stats_collector_t sys_stats_collector(bs_filepath, &sys_stats_collection);

    scoped_ptr_t<initial_joiner_t> initial_joiner;
    if (!joins.empty()) {
        initial_joiner.init(new initial_joiner_t(&connectivity_cluster, &connectivity_cluster_run, joins));
        try {
            wait_interruptible(initial_joiner->get_ready_signal(), stop_cond);
        } catch (interrupted_exc_t) {
            return false;
        }
    }

    perfmon_collection_repo_t perfmon_repo(&get_global_perfmon_collection());

    // Namespace repos

    // (rdb only)

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

    // TODO: Handle interrupted exceptions?
    return do_json_importation(metadata_field(&cluster_semilattice_metadata_t::databases, semilattice_manager_cluster.get_root_view()),
                               metadata_field(&cluster_semilattice_metadata_t::rdb_namespaces, semilattice_manager_cluster.get_root_view()),
                               &rdb_namespace_repo, importer, db_name, table_name, stop_cond);
}

bool get_or_create_namespace(const boost::shared_ptr<semilattice_readwrite_view_t<cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> > > > &namespaces,
                             database_id_t db_id,
                             std::string table_name,
                             namespace_id_t *namespace_out) {
    namespaces_semilattice_metadata_t<rdb_protocol_t> ns = *namespaces->get();
    metadata_searcher_t<namespace_semilattice_metadata_t<rdb_protocol_t> > searcher(&ns.namespaces);
    const char *error;
    std::map<namespace_id_t, deletable_t<namespace_semilattice_metadata_t<rdb_protocol_t> > >::iterator it = searcher.find_uniq(namespace_predicate_t(table_name, db_id), &error);

    if (error != METADATA_SUCCESS) {
        *namespace_out = namespace_id_t();
        return false;
    } else {
        *namespace_out = it->first;
        return true;
    }
}

bool get_or_create_database(const boost::shared_ptr<semilattice_readwrite_view_t<databases_semilattice_metadata_t> > &databases, std::string db_name, database_id_t *db_out) {
    std::map<database_id_t, deletable_t<database_semilattice_metadata_t> > dbmap = databases->get().databases;
    metadata_searcher_t<database_semilattice_metadata_t> searcher(&dbmap);

    const char *error;
    std::map<database_id_t, deletable_t<database_semilattice_metadata_t> >::iterator it = searcher.find_uniq(db_name, &error);

    if (error != METADATA_SUCCESS) {
        // TODO(sam): Actually support _creating_ the database.
        *db_out = database_id_t();
        return false;
    } else {
        *db_out = it->first;
        return true;
    }
}


bool do_json_importation(const boost::shared_ptr<semilattice_readwrite_view_t<databases_semilattice_metadata_t> > &databases,
                         const boost::shared_ptr<semilattice_readwrite_view_t<cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> > > > &namespaces,
                         namespace_repo_t<rdb_protocol_t> *repo, json_importer_t *importer,
                         std::string db_name, std::string table_name, signal_t *interruptor) {

    database_id_t db_id;
    if (!get_or_create_database(databases, db_name, &db_id)) {
        return false;
    }

    namespace_id_t namespace_id;
    if (!get_or_create_namespace(namespaces, db_id, table_name, &namespace_id)) {
        return false;
    }

    // TODO(sam): What if construction fails?  An exception is thrown?
    namespace_repo_t<rdb_protocol_t>::access_t access(repo, namespace_id, interruptor);

    UNUSED namespace_interface_t<rdb_protocol_t> *ni = access.get_namespace_if();




    // bogus implementation
    for (scoped_cJSON_t json; importer->get_json(&json); json.reset(NULL)) {
        debugf("json: %s\n", json.Print().c_str());
    }

    debugf("do_json_importation ... returning bogus success!\n");
    return true;
}
