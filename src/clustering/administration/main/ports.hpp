// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_MAIN_PORTS_HPP_
#define CLUSTERING_ADMINISTRATION_MAIN_PORTS_HPP_

#include <stdint.h>

namespace port_defaults {
const uint16_t peer_port = 29015;
const uint16_t client_port = 0;
const uint16_t http_port = 8080;

/* We currently spin up clusters with port offsets differing by 1, so
   these ports should be reasonably far apart. */
const uint16_t reql_port = 28015;

const uint16_t port_offset = 0;
}

#endif  // CLUSTERING_ADMINISTRATION_MAIN_PORTS_HPP_
