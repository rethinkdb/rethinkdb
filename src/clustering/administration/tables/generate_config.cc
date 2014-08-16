// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/tables/generate_config.hpp"

#include "clustering/administration/servers/name_client.hpp"
#include "clustering/administration/tables/split_points.hpp"

// Because being primary for a shard usually comes with a higher cost than
// being secondary, we want to consider that difference in the replica assignment.
// The concrete value of these doesn't matter, only their ratio
// (float)PRIMARY_USAGE_COST/(float)SECONDARY_USAGE_COST is important.
// As long as PRIMARY_USAGE_COST > SECONDARY_USAGE_COST, this is a solution to
// https://github.com/rethinkdb/rethinkdb/issues/344 (if the machine roles are
// otherwise equal).
#define PRIMARY_USAGE_COST  10
#define SECONDARY_USAGE_COST  8
void calculate_server_usage(
        const table_config_t &config,
        std::map<name_string_t, int> *usage) {
    for (const table_config_t::shard_t &shard : config.shards) {
        for (const name_string_t &server : shard.replica_names) {
            (*usage)[server] += SECONDARY_USAGE_COST;
        }
        (*usage)[shard.director_names[0]] += (PRIMARY_USAGE_COST - SECONDARY_USAGE_COST);
    }
}

/* `validate_params()` checks if `params` are legal. */
static bool validate_params(
        const table_generate_config_params_t &params,
        const std::map<name_string_t, std::set<name_string_t> > &servers_with_tags,
        std::string *error_out) {
    if (params.num_shards <= 0) {
        *error_out = "Every table must have at least one shard.";
        return false;
    }
    static const int max_shards = 32;
    if (params.num_shards > max_shards) {
        *error_out = strprintf("Maximum number of shards is %d.", max_shards);
        return false;
    }
    if (params.num_replicas.count(params.director_tag) == 0 ||
            params.num_replicas.at(params.director_tag) == 0) {
        *error_out = strprintf("Can't use server tag `%s` for directors because you "
            "specified no replicas in server tag `%s`.", params.director_tag.c_str(),
            params.director_tag.c_str());
        return false;
    }
    std::map<name_string_t, name_string_t> servers_claimed;
    for (auto it = params.num_replicas.begin(); it != params.num_replicas.end(); ++it) {
        if (it->second < 0) {
            *error_out = "Can't have a negative number of replicas";
            return false;
        }
        if (it->second == 0) {
            continue;
        }
        for (const name_string_t &name : servers_with_tags.at(it->first)) {
            if (servers_claimed.count(name) == 0) {
                servers_claimed.insert(std::make_pair(name, it->first));
            } else {
                *error_out = strprintf("Server tags `%s` and `%s` overlap; both contain "
                    "server `%s`. The server tags used for replication settings for a "
                    "given table must be non-overlapping.", it->first.c_str(),
                    servers_claimed.at(name).c_str(), name.c_str());
                return false;
            }
        }
    }
    return true;
}

/* `estimate_cost_to_get_up_to_date()` returns a number that describes how much trouble
we expect it to be to get the given machine into an up-to-date state.

This takes O(shards) time, since `business_card` probably contains O(shards) activities.
*/
static double estimate_cost_to_get_up_to_date(
        const reactor_business_card_t &business_card,
        region_t shard) {
    typedef reactor_business_card_t rb_t;
    region_map_t<double> costs(shard, 3);
    for (rb_t::activity_map_t::const_iterator it = business_card.activities.begin();
            it != business_card.activities.end(); it++) {
        region_t intersection = region_intersection(it->second.region, shard);
        if (!region_is_empty(intersection)) {
            int cost;
            if (boost::get<rb_t::primary_when_safe_t>(&it->second.activity)) {
                cost = 0;
            } else if (boost::get<rb_t::primary_t>(&it->second.activity)) {
                cost = 0;
            } else if (boost::get<rb_t::secondary_up_to_date_t>(&it->second.activity)) {
                cost = 1;
            } else if (boost::get<rb_t::secondary_without_primary_t>(&it->second.activity)) {
                cost = 2;
            } else if (boost::get<rb_t::secondary_backfilling_t>(&it->second.activity)) {
                cost = 2;
            } else if (boost::get<rb_t::nothing_when_safe_t>(&it->second.activity)) {
                cost = 3;
            } else if (boost::get<rb_t::nothing_when_done_erasing_t>(&it->second.activity)) {
                cost = 3;
            } else if (boost::get<rb_t::nothing_t>(&it->second.activity)) {
                cost = 3;
            } else {
                // I don't know if this is unreachable, but cost would be uninitialized otherwise  - Sam
                // TODO: Is this really unreachable?
                unreachable();
            }
            /* It's ok to just call `set()` instead of trying to find the minimum
            because activities should never overlap. */
            costs.set(intersection, cost);
        }
    }
    double sum = 0;
    int count = 0;
    for (region_map_t<double>::iterator it = costs.begin(); it != costs.end(); it++) {
        /* TODO: Scale by how much data is in `it->first` */
        sum += it->second;
        count++;
    }
    return sum / count;
}

/* `pick_best_pairings()` chooses which server will host each shard. No server can host
more than one shard. */
static void pick_best_pairings(
        int num_shards,
        /* The map's values are pairs of (shard, server). The keys indicate how expensive
        it is for that shard to be hosted on that server; lower numbers are better. If a
        pairing is not present in the map, then it will be impossible.
        `pick_best_pairings()` will remove pairings from the map as it chooses them. */
        std::multimap< double, std::pair<int, name_string_t> > *costs,
        /* This vector will be filled with the chosen server for each shard */
        std::vector<name_string_t> *picks_out) {
    std::set<name_string_t> servers_used;
    std::set<int> shards_satisfied;
    picks_out->resize(num_shards);
    for (auto it = costs->begin(); it != costs->end();) {
        if (shards_satisfied.count(it->second.first) == 1 ||
                servers_used.count(it->second.second) == 1) {
            ++it;
            continue;
        }
        (*picks_out)[it->second.first] = it->second.second;
        shards_satisfied.insert(it->second.first);
        servers_used.insert(it->second.second);
        {
            /* Remove the selected pairing from the map, so that we won't try to put
            another replica for the same shard on the same server */
            auto jt = it;
            ++jt;
            costs->erase(it);
        }
        if (shards_satisfied.size() == static_cast<size_t>(num_shards)) {
            break;
        }
    }
    guarantee(shards_satisfied.size() == static_cast<size_t>(num_shards));
}

bool table_generate_config(
        server_name_client_t *name_client,
        namespace_id_t table_id,
        real_reql_cluster_interface_t *reql_cluster_interface,
        clone_ptr_t< watchable_t< change_tracking_map_t<peer_id_t,
            namespaces_directory_metadata_t> > > directory_view,
        const std::map<name_string_t, int> &server_usage,

        const table_generate_config_params_t &params,

        signal_t *interruptor,
        table_config_t *config_out,
        std::string *error_out) {

    /* First, fetch a list of servers with each tag mentioned in the params. The reason
    we copy this data to a local variable is that we must use the same tag lists when
    generating the configuration that we do when validating the params, but the tag lists
    returned by `name_client` could change at any time. */
    std::map<name_string_t, std::set<name_string_t> > servers_with_tags;
    for (auto it = params.num_replicas.begin(); it != params.num_replicas.end(); ++it) {
        servers_with_tags.insert(std::make_pair(
            it->first,
            name_client->get_servers_with_tag(it->first)));
    }
    if (servers_with_tags.count(params.director_tag) == 0) {
        servers_with_tags.insert(std::make_pair(
            params.director_tag,
            name_client->get_servers_with_tag(params.director_tag)));
    }

    if (!validate_params(params, servers_with_tags, error_out)) {
        return false;
    }

    /* Fetch reactor information for all of the servers */
    std::map<name_string_t, cow_ptr_t<reactor_business_card_t> > directory_metadata;
    if (table_id != nil_uuid()) {
        std::set<name_string_t> missing;
        directory_view->apply_read(
            [&](const change_tracking_map_t<peer_id_t,
                    namespaces_directory_metadata_t> *map) {
                for (auto it = servers_with_tags.begin();
                          it != servers_with_tags.end();
                        ++it) {
                    for (auto jt = it->second.begin(); jt != it->second.end(); ++jt) {
                        boost::optional<machine_id_t> machine_id =
                            name_client->get_machine_id_for_name(*jt);
                        if (!machine_id) {
                            missing.insert(*jt);
                            continue;
                        }
                        boost::optional<peer_id_t> peer_id =
                            name_client->get_peer_id_for_machine_id(*machine_id);
                        if (!peer_id) {
                            missing.insert(*jt);
                            continue;
                        }
                        auto kt = map->get_inner().find(*peer_id);
                        if (kt == map->get_inner().end()) {
                            missing.insert(*jt);
                            continue;
                        }
                        const namespaces_directory_metadata_t &peer_dir = kt->second;
                        auto lt = peer_dir.reactor_bcards.find(table_id);
                        if (lt == peer_dir.reactor_bcards.end()) {
                            /* don't raise an error in this case */
                            continue;
                        }
                        directory_metadata.insert(std::make_pair(
                            *jt, lt->second.internal));
                    }
                }
            });
        if (!missing.empty()) {
            *error_out = strprintf("Can't configure table because server `%s` is "
                "missing", missing.begin()->c_str());
            return false;
        }
    }

    config_out->shards.resize(params.num_shards);

    /* Decide on the split points for the new config */
    std::vector<store_key_t> split_points;
    if (table_id != nil_uuid()) {
        if (!calculate_split_points_with_distribution(table_id, reql_cluster_interface,
                params.num_shards, interruptor, &split_points)) {
            /* RSI(reql_admin): Use previous split points to refine estimate */
            calculate_split_points_by_estimation(params.num_shards,
                std::vector<store_key_t>(), &split_points);
        }
    } else {
        calculate_split_points_by_estimation(params.num_shards,
            std::vector<store_key_t>(), &split_points);
    }
    for (int i = 0; i < params.num_shards-1; ++i) {
        config_out->shards[i].split_point =
            boost::optional<store_key_t>(split_points[i]);
    }

    /* RSI: Make sure that there are enough servers available */

    /* Finally, fill in the servers */
    for (auto it = params.num_replicas.begin(); it != params.num_replicas.end(); ++it) {
        if (it->second == 0) {
            /* Avoid unnecessary computation and possibly spurious error messages */
            continue;
        }

        name_string_t server_tag = it->first;
        int num_in_tag = servers_with_tags.at(server_tag).size();
        if (num_in_tag < params.num_shards) {
            *error_out = strprintf("You requested %d shards, but there are only %d "
                "servers with the tag `%s`. reconfigure() requires at least as many "
                "servers with each tag as there are shards, so that it can distribute "
                "the shards across servers instead of putting multiple shards on a "
                "single server. You can work around this limitation by generating a "
                "configuration manually instead of using reconfigure().",
                params.num_shards, num_in_tag, server_tag.c_str());
            return false;
        }
        if (num_in_tag < it->second) {
            *error_out = strprintf("You requested %d replicas on servers with the tag "
                "`%s`, but there are only %d servers with the tag `%s`. It's impossible "
                "to have more replicas of the data than there are servers.",
                it->second, server_tag.c_str(), num_in_tag, server_tag.c_str());
            return false;
        }

        /* Calculate how desirable each shard-server pairing is */
        std::multimap<double, std::pair<int, name_string_t> > costs;
        for (int shard = 0; shard < params.num_shards; ++shard) {
            region_t region = hash_region_t<key_range_t>(
                config_out->get_shard_range(shard));
            for (const name_string_t &server : servers_with_tags.at(server_tag)) {
                auto usage_it = server_usage.find(server);
                int usage = (usage_it == server_usage.end()) ? 0 : usage_it->second;
                double usage_cost = usage / static_cast<double>(PRIMARY_USAGE_COST);
                double backfill_cost;
                if (table_id == nil_uuid()) {
                    auto dir_it = directory_metadata.find(server);
                    if (dir_it == directory_metadata.end()) {
                        backfill_cost = 3.0;
                    } else {
                        backfill_cost = estimate_cost_to_get_up_to_date(
                            *dir_it->second, region);
                    }
                } else {
                    /* We're creating a new table, so we won't have to backfill no matter
                    where we put the servers */
                    backfill_cost = 0;
                }
                /* We always prioritize keeping the data close to where it was before
                over distributing the data evenly. The `1000` is arbitrary. */
                double cost = backfill_cost*1000 + usage_cost;
                costs.insert(std::make_pair(cost, std::make_pair(shard, server)));
            }
            /* The above computation takes O(shards*servers*log(...)) time, because
            `estimate_cost_to_get_up_to_date` takes O(shards) time. To avoid locking up
            the server if there are very many shards or servers, yield the CPU
            periodically. */
            coro_t::yield();
            if (interruptor->is_pulsed()) {
                throw interrupted_exc_t();
            }
        }

        for (int replica = 0; replica < it->second; ++replica) {
            std::vector<name_string_t> picks;
            pick_best_pairings(params.num_shards, &costs, &picks);
            for (int shard = 0; shard < params.num_shards; ++shard) {
                config_out->shards[shard].replica_names.insert(picks[shard]);
                if (server_tag == params.director_tag) {
                    config_out->shards[shard].director_names.push_back(picks[shard]);
                }
            }
        }
    }

    return true;
}

/* TODO: This algorithm currently doesn't update the usage map as it assigns servers,
which leads to suboptimal behavior in some cases. For example, suppose the user has four
servers and asks for two shards with two replicas each. The obvious solution is to put
one replica on each server. But this algorithm will sometimes put a director for one
shard on the same machine as the non-director replica for the other shard. */

