// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rpc/directory/map_read_manager.tcc"

/* for unit tests */
template class directory_map_read_manager_t<int, int>;

#include "clustering/table_manager/table_metadata.hpp"
template class directory_map_read_manager_t<
    namespace_id_t, table_meta_business_card_t>;
