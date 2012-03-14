#include "arch/arch.hpp"
#include "arch/os_signal.hpp"
#include "clustering/administration/http_server.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/persist.hpp"
#include "clustering/administration/reactor_driver.hpp"
#include "clustering/main/serve.hpp"
#include "http/file_server.hpp"
#include "http/routing_server.hpp"
#include "http/routing_server.hpp"
#include "memcached/clustering.hpp"
#include "mock/dummy_protocol.hpp"
#include "mock/dummy_protocol_parser.hpp"
#include "rpc/connectivity/cluster.hpp"
#include "rpc/connectivity/cluster.hpp"
#include "rpc/connectivity/multiplexer.hpp"
#include "rpc/directory/manager.hpp"
#include "rpc/mailbox/mailbox.hpp"
#include "rpc/semilattice/semilattice_manager.hpp"

bool serve(const std::string &filepath, const std::vector<peer_address_t> &joins, int port, machine_id_t machine_id, const cluster_semilattice_metadata_t &semilattice_metadata) {

    std::cout << "Establishing cluster node on port " << port << "..." << std::endl;

    connectivity_cluster_t connectivity_cluster; 
    message_multiplexer_t message_multiplexer(&connectivity_cluster);

    message_multiplexer_t::client_t mailbox_manager_client(&message_multiplexer, 'M');
    mailbox_manager_t mailbox_manager(&mailbox_manager_client);
    message_multiplexer_t::client_t::run_t mailbox_manager_client_run(&mailbox_manager_client, &mailbox_manager);

    message_multiplexer_t::client_t semilattice_manager_client(&message_multiplexer, 'S');
    semilattice_manager_t<cluster_semilattice_metadata_t> semilattice_manager_cluster(&semilattice_manager_client, semilattice_metadata);
    message_multiplexer_t::client_t::run_t semilattice_manager_client_run(&semilattice_manager_client, &semilattice_manager_cluster);

    message_multiplexer_t::client_t directory_manager_client(&message_multiplexer, 'D');
    directory_readwrite_manager_t<cluster_directory_metadata_t> directory_manager(&directory_manager_client, cluster_directory_metadata_t(machine_id));
    message_multiplexer_t::client_t::run_t directory_manager_client_run(&directory_manager_client, &directory_manager);

    message_multiplexer_t::run_t message_multiplexer_run(&message_multiplexer);
    connectivity_cluster_t::run_t connectivity_cluster_run(&connectivity_cluster, port, &message_multiplexer_run);

    for (int i = 0; i < (int)joins.size(); i++) {
        connectivity_cluster_run.join(joins[i]);
    }

    /* TODO: Warn if the `join()` didn't go through. */

    metadata_persistence::semilattice_watching_persister_t persister(filepath, machine_id, semilattice_manager_cluster.get_root_view());

    reactor_driver_t<mock::dummy_protocol_t> dummy_reactor_driver(&mailbox_manager,
                                                                  directory_manager.get_root_view()->subview(field_lens(&cluster_directory_metadata_t::dummy_namespaces)), 
                                                                  metadata_field(&cluster_semilattice_metadata_t::dummy_namespaces, semilattice_manager_cluster.get_root_view()),
                                                                  directory_manager.get_root_view()->subview(field_lens(&cluster_directory_metadata_t::machine_id)),
                                                                  filepath);

    reactor_driver_t<memcached_protocol_t> memcached_reactor_driver(&mailbox_manager,
                                                                    directory_manager.get_root_view()->subview(field_lens(&cluster_directory_metadata_t::memcached_namespaces)), 
                                                                    metadata_field(&cluster_semilattice_metadata_t::memcached_namespaces, semilattice_manager_cluster.get_root_view()),
                                                                    directory_manager.get_root_view()->subview(field_lens(&cluster_directory_metadata_t::machine_id)),
                                                                    filepath);

    mock::dummy_protocol_parser_maker_t dummy_parser_maker(&mailbox_manager, 
                                                           metadata_field(&cluster_semilattice_metadata_t::dummy_namespaces, semilattice_manager_cluster.get_root_view()),
                                                           directory_manager.get_root_view()->subview(field_lens(&cluster_directory_metadata_t::dummy_namespaces)));

    memcached_parser_maker_t mc_parser_maker(&mailbox_manager, 
                                             metadata_field(&cluster_semilattice_metadata_t::memcached_namespaces, semilattice_manager_cluster.get_root_view()),
                                             directory_manager.get_root_view()->subview(field_lens(&cluster_directory_metadata_t::memcached_namespaces)));

    //TODO obviously this needs to go away from this function.
    blueprint_http_server_t bp_server(semilattice_manager_cluster.get_root_view(),
                                      directory_manager.get_root_view(),
                                      machine_id);
    std::set<std::string> white_list;
    white_list.insert("/cluster.css");
    white_list.insert("/cluster.html");
    white_list.insert("/cluster-min.js");
    white_list.insert("/favicon.ico");
    white_list.insert("/js/backbone.js");
    white_list.insert("/js/bootstrap/bootstrap-alert.js");
    white_list.insert("/js/bootstrap/bootstrap-modal.js");
    white_list.insert("/js/bootstrap/bootstrap-tab.js");
    white_list.insert("/js/d3.v2.min.js");
    white_list.insert("/js/date-en-US.js");
    white_list.insert("/js/flot/jquery.flot.js");
    white_list.insert("/js/flot/jquery.flot.resize.js");
    white_list.insert("/js/handlebars-1.0.0.beta.6.js");
    white_list.insert("/js/jquery-1.7.1.min.js");
    white_list.insert("/js/jquery.form.js");
    white_list.insert("/js/jquery.hotkeys.js");
    white_list.insert("/js/jquery.sparkline.min.js");
    white_list.insert("/js/jquery.timeago.js");
    white_list.insert("/js/jquery.validate.min.js");
    white_list.insert("/js/underscore-min.js");
    http_file_server_t file_server(white_list, "../build/debug/web");

    std::map<std::string, http_server_t *> routes;
    routes["ajax"] = & bp_server;

    routing_server_t server(port + 1000, &file_server, routes);

    std::cout << "Server started; send SIGINT to stop." << std::endl;

    wait_for_sigint();

    return true;
}
