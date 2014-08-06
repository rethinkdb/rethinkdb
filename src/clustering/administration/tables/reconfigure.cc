// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/tables/reconfigure.hpp"

#include "clustering/administration/servers/name_client.hpp"

/* RSI(reql_admin): This is a barely-usable stub implementation. */
table_config_t table_reconfigure(server_name_client_t *name_client) {
    table_config_t config;
    std::vector<name_string_t> names;
    name_client->get_name_to_machine_id_map()->apply_read(
        [&](const std::map<name_string_t, machine_id_t> *map) {
            for (auto it = map->begin(); it != map->end(); ++it) {
                names.push_back(it->first);
            }
        });
    if (names.size() == 0) {
        /* This is a degenerate case. We can't possibly produce a meaningful blueprint if
        there are no named servers in existence. Maybe this should produce an error; but
        for now we hack it. */
        names.push_back(name_string_t::guarantee_valid("fake_server"));
    }
    table_config_t::shard_t shard;
    for (const name_string_t &name : names) {
        shard.replica_names.insert(name);
        shard.director_names.push_back(name);
    }
    config.shards.push_back(shard);
    return config;
}

