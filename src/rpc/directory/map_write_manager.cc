// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "containers/archive/archive.hpp"
#include "rpc/directory/map_write_manager.tcc"

template class directory_map_write_manager_t<int, int>;

#include "clustering/table_manager/table_metadata.hpp"
template class directory_map_write_manager_t<
    namespace_id_t, table_manager_bcard_t>;

#include "clustering/query_routing/metadata.hpp"
template class directory_map_write_manager_t<
    std::pair<namespace_id_t, uuid_u>, table_query_bcard_t>;

template class directory_map_write_manager_t<
    server_id_t, empty_value_t>;
