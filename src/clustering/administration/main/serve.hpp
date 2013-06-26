// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_MAIN_SERVE_HPP_
#define CLUSTERING_ADMINISTRATION_MAIN_SERVE_HPP_

#include <set>
#include <string>

#include "clustering/administration/metadata.hpp"
#include "clustering/administration/persist.hpp"
#include "arch/address.hpp"

namespace extproc { class spawner_info_t; }

#define MAX_PORT 65536

inline void sanitize_port(int port, const char *name, int port_offset) {
    if (port >= MAX_PORT) {
        if (port_offset == 0) {
            nice_crash("%s has a value (%d) above the maximum allowed port (%d).", name, port, MAX_PORT);
        } else {
            nice_crash("%s has a value (%d) above the maximum allowed port (%d). Note port_offset is set to %d which may cause this error.", name, port, MAX_PORT, port_offset);
        }
    }
}

struct service_address_ports_t {
    service_address_ports_t() :
        port(0),
        client_port(0),
        http_port(0),
        reql_port(0),
        port_offset(0) { }

    service_address_ports_t(const std::set<ip_address_t> &_local_addresses,
                            int _port,
                            int _client_port,
                            bool _http_admin_is_disabled,
                            int _http_port,
                            int _reql_port,
                            int _port_offset) :
        local_addresses(_local_addresses),
        port(_port),
        client_port(_client_port),
        http_admin_is_disabled(_http_admin_is_disabled),
        http_port(_http_port),
        reql_port(_reql_port),
        port_offset(_port_offset)
    {
            sanitize_port(port, "port", port_offset);
            sanitize_port(client_port, "client_port", port_offset);
            sanitize_port(http_port, "http_port", port_offset);
            sanitize_port(reql_port, "reql_port", port_offset);
    }

    std::string get_addresses_string() const;

    bool is_bind_all() const;

    std::set<ip_address_t> local_addresses;
    int port;
    int client_port;
    bool http_admin_is_disabled;
    int http_port;
    int reql_port;
    int port_offset;
};

/* This has been factored out from `command_line.hpp` because it takes a very
long time to compile. */

bool serve(extproc::spawner_info_t *spawner_info,
           io_backender_t *io_backender,
           const base_path_t &base_path,
           metadata_persistence::cluster_persistent_file_t *cluster_persistent_file,
           metadata_persistence::auth_persistent_file_t *auth_persistent_file,
           const peer_address_set_t &joins,
           service_address_ports_t ports,
           std::string web_assets,
           signal_t *stop_cond,
           const boost::optional<std::string>& config_file);

bool serve_proxy(extproc::spawner_info_t *spawner_info,
                 const peer_address_set_t &joins,
                 service_address_ports_t ports,
                 std::string web_assets,
                 signal_t *stop_cond,
                 const boost::optional<std::string>& config_file);

#endif /* CLUSTERING_ADMINISTRATION_MAIN_SERVE_HPP_ */
