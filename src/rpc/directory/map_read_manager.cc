// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rpc/directory/map_read_manager.tcc"

template class directory_map_read_manager_t<int, int>;

#include "clustering/administration/tables/table_metadata.hpp"
#include "containers/archive/cow_ptr_type.hpp"
template class directory_map_read_manager_t<
    namespace_id_t, namespace_directory_metadata_t>;
