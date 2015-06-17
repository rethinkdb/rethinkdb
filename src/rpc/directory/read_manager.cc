// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rpc/directory/read_manager.tcc"

template class directory_read_manager_t<int>;

#include "clustering/administration/metadata.hpp"
template class directory_read_manager_t<cluster_directory_metadata_t>;

