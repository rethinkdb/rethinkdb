// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/servers/server_metadata.hpp"

#include "containers/archive/stl_types.hpp"
#include "logger.hpp"

RDB_IMPL_SERIALIZABLE_3_SINCE_v2_1(server_config_t, name, tags, cache_size_bytes);
RDB_IMPL_EQUALITY_COMPARABLE_3(server_config_t, name, tags, cache_size_bytes);

RDB_IMPL_SERIALIZABLE_2_SINCE_v2_1(server_config_versioned_t, config, version);
RDB_IMPL_EQUALITY_COMPARABLE_2(server_config_versioned_t, config, version);

RDB_IMPL_SERIALIZABLE_1_SINCE_v2_1(server_name_map_t, names);
RDB_IMPL_EQUALITY_COMPARABLE_1(server_name_map_t, names);

RDB_IMPL_SERIALIZABLE_1(server_config_business_card_t, set_config_addr);
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(server_config_business_card_t);

name_string_t server_name_map_t::get(const server_id_t &sid) const {
    auto it = names.find(sid);
    if (it == names.end()) {
        /* Issue #7158, #6520: This can happen in production when a node is removed
        from the cluster or during cluster state transitions. We should never crash
        here - instead return a placeholder name and log the error. */
        std::string found;
        for (const auto &pair : names) {
            if (!found.empty()) {
                found += ", ";
            }
            found += uuid_to_str(pair.first.get_uuid()) + "=" +
                std::string(pair.second.second.c_str());
        }
        if (found.empty()) {
            found = "(none)";
        }
        /* Always degrade gracefully - this is a recoverable error condition */
        logERR("Internal error: Couldn't find server name for server ID. Expected: %s "
            "Found: %s This can happen during cluster state changes.", sid.print().c_str(),
            found.c_str());
        return name_string_t::guarantee_valid("__unknown_server__");
    } else {
        return it->second.second;
    }
}
