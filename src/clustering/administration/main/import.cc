#include "clustering/administration/main/import.hpp"

// TODO: Which of these headers, other than rdb_protocol/query_language.hpp, contains rdb_protocol_t stuff?

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
#include "clustering/administration/suggester.hpp"
#include "clustering/administration/sys_stats.hpp"
#include "extproc/pool.hpp"
#include "http/json.hpp"
#include "rdb_protocol/query_language.hpp"
#include "rpc/connectivity/multiplexer.hpp"
#include "rpc/directory/read_manager.hpp"
#include "rpc/directory/write_manager.hpp"
#include "rpc/semilattice/semilattice_manager.hpp"
#include "rpc/semilattice/view/field.hpp"
#include "utils.hpp"

bool do_json_importation(machine_id_t us,
                         const boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> > &metadata,
                         const clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > > &directory,
                         namespace_repo_t<rdb_protocol_t> *repo, json_importer_t *importer,
                         json_import_target_t target,
                         signal_t *interruptor);

bool get_other_peer(const std::set<peer_id_t> &peers_list, const peer_id_t &me, peer_id_t *other_peer_out) {
    for (std::set<peer_id_t>::const_iterator it = peers_list.begin(); it != peers_list.end(); ++it) {
        if (*it != me) {
            *other_peer_out = *it;
            return true;
        }
    }
    return false;
}


bool run_json_import(extproc::spawner_t::info_t *spawner_info, peer_address_set_t joins, int ports_port, int ports_client_port, json_import_target_t target, json_importer_t *importer, signal_t *stop_cond) {

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
            connectivity_cluster.get_me(),
            get_ips(),
            stat_manager.get_address(),
            metadata_change_handler.get_request_mailbox_address(),
            log_server.get_business_card(),
            PROXY_PEER));

    message_multiplexer_t::client_t directory_manager_client(&message_multiplexer, 'D');
    // TODO: Are we going to use the write manager at all?  Could we just remove it?  Just wondering.
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

    peer_id_t other_peer;
    if (!get_other_peer(connectivity_cluster.get_peers_list(), connectivity_cluster.get_me(), &other_peer)) {
        printf("Aborting!  Could not find other peer after it connected!\n");
        return false;
    }

    // Wait for semilattice data.
    semilattice_manager_cluster.get_root_view()->sync_from(other_peer, stop_cond);

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
                               semilattice_manager_cluster.get_root_view(),
                               directory_read_manager.get_root_view(),
                               &rdb_namespace_repo, importer, target, stop_cond);
}


bool get_or_create_namespace(machine_id_t us,
                             namespace_repo_t<rdb_protocol_t> *ns_repo,
                             const boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> > &metadata,
                             const clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > > &directory,
                             database_id_t db_id,
                             boost::optional<std::string> datacenter_name,
                             std::string table_name,
                             std::string primary_key_in,
                             namespace_id_t *namespace_out,
                             signal_t *interruptor) {
    boost::shared_ptr<semilattice_read_view_t<datacenters_semilattice_metadata_t> > datacenters = metadata_field(&cluster_semilattice_metadata_t::datacenters, metadata);
    boost::shared_ptr<semilattice_readwrite_view_t<cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> > > > namespaces = metadata_field(&cluster_semilattice_metadata_t::rdb_namespaces, metadata);

    namespaces_semilattice_metadata_t<rdb_protocol_t> original_nss = *namespaces->get();
    metadata_searcher_t<namespace_semilattice_metadata_t<rdb_protocol_t> > searcher(&original_nss.namespaces);
    metadata_search_status_t error;
    std::map<namespace_id_t, deletable_t<namespace_semilattice_metadata_t<rdb_protocol_t> > >::iterator it = searcher.find_uniq(namespace_predicate_t(table_name, db_id), &error);

    bool result = true;

    *namespace_out = namespace_id_t();

    if (error == METADATA_SUCCESS) {
        std::string existing_pk = it->second.get().primary_key.get();
        if (existing_pk != primary_key_in) {
            printf("Successfully found namespace %s, with wrong primary key '%s'\n",
                   uuid_to_str(it->first).c_str(), existing_pk.c_str());
            result = false;
        } else {
            printf("Successfully found namespace %s\n", uuid_to_str(it->first).c_str());
            *namespace_out = it->first;
        }
    } else if (error == METADATA_ERR_NONE) {
        datacenter_id_t dc_id = nil_uuid();

        if (datacenter_name) {
            datacenters_semilattice_metadata_t dcs = datacenters->get();
            metadata_searcher_t<datacenter_semilattice_metadata_t> dc_searcher(&dcs.datacenters);
            metadata_search_status_t dc_error;
            std::map<datacenter_id_t, deletable_t<datacenter_semilattice_metadata_t> >::iterator jt = dc_searcher.find_uniq(*datacenter_name, &dc_error);

            if (dc_error != METADATA_SUCCESS) {
                printf("Could not find datacenter. error = %d\n", dc_error);
                return false;
            }

            dc_id = jt->first;
        }

        namespace_semilattice_metadata_t<rdb_protocol_t> ns = new_namespace<rdb_protocol_t>(us, db_id, dc_id, table_name, primary_key_in, 0 /* unused memcached port arg */, GIGABYTE);
        namespaces_semilattice_metadata_t<rdb_protocol_t> nss;
        namespace_id_t ns_id = generate_uuid();
        nss.namespaces.insert(std::make_pair(ns_id, ns));
        namespaces->join(cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> >(nss));

        // Try fill_in_blueprints twice.  Sigh.
        cluster_semilattice_metadata_t metadata_copy = metadata->get();
        bool try_another_fill_in_blueprints = false;
        try {
            fill_in_blueprints(&metadata_copy, directory->get(), us);
        } catch (missing_machine_exc_t exc) {
            try_another_fill_in_blueprints = true;
        }

        if (try_another_fill_in_blueprints) {
            nap(1000);
            metadata_copy = metadata->get();
            fill_in_blueprints(&metadata_copy, directory->get(), us);
        }

        metadata->join(metadata_copy);

        printf("Created namespace.  Waiting for readiness...\n");

        wait_for_rdb_table_readiness(ns_repo, ns_id, interruptor);

        printf("Done waiting for readiness.\n");

        *namespace_out = ns_id;
    } else {
        printf("Error searching for namespace.\n");
        result = false;
    }
    return result;
}

bool get_or_create_database(UNUSED machine_id_t us,
                            const boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> > &metadata,
                            std::string db_name, database_id_t *db_out) {
    guarantee(!db_name.empty());
    boost::shared_ptr<semilattice_readwrite_view_t<databases_semilattice_metadata_t> > databases = metadata_field(&cluster_semilattice_metadata_t::databases, metadata);
    std::map<database_id_t, deletable_t<database_semilattice_metadata_t> > dbmap = databases->get().databases;
    metadata_searcher_t<database_semilattice_metadata_t> searcher(&dbmap);

    metadata_search_status_t error;
    std::map<database_id_t, deletable_t<database_semilattice_metadata_t> >::iterator it = searcher.find_uniq(db_name, &error);

    if (error == METADATA_SUCCESS) {
        *db_out = it->first;
        return true;
    } else if (error == METADATA_ERR_NONE) {
        printf("No database named '%s' found.\n", db_name.c_str());
    } else if (error == METADATA_ERR_MULTIPLE) {
        printf("Error searching for database.  (Multiple databases are named '%s'.)\n", db_name.c_str());
    } else {
        printf("Error when searching for database named '%s'.\n", db_name.c_str());
    }

    *db_out = database_id_t();
    return false;
}


bool do_json_importation(machine_id_t us,
                         const boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> > &metadata,
                         const clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > > &directory,
                         namespace_repo_t<rdb_protocol_t> *repo, json_importer_t *importer,
                         json_import_target_t target,
                         signal_t *interruptor) {

    const std::string db_name = target.db_name;
    const boost::optional<std::string> datacenter_name = target.datacenter_name;
    const std::string table_name = target.table_name;

    database_id_t db_id;
    if (!get_or_create_database(us, metadata, db_name, &db_id)) {
        printf("could not get or create database named '%s'\n", db_name.c_str());
        return false;
    }

    namespace_id_t namespace_id;
    std::string primary_key;
    if (!get_or_create_namespace(us, repo, metadata, directory, db_id, datacenter_name, table_name, target.primary_key, &namespace_id, interruptor)) {
        printf("could not get or create table named '%s' (in db '%s')\n", table_name.c_str(), uuid_to_str(db_id).c_str());
        return false;
    }

    namespace_repo_t<rdb_protocol_t>::access_t access(repo, namespace_id, interruptor);

    namespace_interface_t<rdb_protocol_t> *ni = access.get_namespace_if();

    wait_interruptible(ni->get_initial_ready_signal(), interruptor);

    order_source_t order_source;

    int64_t num_imported_rows = 0;
    int64_t num_duplicate_pkey = 0;

    bool importation_complete = false;
    try {
        printf("Importing data...\n");
        for (scoped_cJSON_t json; importer->next_json(&json); json.reset(NULL)) {
            cJSON *pkey_value = json.GetObjectItem(target.primary_key.c_str());

            // Autogenerate primary keys for records without them
            if (!pkey_value) {
                std::string generated_pk = uuid_to_str(generate_uuid());
                pkey_value = cJSON_CreateString(generated_pk.c_str());
                json.AddItemToObject(target.primary_key.c_str(), pkey_value);
            }

            // TODO: This code is duplicated with something.  Like insert(...) in query_language.cc.
            if (pkey_value->type != cJSON_String && pkey_value->type != cJSON_Number) {
                // This cannot happen with CRSV because we only parse strings and numbers.
                printf("Primary key spotted with invalid value!  (Neither string nor number.)\n");
                continue;
            }

            std::string internal_key = cJSON_print_lexicographic(pkey_value);

            if (internal_key.size() > MAX_KEY_SIZE) {
                printf("Primary key %s too large (when used for storage), ignoring.\n", cJSON_print_std_string(pkey_value).c_str());
                continue;
            }

            store_key_t key(internal_key);

            boost::shared_ptr<scoped_cJSON_t> json_copy_fml(new scoped_cJSON_t(json.DeepCopy()));

            rdb_protocol_t::point_write_t point_write(key, json_copy_fml, false);
            rdb_protocol_t::write_t rdb_write;
            rdb_write.write = point_write;
            rdb_protocol_t::write_response_t response;
            ni->write(rdb_write, &response, order_source.check_in("do_json_importation"), interruptor);

            if (!boost::get<rdb_protocol_t::point_write_response_t>(&response.response)) {
                printf("Internal error: Attempted a point write (for key %s), but did not get a point write response.\n", cJSON_print_std_string(pkey_value).c_str());
            } else {
                rdb_protocol_t::point_write_response_t *resp = boost::get<rdb_protocol_t::point_write_response_t>(&response.response);
                switch (resp->result) {
                case DUPLICATE:
                    printf("An entry with primary key %s already exists, and has not been overwritten.\n", cJSON_print_std_string(pkey_value).c_str());
                    ++num_duplicate_pkey;
                    break;
                case STORED:
                    ++num_imported_rows;
                    break;
                default:
                    unreachable();
                }
            }
        }

        importation_complete = true;
    } catch (interrupted_exc_t exc) {
        // do nothing.
    }

    printf("%s  Successfully imported %ld row%s.  Ignored %ld row%s with duplicated primary key.  %s\n",
           importation_complete ? "Import completed." : "Interrupted, import partially completed.",
           num_imported_rows,
           num_imported_rows == 1 ? "" : "s",
           num_duplicate_pkey,
           num_duplicate_pkey == 1 ? "" : "s",
           importer->get_error_information().c_str());

    return true;
}
