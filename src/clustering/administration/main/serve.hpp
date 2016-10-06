// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_MAIN_SERVE_HPP_
#define CLUSTERING_ADMINISTRATION_MAIN_SERVE_HPP_

#include <set>
#include <string>
#include <utility>
#include <vector>
#include <memory>

#include "clustering/administration/metadata.hpp"
#include "clustering/administration/persist/file.hpp"
#include "clustering/administration/main/version_check.hpp"
#include "arch/address.hpp"
#include "arch/io/openssl.hpp"

class os_signal_cond_t;

class invalid_port_exc_t : public std::exception {
public:
    invalid_port_exc_t(const std::string& name, int port, int port_offset) {
        if (port_offset == 0) {
            info = strprintf("%s has a value (%d) above the maximum allowed port (%d).",
                             name.c_str(), port, MAX_PORT);
        } else {
            info = strprintf("%s has a value (%d) above the maximum allowed port (%d)."
                             " Note port_offset is set to %d which may cause this error.",
                             name.c_str(), port, MAX_PORT, port_offset);
        }
    }
    ~invalid_port_exc_t() throw () { }
    const char *what() const throw () {
        return info.c_str();
    }
private:
    std::string info;
};

inline void sanitize_port(int port, const char *name, int port_offset) {
    if (port >= MAX_PORT) {
        throw invalid_port_exc_t(name, port, port_offset);
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
                            const std::set<ip_address_t> &_local_addresses_cluster,
                            const std::set<ip_address_t> &_local_addresses_driver,
                            const std::set<ip_address_t> &_local_addresses_http,
                            const peer_address_t &_canonical_addresses,
                            int _port,
                            int _client_port,
                            bool _http_admin_is_disabled,
                            int _http_port,
                            int _reql_port,
                            int _port_offset) :
        local_addresses(_local_addresses),
        local_addresses_cluster(_local_addresses_cluster),
        local_addresses_driver(_local_addresses_driver),
        local_addresses_http(_local_addresses_http),
        canonical_addresses(_canonical_addresses),
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

    std::string get_addresses_string(
        std::set<ip_address_t> actual_addresses) const;

    bool is_bind_all(std::set<ip_address_t> addresses) const;

    std::set<ip_address_t> local_addresses;
    std::set<ip_address_t> local_addresses_cluster;
    std::set<ip_address_t> local_addresses_driver;
    std::set<ip_address_t> local_addresses_http;

    peer_address_t canonical_addresses;
    int port;
    int client_port;
    bool http_admin_is_disabled;
    int http_port;
    int reql_port;
    int port_offset;
};

typedef std::shared_ptr<tls_ctx_t> shared_ssl_ctx_t;

class tls_configs_t {
public:
    shared_ssl_ctx_t web;
    shared_ssl_ctx_t driver;
    shared_ssl_ctx_t cluster;
};

peer_address_set_t look_up_peers_addresses(const std::vector<host_and_port_t> &names);

class serve_info_t {
public:
    serve_info_t(std::vector<host_and_port_t> &&_joins,
                 std::string &&_reql_http_proxy,
                 std::string &&_web_assets,
                 update_check_t _do_version_checking,
                 service_address_ports_t _ports,
                 boost::optional<std::string> _config_file,
                 std::vector<std::string> &&_argv,
                 const int _join_delay_secs,
                 const int _node_reconnect_timeout_secs,
                 tls_configs_t _tls_configs) :
        joins(std::move(_joins)),
        reql_http_proxy(std::move(_reql_http_proxy)),
        web_assets(std::move(_web_assets)),
        do_version_checking(_do_version_checking),
        ports(_ports),
        config_file(_config_file),
        argv(std::move(_argv)),
        join_delay_secs(_join_delay_secs),
        node_reconnect_timeout_secs(_node_reconnect_timeout_secs)
    {
        tls_configs = _tls_configs;
    }

    void look_up_peers() {
        peers = look_up_peers_addresses(joins);
    }

    const std::vector<host_and_port_t> joins;
    peer_address_set_t peers;
    std::string reql_http_proxy;
    std::string web_assets;
    update_check_t do_version_checking;
    service_address_ports_t ports;
    boost::optional<std::string> config_file;
    /* The original arguments, so we can display them in `server_status`. All the
    argument parsing has already been completed at this point. */
    std::vector<std::string> argv;
    int join_delay_secs;
    int node_reconnect_timeout_secs;
    tls_configs_t tls_configs;
};

/* This has been factored out from `command_line.hpp` because it takes a very
long time to compile. */

bool serve(io_backender_t *io_backender,
           const base_path_t &base_path,
           metadata_file_t *metadata_file,
           const serve_info_t &serve_info,
           os_signal_cond_t *stop_cond);

bool serve_proxy(const serve_info_t& serve_info,
                 const std::string &initial_password,
                 os_signal_cond_t *stop_cond);

#endif /* CLUSTERING_ADMINISTRATION_MAIN_SERVE_HPP_ */
