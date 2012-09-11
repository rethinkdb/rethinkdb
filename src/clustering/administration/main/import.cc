#include "clustering/administration/main/import.hpp"

#include "arch/io/network.hpp"
#include "clustering/administration/issues/local.hpp"
#include "clustering/administration/logger.hpp"
#include "clustering/administration/main/json_import.hpp"
#include "clustering/administration/metadata.hpp"
#include "http/json.hpp"
#include "rpc/connectivity/multiplexer.hpp"
#include "rpc/semilattice/semilattice_manager.hpp"
#include "utils.hpp"

bool run_json_import(UNUSED io_backender_t *backender, UNUSED std::set<peer_address_t> peers, UNUSED int client_port, UNUSED std::string db_table, UNUSED json_importer_t *importer) {

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





    scoped_cJSON_t json;
    for (scoped_cJSON_t json; importer->get_json(&json); json.reset(NULL)) {
        debugf("json: %s\n", json.Print().c_str());
    }


    debugf("run_json_import... returning bogus success!\n");
    return true;
}
