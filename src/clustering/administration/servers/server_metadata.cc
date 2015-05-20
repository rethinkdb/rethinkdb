// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/servers/server_metadata.hpp"

// RSI(raft): Some of these should be `SINCE_v1_N`, where `N` is the version number at
// which Raft is released.

RDB_IMPL_SERIALIZABLE_3_SINCE_v1_16(server_config_t, name, tags, cache_size_bytes);
RDB_IMPL_EQUALITY_COMPARABLE_3(server_config_t, name, tags, cache_size_bytes);

RDB_IMPL_SERIALIZABLE_2_SINCE_v1_16(server_config_versioned_t, config, version);
RDB_IMPL_EQUALITY_COMPARABLE_2(server_config_versioned_t, config, version);

RDB_IMPL_SERIALIZABLE_1_SINCE_v1_16(server_name_map_t, names);
RDB_IMPL_EQUALITY_COMPARABLE_1(server_name_map_t, names);

RDB_IMPL_SERIALIZABLE_1(server_config_business_card_t, set_config_addr);
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(server_config_business_card_t);

name_string_t server_name_map_t::get(const server_id_t &sid) const {
    auto it = names.find(sid);
    rassert(it != names.end());
    if (it == names.end()) {
        /* This is a crash that's relatively likely to come up in production, simply
        because the code paths for copying around the server names are easy to get
        wrong. So in release mode we degrade gracefully. */
#ifndef NDEBUG
        crash("server_name_map doesn't contain the expected server");
#else
        logERR("Internal error: Couldn't find server name for server ID. Please file a "
            "bug report."); 
        return name_string_t::guarantee_valid("__unknown_server__");
#endif /* NDEBUG */
    } else {
        return it->second.second;
    }
}

