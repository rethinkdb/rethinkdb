#ifndef CLUSTERING_ADMINISTRATION_MAIN_PORTS_HPP_
#define CLUSTERING_ADMINISTRATION_MAIN_PORTS_HPP_

namespace port_defaults {
const int peer_port = 29015;
const int client_port = 0;
const int http_port = 8080;

/* We currently spin up clusters with port offsets differing by 1, so
   these ports should be reasonably far apart. */
const int reql_port = 28015;

const int port_offset = 0;
}

#endif  // CLUSTERING_ADMINISTRATION_MAIN_PORTS_HPP_
