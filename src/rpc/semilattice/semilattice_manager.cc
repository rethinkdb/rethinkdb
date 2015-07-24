// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rpc/semilattice/semilattice_manager.tcc"

#include "clustering/administration/metadata.hpp"
template class semilattice_manager_t<cluster_semilattice_metadata_t>;
template class semilattice_manager_t<auth_semilattice_metadata_t>;
template class semilattice_manager_t<heartbeat_semilattice_metadata_t>;
