// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_MAIN_SERVE_HPP_
#define CLUSTERING_ADMINISTRATION_MAIN_SERVE_HPP_

#include <set>
#include <string>

#include "clustering/administration/metadata.hpp"
#include "clustering/administration/persist.hpp"
#include "extproc/spawner.hpp"
#include "arch/address.hpp"

struct service_address_ports_t {
    service_address_ports_t() : port_offset(0) { }

    service_address_ports_t(const std::set<ip_address_t> &_local_addresses,
                            portno_t _port,
                            portno_t _client_port,
                            bool _http_admin_is_disabled,
                            portno_t _http_port,
                            portno_t _reql_port,
                            int _port_offset) :
        local_addresses(_local_addresses),
        port(_port),
        client_port(_client_port),
        http_admin_is_disabled(_http_admin_is_disabled),
        http_port(_http_port),
        reql_port(_reql_port),
        port_offset(_port_offset) {
        // TODO: Figure out if port_offset is used anywhere.  Previous code suggested that it had
        // already been applied.
    }

    std::string get_addresses_string() const;

    bool is_bind_all() const;

    std::set<ip_address_t> local_addresses;
    portno_t port;
    portno_t client_port;
    bool http_admin_is_disabled;
    portno_t http_port;
    portno_t reql_port;
    int port_offset;
};

/* This has been factored out from `command_line.hpp` because it takes a very
long time to compile. */

bool serve(extproc::spawner_t::info_t *spawner_info,
           io_backender_t *io_backender,
           const std::string &filepath,
           metadata_persistence::persistent_file_t *persistent_file,
           const peer_address_set_t &joins,
           service_address_ports_t ports,
           machine_id_t machine_id,
           const cluster_semilattice_metadata_t &semilattice_metadata,
           std::string web_assets,
           signal_t *stop_cond,
           const boost::optional<std::string>& config_file);

bool serve_proxy(extproc::spawner_t::info_t *spawner_info,
                 const peer_address_set_t &joins,
                 service_address_ports_t ports,
                 machine_id_t machine_id,
                 const cluster_semilattice_metadata_t &semilattice_metadata,
                 std::string web_assets,
                 signal_t *stop_cond,
                 const boost::optional<std::string>& config_file);

#endif /* CLUSTERING_ADMINISTRATION_MAIN_SERVE_HPP_ */
