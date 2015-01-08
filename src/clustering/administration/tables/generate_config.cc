// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/tables/generate_config.hpp"

#include "clustering/administration/servers/config_client.hpp"
#include "containers/counted.hpp"

/* `long_calculation_yielder_t` is used in a long-running calculation to periodically
yield control of the CPU, thereby preventing locking up the server. Construct one at the
beginning of the calculation and call `maybe_yield()` regularly during the calculation.
`maybe_yield()` will sometimes call `coro_t::yield()`. The advantage over just calling
`coro_t::yield()` directly is that `long_calculation_yielder_t` won't yield unless this
coroutine has held the CPU for a long time, so it's reasonable to call it even in a tight
inner loop. It also checks the whether an interruptor signal has been pulsed. */
class long_calculation_yielder_t {
public:
    long_calculation_yielder_t() : t(get_ticks()) { }
    void maybe_yield(signal_t *interruptor) {
        ticks_t now = get_ticks();
        /* We yield every 10ms. */
        if (now > t + secs_to_ticks(1) / 100) {
            coro_t::yield();
            t = now;
        }
        if (interruptor->is_pulsed()) {
            throw interrupted_exc_t();
        }
    }
private:
    ticks_t t;
};

// Because being the primary replica for a shard usually comes with a higher cost than
// being a secondary replica, we want to consider that difference in the replica
// assignment. The concrete value of these doesn't matter, only their ratio
// (float)PRIMARY_USAGE_COST/(float)SECONDARY_USAGE_COST is important.
// As long as PRIMARY_USAGE_COST > SECONDARY_USAGE_COST, this is a solution to
// https://github.com/rethinkdb/rethinkdb/issues/344 (if the server roles are
// otherwise equal).
#define PRIMARY_USAGE_COST  10
#define SECONDARY_USAGE_COST  8
void calculate_server_usage(
        const table_config_t &config,
        std::map<server_id_t, int> *usage) {
    for (const table_config_t::shard_t &shard : config.shards) {
        for (const server_id_t &server : shard.replicas) {
            (*usage)[server] += SECONDARY_USAGE_COST;
        }
        (*usage)[shard.primary_replica] += (PRIMARY_USAGE_COST - SECONDARY_USAGE_COST);
    }
}

/* `validate_params()` checks if `params` are legal. */
static bool validate_params(
        const table_generate_config_params_t &params,
        const std::map<name_string_t, std::set<server_id_t> > &servers_with_tags,
        const std::map<server_id_t, name_string_t> &server_names,
        std::string *error_out) {
    if (params.num_shards <= 0) {
        *error_out = "Every table must have at least one shard.";
        return false;
    }
    size_t total_replicas = 0;
    for (const auto &pair : params.num_replicas) {
        total_replicas += pair.second;
    }
    if (total_replicas == 0) {
        *error_out = "You must set `replicas` to at least one. `replicas` includes the "
            "primary replica; if there are zero replicas, there is nowhere to put the "
            "data.";
        return false;
    }
    static const size_t max_shards = 32;
    if (params.num_shards > max_shards) {
        *error_out = strprintf("Maximum number of shards is %zu.", max_shards);
        return false;
    }
    if (params.num_replicas.count(params.primary_replica_tag) == 0 ||
            params.num_replicas.at(params.primary_replica_tag) == 0) {
        *error_out = strprintf("Can't use server tag `%s` for primary replicas "
                               "because you specified no replicas in server tag `%s`.",
                               params.primary_replica_tag.c_str(),
                               params.primary_replica_tag.c_str());
        return false;
    }
    std::map<server_id_t, name_string_t> servers_claimed;
    for (auto it = params.num_replicas.begin(); it != params.num_replicas.end(); ++it) {
        if (it->second == 0) {
            continue;
        }
        for (const server_id_t &server : servers_with_tags.at(it->first)) {
            if (servers_claimed.count(server) == 0) {
                servers_claimed.insert(std::make_pair(server, it->first));
            } else {
                *error_out = strprintf("Server tags `%s` and `%s` overlap; the server "
                    "`%s` has both tags. The server tags used for replication settings "
                    "for a given table must be non-overlapping.", it->first.c_str(),
                    servers_claimed.at(server).c_str(), server_names.at(server).c_str());
                return false;
            }
        }
    }
    return true;
}

/* `estimate_cost_to_get_up_to_date()` returns a number that describes how much trouble
we expect it to be to get the given server into an up-to-date state.

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

/* A `pairing_t` represents the possibility of using the given server as a replica for
the given shard.

 We sort pairings according to three variables: `self_usage_cost`, `backfill_cost`, and
`other_usage_cost`. `self_usage_cost` is the sum of `PRIMARY_USAGE_COST` and
`SECONDARY_USAGE_COST` for other shards in the same table on that server;
`other_usage_cost` is for shards of other tables on the server. `backfill_cost` is the
cost to copy data to the given server, as computed by
`estimate_cost_to_get_up_to_date()`. When comparing two pairings, we first prioritize
`self_usage_cost`, then `backfill_cost`, then `other_usage_cost`.

Because we'll be regularly updating `self_usage_cost`, we want to make updating it
inexpensive. We solve this by storing `self_usage_cost` for an entire group of pairings
(a `server_pairings_t`) simultaneously. The `server_pairings_t`s are themselves sorted
first by `self_usage_cost` and then by the cost of the cheapest internal `pairing_t`. */

class pairing_t {
public:
    double backfill_cost;
    size_t shard;
};

class server_pairings_t {
public:
    server_pairings_t() { }
    server_pairings_t(const server_pairings_t &) = default;
    server_pairings_t(server_pairings_t &&) = default;
    server_pairings_t &operator=(server_pairings_t &&) = default;

    int self_usage_cost;
    std::multiset<pairing_t> pairings;
    int other_usage_cost;
    server_id_t server;
};

bool operator<(const pairing_t &x, const pairing_t &y) {
    return x.backfill_cost < y.backfill_cost;
}

bool operator<(const counted_t<countable_wrapper_t<server_pairings_t> > &x,
               const counted_t<countable_wrapper_t<server_pairings_t> > &y) {
    guarantee(!x->pairings.empty());
    guarantee(!y->pairings.empty());
    if (x->self_usage_cost < y->self_usage_cost) {
        return true;
    } else if (x->self_usage_cost > y->self_usage_cost) {
        return false;
    } else if (*x->pairings.begin() < *y->pairings.begin()) {
        return true;
    } else if (*y->pairings.begin() < *x->pairings.begin()) {
        return false;
    } else {
        return (x->other_usage_cost < y->other_usage_cost);
    }
}

/* `pick_best_pairings()` chooses the `num_replicas` best pairings for each shard from
the given set of pairings. It reports its choices by calling `callback`. */
void pick_best_pairings(
        size_t num_shards,
        size_t num_replicas,
        std::multiset<counted_t<countable_wrapper_t<server_pairings_t> > > &&pairings,
        int usage_cost,
        long_calculation_yielder_t *yielder,
        signal_t *interruptor,
        const std::function<void(size_t, server_id_t)> &callback) {
    std::vector<size_t> shard_replicas(num_shards, 0);
    size_t total_replicas = 0;
    while (total_replicas < num_shards * num_replicas) {
        counted_t<countable_wrapper_t<server_pairings_t> > sp = *pairings.begin();
        pairings.erase(pairings.begin());
        auto it = sp->pairings.begin();
        if (shard_replicas[it->shard] < num_replicas) {
            callback(it->shard, sp->server);
            ++shard_replicas[it->shard];
            ++total_replicas;
            sp->self_usage_cost += usage_cost;
        }
        sp->pairings.erase(it);
        if (!sp->pairings.empty()) {
            pairings.insert(sp);
        }
        yielder->maybe_yield(interruptor);
    }
}

bool table_generate_config(
        server_config_client_t *server_config_client,
        namespace_id_t table_id,
        watchable_map_t<std::pair<peer_id_t, namespace_id_t>,
                        namespace_directory_metadata_t> *directory_view,
        const std::map<server_id_t, int> &server_usage,
        const table_generate_config_params_t &params,
        const table_shard_scheme_t &shard_scheme,

        signal_t *interruptor,
        table_config_t *config_out,
        std::string *error_out) {

    long_calculation_yielder_t yielder;

    /* First, make local copies of the server name map and the list of servers with each
    tag. We have to make local copies because the values returned by
    `server_config_client` could change at any time, but we need the values to be
    consistent. */
    std::map<server_id_t, name_string_t> server_names =
        server_config_client->get_server_id_to_name_map()->get();
    std::map<name_string_t, std::set<server_id_t> > servers_with_tags;
    for (auto it = params.num_replicas.begin(); it != params.num_replicas.end(); ++it) {
        std::set<server_id_t> servers =
            server_config_client->get_servers_with_tag(it->first);
        auto pair = servers_with_tags.insert(
            std::make_pair(it->first, std::set<server_id_t>()));
        for (const server_id_t &server : servers) {
            /* It's possible that due to a race condition, a server might appear in
            `servers_with_tags` but not `server_names`. We filter such servers out so
            that the code below us sees consistent data. */
            if (server_names.count(server) == 1) {
                pair.first->second.insert(server);
            }
        }
    }

    if (!validate_params(params, servers_with_tags, server_names, error_out)) {
        return false;
    }

    /* Fetch reactor information for all of the servers */
    std::map<server_id_t, cow_ptr_t<reactor_business_card_t> > directory_metadata;
    if (table_id != nil_uuid()) {
        std::set<server_id_t> disconnected;
        for (auto it = servers_with_tags.begin();
                  it != servers_with_tags.end();
                ++it) {
            for (auto jt = it->second.begin(); jt != it->second.end(); ++jt) {
                server_id_t server_id = *jt;
                boost::optional<peer_id_t> peer_id =
                    server_config_client->get_peer_id_for_server_id(server_id);
                if (!static_cast<bool>(peer_id)) {
                    disconnected.insert(server_id);
                    continue;
                }
                directory_view->read_key(std::make_pair(*peer_id, table_id),
                    [&](const namespace_directory_metadata_t *metadata) {
                        /* If this is `nullptr`, that means that the server is connected
                        but it doesn't have a reactor entry for this table. This is
                        usually because the table was just created or the server just
                        reconnected. In this case, we don't put an entry in the map, and
                        this is equivalent to assuming the server has no data for the
                        shard. */
                        if (metadata != nullptr) {
                            directory_metadata[server_id] = metadata->internal;
                        }
                    });
            }
        }
        if (!disconnected.empty()) {
            *error_out = strprintf("Can't configure table because server `%s` is "
                "disconnected", server_names.at(*disconnected.begin()).c_str());
            return false;
        }
    }

    yielder.maybe_yield(interruptor);

    config_out->shards.resize(params.num_shards);

    size_t total_replicas = 0;
    for (auto it = params.num_replicas.begin(); it != params.num_replicas.end(); ++it) {
        if (it->second == 0) {
            /* Avoid unnecessary computation and possibly spurious error messages */
            continue;
        }

        total_replicas += it->second;

        name_string_t server_tag = it->first;
        size_t num_in_tag = servers_with_tags.at(server_tag).size();
        if (num_in_tag < it->second) {
            *error_out = strprintf("Can't put %zu replicas on servers with the tag "
                "`%s` because there are only %zu servers with the tag `%s`. It's "
                "impossible to have more replicas of the data than there are servers.",
                it->second, server_tag.c_str(), num_in_tag, server_tag.c_str());
            return false;
        }

        /* Compute the desirability of each shard/server pair */
        std::map<server_id_t, server_pairings_t> pairings;
        for (const server_id_t &server : servers_with_tags.at(server_tag)) {
            server_pairings_t sp;
            sp.server = server;
            sp.self_usage_cost = 0;
            auto u_it = server_usage.find(server);
            sp.other_usage_cost = (u_it == server_usage.end()) ? 0 : u_it->second;
            for (size_t shard = 0; shard < params.num_shards; ++shard) {
                pairing_t p;
                p.shard = shard;
                if (table_id != nil_uuid()) {
                    auto dir_it = directory_metadata.find(server);
                    if (dir_it == directory_metadata.end()) {
                        p.backfill_cost = 3.0;
                    } else {
                        p.backfill_cost = estimate_cost_to_get_up_to_date(
                            *dir_it->second,
                            hash_region_t<key_range_t>(
                                shard_scheme.get_shard_range(shard)));
                    }
                } else {
                    /* We're creating a new table, so we won't have to backfill no matter
                    where we put the servers */
                    p.backfill_cost = 0;
                }
                sp.pairings.insert(p);
            }
            pairings[server] = std::move(sp);
            yielder.maybe_yield(interruptor);
        }

        /* This algorithm has a flaw; it will sometimes distribute replicas unevenly. For
        example, suppose that we have three servers, A, B, and C; three shards; and we
        want to place a primary replica and a secondary replica for each shard. We assign
        primary replicas as follows:
                    server: A B C
           primary replica: 1 2 3
        Now, it's time to assign secondary replicas. We start assigning secondaries as
        follows:
                      server: A B C
             primary replica: 1 2 3
           secondary replica: 2 1
        When it comes time to place the replica for shard 3, we cannot place it on server
        C because server C is already the primary replica for shard 3. So we have to
        place it on server A or B. So we end up with a server with 3 replicas and a
        server with only 1 replica, instead of having two replicas on each server. */

        /* First, select the primary replicas if appropriate. We select primary
        replicas separately before selecting secondary replicas because it's important
        for all the primary replicas to end up on different servers if possible. */
        if (server_tag == params.primary_replica_tag) {
            std::multiset<counted_t<countable_wrapper_t<server_pairings_t> > > s;
            for (const auto &x : pairings) {
                if (!x.second.pairings.empty()) {
                    s.insert(make_counted<countable_wrapper_t<server_pairings_t> >(
                        x.second));
                }
            }
            pick_best_pairings(
                params.num_shards,
                1,   /* only one primary replica per shard */
                std::move(s),
                PRIMARY_USAGE_COST,
                &yielder,
                interruptor,
                [&](size_t shard, const server_id_t &server) {
                    guarantee(config_out->shards[shard].primary_replica.is_unset());
                    config_out->shards[shard].replicas.insert(server);
                    config_out->shards[shard].primary_replica = server;
                    /* We have to update `pairings` as priamry replicas are selected so
                    that our second call to `pick_best_pairings()` will take into account
                    the choices made in this round. */
                    pairings[server].self_usage_cost += PRIMARY_USAGE_COST;
                    for (auto p_it = pairings[server].pairings.begin();
                              p_it != pairings[server].pairings.end();
                            ++p_it) {
                        if (p_it->shard == shard) {
                            pairings[server].pairings.erase(p_it);
                            break;
                        }
                    }
                });
        }

        /* Now select the remaining secondary replicas. */
        std::multiset<counted_t<countable_wrapper_t<server_pairings_t> > > s;
        for (const auto &x : pairings) {
            if (!x.second.pairings.empty()) {
                s.insert(make_counted<countable_wrapper_t<server_pairings_t> >(
                    std::move(x.second)));
            }
        }
        pick_best_pairings(
            params.num_shards,
            it->second - (server_tag == params.primary_replica_tag ? 1 : 0),
            std::move(s),
            SECONDARY_USAGE_COST,
            &yielder,
            interruptor,
            [&](size_t shard, const server_id_t &server) {
                config_out->shards[shard].replicas.insert(server);
            });
    }

    for (size_t shard = 0; shard < params.num_shards; ++shard) {
        guarantee(!config_out->shards[shard].primary_replica.is_unset());
        guarantee(config_out->shards[shard].replicas.size() == total_replicas);
    }

    return true;
}


