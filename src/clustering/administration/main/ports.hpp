#ifndef CLUSTERING_ADMINISTRATION_MAIN_PORTS_HPP_
#define CLUSTERING_ADMINISTRATION_MAIN_PORTS_HPP_

/* We currently spin up clusters with port offsets differing by 1, so
   these ports should be reasonably far apart. */
namespace base_ports {
const int rdb_protocol = 12346;
const int http_rdb_protocol = 13457;
}

namespace port_constants {
const int namespace_port = 11213;
}

namespace port_defaults {
const int peer_port = 20300;
const int client_port = 0;
const int http_port = 0;

const int port_offset = 0;
}

namespace port_offsets {
const int http = 1000;
}

#endif  // CLUSTERING_ADMINISTRATION_MAIN_PORTS_HPP_
