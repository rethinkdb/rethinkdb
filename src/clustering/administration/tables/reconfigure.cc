// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/tables/reconfigure.hpp"

#include "clustering/administration/servers/name_client.hpp"

// Because being primary for a shard usually comes with a higher cost than
// being secondary, we want to consider that difference in the replica assignment.
// The concrete value of these doesn't matter, only their ratio
// (float)PRIMARY_USAGE_COST/(float)SECONDARY_USAGE_COST is important.
// As long as PRIMARY_USAGE_COST > SECONDARY_USAGE_COST, this is a solution to
// https://github.com/rethinkdb/rethinkdb/issues/344 (if the machine roles are
// otherwise equal).
#define PRIMARY_USAGE_COST  10
#define SECONDARY_USAGE_COST  8
void calculate_usage(
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
        const table_reconfigure_params_t &params,
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
    if (num_replicas.count(director_tag) == 0 || num_replicas.at(director_tag) == 0) {
        *error_out = strprintf("Can't use server tag `%s` for directors because you "
            "specified no replicas in server tag `%s`.", director_tag.c_str(),
            director_tag.c_str());
        return false;
    }
    std::map<name_string_t, name_string_t> servers_claimed;
    for (auto it = num_replicas.begin(); it != num_replicas.end(); ++it) {
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

This takes O(shards) time. */
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

static void filter_pairings(
        std::multimap< double, std::pair<int, name_string_t> > *priorities,
        int filter_out_shard,
        name_string_t filter_out_server) {
    for (auto it = priorities->begin(); it != priorities->end(); ) {
        auto jt = it;
        ++jt;
        if (it->second.first == filter_out_shard ||
                it->second.second == filter_out_server) {
            priorities->erase(it);
        }
        it = jt;
    }
}

/* `pick_best_pairings()` chooses which server will host each shard. No server can host
more than one shard.

This takes O(shards^2*servers*log(shards*servers)) time. */
static void pick_best_pairings(
        int num_shards,
        /* The map's values are pairs of (shard, server). The keys indicate how desirable
        it is for that shard to be hosted on that server. If a pairing is not present in
        the map, then it will be impossible. */
        const std::multimap< double, std::pair<int, name_string_t> > &priorities,
        /* This keeps track of which servers have already been claimed. */
        /* This vector will be filled with the chosen server for each shard */
        std::vector<name_string_t> *picks_out) {
    



    picks_out->resize(num_shards);
    int shards_to_go = num_shards;
    while (shards_to_go > 0) {
        /* If `priorities` is empty, this means that we're stuck; it's impossible to
        place any more shards on servers. The caller is responsible for preventing this.
        */
        guarantee(!priorities.empty());
        /* Select the most desirable single pairing */
        double best_priority = priorities.back().first;
        /* If there are multiple equally desirable pairings, pick one randomly */
        int which_of_the_best = randint(priorities.count(best_priority));
        auto it = priorities.lower_bound(best_priority);
        while (which_of_the_best-- > 0) {
            ++it;
        }
        /* Record our choice */
        int pick_shard = it->second.first;
        name_string_t pick_name = it->second.second;
        (*picks_out)[pick_shard] = pick_name;
        /* Remove any pairings with the same shard or server as we just chose */
        filter_pairings(&priorities, pick_shard, pick_name);
        shards_to_go--;
    }
}

/* This takes O(replicas*shards^2*servers*log(shards*servers)) time. Assuming 100 servers
and 30 shards, that could mean multiple seconds of CPU time. We deal with this by calling
`coro_t::yield()` periodically in the computation. We call it approximately
O(replicas*shards) times. */
bool table_reconfigure(
        server_name_client_t *name_client,
        namespace_id_t table_id,
        real_reql_cluster_interface_t *reql_cluster_interface,
        clone_ptr_t< watchable_t< change_tracking_map_t<peer_id_t,
            cow_ptr_t<namespaces_directory_metadata_t> > > > directory_view,
        const std::map<name_string_t, int> &usage,

        const table_reconfigure_params_t &params,

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
            it->first,
            name_client->get_servers_with_tag(it->first)));
    }

    if (!validate_params(params, servers_with_tags)) {
        return false;
    }

    /* Fetch reactor information for all of the servers */
    std::map<name_string_t, reactor_business_card_t> directory_metadata;
    std::set<name_string_t> missing;
    directory_view->apply_read(
        [&](const change_tracking_map_t<peer_id_t,
                namespaces_directory_metadata_t> *map) {
            for (auto it = servers_with_tags.begin();
                      it != servers_with_tags.end();
                    ++it) {
                for (auto jt = it->second.begin(); jt != it->second.end(); ++jt) {
                    boost::optional<machine_id_t> machine_id =
                        name_server->get_machine_id_for_name(*jt);
                    if (!machine_id) {
                        missing.insert(*jt);
                        continue;
                    }
                    boost::optional<peer_id_t> peer_id =
                        name_server->get_peer_id_for_name(*machine_id);
                    if (!peer_id) {
                        missing.insert(*jt);
                        continue;
                    }
                    auto kt = map->get_inner().find(peer_id);
                    if (kt == map->get_inner().end()) {
                        missing.insert(*jt);
                        continue;
                    }
                    directory_metadata.insert(std::make_pair(*jt, kt->second->internal));
                }
            }
        });
    if (!missing.empty()) {
        *error_out = strprintf("Can't configure table because server `%s` is missing",
            *missing.begin());
        return false;
    }

    /* Decide on the split points for the new config */
    std::vector<store_key_t> split_points = ...;

    /* Finally, fill in the servers */
    table_config_t config;
    config.shards.resize(params.num_shards);
    for (auto it = params.num_replicas.begin(); it != params.num_replicas.end(); ++it) {
        /* Calculate how much each shard "wants" each machine */
        std::multimap<double, std::pair<int, name_string_t> > priorities;
        for (int shard = 0; shard < params.num_shards; ++shard) {
            region_t region = hash_region_t<key_range_t>(config.get_shard_range(shard));
            for (const name_string_t &server : servers_with_tags.at(it->first)) {
                auto usage_it = usage.find(server);
                int usage = (usage_it == usage.end()) ? 0 : usage_it->second;
                double usage_cost = usage / static_cast<double>(PRIMARY_USAGE_COST);
                double backfill_cost = estimate_cost_to_get_up_to_date(
                    directory_metadata.at(server), region);
                double priority = - (backfill_cost*100 + usage_cost);
                priorities.insert(std::make_pair(
                    priority, std::make_pair(shard, server)));
            }
            /* This can be a slow computation, so don't lock up the server */
            coro_t::yield();
            if (interruptor->is_pulsed()) {
                throw interrupted_exc_t();
            }
        }

        for (int replica = 0; replica < it->second; ++replica) {
            std::vector<name_string_t> picks;
            pick_best_pairings(params.num_shards, priorities, &picks);
            for (int shard = 0; shard < params.num_shards; ++shard) {
                config.shards[shard].replicas.insert(picks[shard]);
                if (it->first == params.director_tag) {
                    config.shards[shard].directors.push_back(picks[shard]);
                }
                /* Remove any pairings that use the server we just claimed */
                filter_priorities(&priorities, -1, picks[shard]);
                /* This can be a slow computation, so don't lock up the server */
                coro_t::yield();
                if (interruptor->is_pulsed()) {
                    throw interrupted_exc_t();
                }
            }
        }
    }

    return config;
}

