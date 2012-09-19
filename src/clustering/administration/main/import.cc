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

bool do_json_importation(machine_id_t machine_id,
                         const boost::shared_ptr<semilattice_readwrite_view_t<databases_semilattice_metadata_t> > &databases,
                         const boost::shared_ptr<semilattice_read_view_t<datacenters_semilattice_metadata_t> > &datacenters,
                         const boost::shared_ptr<semilattice_readwrite_view_t<cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> > > > &namespaces,
                         namespace_repo_t<rdb_protocol_t> *repo, json_importer_t *importer,
                         json_import_target_t target,
                         signal_t *interruptor);


bool run_json_import(extproc::spawner_t::info_t *spawner_info, std::set<peer_address_t> joins, int ports_port, int ports_client_port, json_import_target_t target, json_importer_t *importer, signal_t *stop_cond) {

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
    return do_json_importation(machine_id,
                               metadata_field(&cluster_semilattice_metadata_t::databases, semilattice_manager_cluster.get_root_view()),
                               metadata_field(&cluster_semilattice_metadata_t::datacenters, semilattice_manager_cluster.get_root_view()),
                               metadata_field(&cluster_semilattice_metadata_t::rdb_namespaces, semilattice_manager_cluster.get_root_view()),
                               &rdb_namespace_repo, importer, target, stop_cond);
}

bool get_or_create_namespace(machine_id_t us,
                             const boost::shared_ptr<semilattice_read_view_t<datacenters_semilattice_metadata_t> > &datacenters,
                             const boost::shared_ptr<semilattice_readwrite_view_t<cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> > > > &namespaces,
                             database_id_t db_id,
                             boost::optional<std::string> datacenter_name,
                             std::string table_name,
                             boost::optional<std::string> maybe_primary_key,
                             namespace_id_t *namespace_out,
                             std::string *primary_key_out) {
    namespaces_semilattice_metadata_t<rdb_protocol_t> ns = *namespaces->get();
    metadata_searcher_t<namespace_semilattice_metadata_t<rdb_protocol_t> > searcher(&ns.namespaces);
    metadata_search_status_t error;
    std::map<namespace_id_t, deletable_t<namespace_semilattice_metadata_t<rdb_protocol_t> > >::iterator it = searcher.find_uniq(namespace_predicate_t(table_name, db_id), &error);

    if (error == METADATA_SUCCESS) {
        std::string existing_pk = it->second.get().primary_key.get();
        if (maybe_primary_key) {
            if (existing_pk != *maybe_primary_key) {
                *namespace_out = namespace_id_t();
                primary_key_out->clear();
                return false;
            }
        }
        *primary_key_out = existing_pk;
        *namespace_out = it->first;
        return true;
    } else if (error == METADATA_ERR_NONE) {
        // We need a primary key if we are to be creating a namespace.
        if (!maybe_primary_key) {
            *namespace_out = namespace_id_t();
            primary_key_out->clear();
            return false;
        }
        std::string primary_key = *maybe_primary_key;

        if (!datacenter_name) {
            *namespace_out = namespace_id_t();
            primary_key_out->clear();
            return false;
        }

        datacenters_semilattice_metadata_t dcs = datacenters->get();
        metadata_searcher_t<datacenter_semilattice_metadata_t> dc_searcher(&dcs.datacenters);
        metadata_search_status_t dc_error;
        std::map<datacenter_id_t, deletable_t<datacenter_semilattice_metadata_t> >::iterator it = dc_searcher.find_uniq(*datacenter_name, &dc_error);

        if (dc_error != METADATA_SUCCESS) {
            // TODO(sam): Add a way to produce a good error message.
            *namespace_out = namespace_id_t();
            primary_key_out->clear();
            return false;
        }

        datacenter_id_t dc_id = it->first;

        namespace_semilattice_metadata_t<rdb_protocol_t> ns = new_namespace<rdb_protocol_t>(us, db_id, dc_id, table_name, primary_key, 0 /* unused memcached port arg */, GIGABYTE);
        namespaces_semilattice_metadata_t<rdb_protocol_t> nss;
        namespace_id_t ns_id = generate_uuid();
        nss.namespaces.insert(std::make_pair(ns_id, ns));
        namespaces->join(cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> >(nss));

        *namespace_out = ns_id;
        *primary_key_out = primary_key;
        return true;
    } else {
        // TODO(sam): Add a way to produce a good error message.
        *namespace_out = namespace_id_t();
        primary_key_out->clear();
        return false;
    }
}

bool get_or_create_database(machine_id_t us,
                            const boost::shared_ptr<semilattice_readwrite_view_t<databases_semilattice_metadata_t> > &databases,
                            std::string db_name, database_id_t *db_out) {
    std::map<database_id_t, deletable_t<database_semilattice_metadata_t> > dbmap = databases->get().databases;
    metadata_searcher_t<database_semilattice_metadata_t> searcher(&dbmap);

    metadata_search_status_t error;
    std::map<database_id_t, deletable_t<database_semilattice_metadata_t> >::iterator it = searcher.find_uniq(db_name, &error);

    if (error == METADATA_SUCCESS) {
        *db_out = it->first;
        return true;
    } else if (error == METADATA_ERR_NONE) {
        databases_semilattice_metadata_t dbs;
        database_semilattice_metadata_t db;
        db.name = vclock_t<std::string>(db_name, us);
        database_id_t db_id = generate_uuid();
        dbs.databases.insert(std::make_pair(db_id, db));

        databases->join(dbs);

        *db_out = db_id;
        return true;
    } else {
        *db_out = database_id_t();
        return false;
    }
}


bool do_json_importation(machine_id_t us,
                         const boost::shared_ptr<semilattice_readwrite_view_t<databases_semilattice_metadata_t> > &databases,
                         const boost::shared_ptr<semilattice_read_view_t<datacenters_semilattice_metadata_t> > &datacenters,
                         const boost::shared_ptr<semilattice_readwrite_view_t<cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> > > > &namespaces,
                         namespace_repo_t<rdb_protocol_t> *repo, json_importer_t *importer,
                         json_import_target_t target,
                         signal_t *interruptor) {
    std::string db_name = target.db_name;
    boost::optional<std::string> datacenter_name = target.datacenter_name;
    std::string table_name = target.table_name;
    boost::optional<std::string> maybe_primary_key = target.primary_key;

    database_id_t db_id;
    if (!get_or_create_database(us, databases, db_name, &db_id)) {
        debugf("could not get or create database named '%s'\n", db_name.c_str());
        return false;
    }

    namespace_id_t namespace_id;
    std::string primary_key;
    if (!get_or_create_namespace(us, datacenters, namespaces, db_id, datacenter_name, table_name, maybe_primary_key, &namespace_id, &primary_key)) {
        debugf("could not get or create table named '%s' (in db '%s')\n", table_name.c_str(), uuid_to_str(db_id).c_str());
        return false;
    }

    // TODO(sam): What if construction fails?  An exception is thrown?
    namespace_repo_t<rdb_protocol_t>::access_t access(repo, namespace_id, interruptor);

    namespace_interface_t<rdb_protocol_t> *ni = access.get_namespace_if();

    order_source_t order_source;

    for (scoped_cJSON_t json; importer->get_json(&json); json.reset(NULL)) {
        debugf("json: %s\n", json.Print().c_str());

        cJSON *pkey_value = json.GetObjectItem(primary_key.c_str());
        if (!pkey_value) {
            debugf("pkey_value NULL\n");
            // TODO(sam): We want to silently ignore?
            continue;
        }

        if (pkey_value->type != cJSON_String && pkey_value->type != cJSON_Number) {
            debugf("pkey_value->type is bad\n");
            // TODO(sam): We want to silently ignore?
            continue;
        }

        std::string internal_key = cJSON_print_lexicographic(pkey_value);

        if (internal_key.size() > MAX_KEY_SIZE) {
            debugf("key size is overlarge\n");
            // TODO(sam): We want to silently ignore?
            continue;
        }

        store_key_t key(internal_key);

        boost::shared_ptr<scoped_cJSON_t> json_copy_fml(new scoped_cJSON_t(json.DeepCopy()));

        rdb_protocol_t::point_write_t point_write(key, json_copy_fml, true);
        rdb_protocol_t::write_t rdb_write;
        rdb_write.write = point_write;
        rdb_protocol_t::write_response_t response;
        ni->write(rdb_write, &response, order_source.check_in("do_json_importation"), interruptor);

        if (!boost::get<rdb_protocol_t::point_write_response_t>(&response.response)) {
            debugf("did not get a point write response\n");
        } else {
            rdb_protocol_t::point_write_response_t *resp = boost::get<rdb_protocol_t::point_write_response_t>(&response.response);
            debugf("point write response: %s\n", resp->result == STORED ? "STORED" : resp->result == DUPLICATE ? "DUPLICATE" : "???");
        }

        // We don't care about the response.
    }

    debugf("do_json_importation ... returning bogus success!\n");
    return true;
}
