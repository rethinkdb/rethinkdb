#ifndef CLUSTERING_ADMINISTRATION_MAIN_SERVE_HPP_
#define CLUSTERING_ADMINISTRATION_MAIN_SERVE_HPP_

#include "clustering/administration/metadata.hpp"
#include "clustering/administration/persist.hpp"

struct service_ports_t {
    service_ports_t(int _port, int _client_port, int _http_port DEBUG_ONLY(, int _port_offset))
        : port(_port), client_port(_client_port), http_port(_http_port)
#ifndef NDEBUG
        , port_offset(_port_offset)
#endif
    { }

    int port;
    int client_port;
    int http_port;
#ifndef NDEBUG
    int port_offset;
#endif
};

/* This has been factored out from `command_line.hpp` because it takes a very
long time to compile. */

bool serve(io_backender_t *io_backender, const std::string &filepath, metadata_persistence::persistent_file_t *persistent_file, const std::set<peer_address_t> &joins, service_ports_t ports, machine_id_t machine_id, const cluster_semilattice_metadata_t &semilattice_metadata, std::string web_assets, signal_t *stop_cond);

bool serve_proxy(io_backender_t *io_backender, const std::string &logfilepath, const std::set<peer_address_t> &joins, service_ports_t ports, machine_id_t machine_id, const cluster_semilattice_metadata_t &semilattice_metadata, std::string web_assets, signal_t *stop_cond);

#endif /* CLUSTERING_ADMINISTRATION_MAIN_SERVE_HPP_ */
