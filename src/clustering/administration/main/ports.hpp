#ifndef CLUSTERING_ADMINISTRATION_MAIN_PORTS
#define CLUSTERING_ADMINISTRATION_MAIN_PORTS

/* We currently spin up clusters with port offsets differing by 1, so
   these ports should be reasonably far apart. */
namespace base_ports {
const int rdb_protocol = 12346;
const int http_rdb_protocol = 13457;
}

#endif //CLUSTERING_ADMINISTRATION_MAIN_PORTS
