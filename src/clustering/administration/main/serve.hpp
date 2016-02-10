// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_MAIN_SERVE_HPP_
#define CLUSTERING_ADMINISTRATION_MAIN_SERVE_HPP_

#include <set>
#include <string>
#include <utility>
#include <vector>

#include <openssl/ssl.h>

#include "clustering/administration/metadata.hpp"
#include "clustering/administration/persist/file.hpp"
#include "clustering/administration/main/version_check.hpp"
#include "arch/address.hpp"

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
                            const peer_address_t &_canonical_addresses,
                            int _port,
                            int _client_port,
                            bool _http_admin_is_disabled,
                            int _http_port,
                            int _reql_port,
                            int _port_offset) :
        local_addresses(_local_addresses),
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

    std::string get_addresses_string() const;

    bool is_bind_all() const;

    std::set<ip_address_t> local_addresses;
    peer_address_t canonical_addresses;
    int port;
    int client_port;
    bool http_admin_is_disabled;
    int http_port;
    int reql_port;
    int port_offset;
};

struct tls_configs_t {
    SSL_CTX *web;
    SSL_CTX *driver;
    SSL_CTX *cluster;
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
                 tls_configs_t _tls_configs) :
        joins(std::move(_joins)),
        reql_http_proxy(std::move(_reql_http_proxy)),
        web_assets(std::move(_web_assets)),
        do_version_checking(_do_version_checking),
        ports(_ports),
        config_file(_config_file),
        argv(std::move(_argv))
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
                 os_signal_cond_t *stop_cond);

#endif /* CLUSTERING_ADMINISTRATION_MAIN_SERVE_HPP_ */
