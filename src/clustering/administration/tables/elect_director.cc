// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/tables/elect_director.hpp"

#include "clustering/administration/namespace_metadata.hpp"
#include "clustering/administration/servers/name_client.hpp"

/* RSI(reql_admin): This is a barely-usable stub implementation. */
std::vector<machine_id_t> table_elect_directors(
        const table_config_t &config,
        server_name_client_t *name_client) {
    std::vector<machine_id_t> directors(config.shards.size());
    for (size_t i = 0; i < config.shards.size(); i++) {
        directors[i] = nil_uuid();
        for (const name_string_t &name : config.shards[i].director_names) {
            bool found = false;
            name_client->get_name_to_machine_id_map()->apply_read(
                [&](const std::multimap<name_string_t, machine_id_t> *name_map) {
                    auto it = name_map->find(name);
                    if (it != name_map->end()) {
                        directors[i] = it->second;
                        found = true;
                    }
                });
            if (found) {
                break;
            }
        }
    }
    return directors;
}

