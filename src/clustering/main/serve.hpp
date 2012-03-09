#ifndef __CLUSTERING_MAIN_SERVE_HPP__
#define __CLUSTERING_MAIN_SERVE_HPP__

#include "clustering/administration/metadata.hpp"

/* This has been factored out from `command_line.hpp` because it takes a very
long time to compile. */

bool serve(const std::string &filepath, const std::vector<peer_address_t> &joins, int port, machine_id_t machine_id, const cluster_semilattice_metadata_t &semilattice_metadata);

#endif /* __CLUSTERING_ADMINISTRATION_SERVE_HPP__ */
