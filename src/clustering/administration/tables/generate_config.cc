// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/tables/generate_config.hpp"

#include "clustering/administration/servers/config_client.hpp"
#include "clustering/table_manager/table_meta_client.hpp"
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
        for (const server_id_t &server : shard.all_replicas) {
            (*usage)[server] += SECONDARY_USAGE_COST;
        }
        (*usage)[shard.primary_replica] += (PRIMARY_USAGE_COST - SECONDARY_USAGE_COST);
    }
}

/* `validate_params()` checks if `params` are legal. */
static void validate_params(
        const table_generate_config_params_t &params,
        const std::map<name_string_t, std::set<server_id_t> > &servers_with_tags,
        const std::map<server_id_t, name_string_t> &server_names)
        THROWS_ONLY(admin_op_exc_t) {
    if (params.num_shards <= 0) {
        throw admin_op_exc_t("Every table must have at least one shard.");
    }
    size_t total_replicas = 0;
    for (const auto &pair : params.num_replicas) {
        total_replicas += pair.second;
    }
    if (total_replicas == 0) {
        throw admin_op_exc_t("You must set `replicas` to at least one. `replicas` "
            "includes the primary replica; if there are zero replicas, there is nowhere "
            "to put the data.");
    }
    static const size_t max_shards = 32;
    if (params.num_shards > max_shards) {
        throw admin_op_exc_t(strprintf("Maximum number of shards is %zu.", max_shards));
    }
    if (params.num_replicas.count(params.primary_replica_tag) == 0 ||
            params.num_replicas.at(params.primary_replica_tag) == 0) {
        throw admin_op_exc_t(strprintf("Can't use server tag `%s` for primary replicas "
            "because you specified no replicas in server tag `%s`.",
            params.primary_replica_tag.c_str(), params.primary_replica_tag.c_str()));
    }
    for (const name_string_t &nonvoting_tag : params.nonvoting_replica_tags) {
        if (params.num_replicas.count(nonvoting_tag) == 0) {
            throw admin_op_exc_t(strprintf("You specified that the replicas in server "
                "tag `%s` should be non-voting, but you didn't specify a number of "
                "replicas in server tag `%s`.",
                nonvoting_tag.c_str(), nonvoting_tag.c_str()));
        }
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
                throw admin_op_exc_t(strprintf("Server tags `%s` and `%s` overlap; the "
                    "server `%s` has both tags. The server tags used for replication "
                    "settings for a given table must be non-overlapping.",
                    it->first.c_str(), servers_claimed.at(server).c_str(),
                    server_names.at(server).c_str()));
            }
        }
    }
}

/* `estimate_backfill_cost()` estimates the cost of backfilling the data in `range` from
the locations specified in `old_config` to the server `server`. The cost is measured in
arbitrary units that are only comparable to each other. */
double estimate_backfill_cost(
        const key_range_t &range,
        const table_config_and_shards_t &old_config,
        const server_id_t &server) {
    /* If `range` aligns perfectly to an existing shard, the cost is 0.0 if `server` is
    already primary for that shard; 1.0 if it's already secondary; and 2.0 otherwise. If
    `range` overlaps multiple existing shards' ranges, we average over all of them. */
    guarantee(!range.is_empty());
    double numerator = 0;
    int denominator = 0;
    for (size_t i = 0; i < old_config.shard_scheme.num_shards(); ++i) {
        if (old_config.shard_scheme.get_shard_range(i).overlaps(range)) {
            ++denominator;
            if (old_config.config.shards[i].primary_replica == server) {
                numerator += 0.0;
            } else if (old_config.config.shards[i].all_replicas.count(server)) {
                if (old_config.config.shards[i].nonvoting_replicas.count(server) == 0) {
                    numerator += 1.0;
                } else {
                    numerator += 2.0;
                }
            } else {
                numerator += 3.0;
            }
        }
    }
    guarantee(denominator != 0);
    return numerator / denominator;
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

void table_generate_config(
        server_config_client_t *server_config_client,
        namespace_id_t table_id,
        table_meta_client_t *table_meta_client,
        const table_generate_config_params_t &params,
        const table_shard_scheme_t &shard_scheme,
        signal_t *interruptor,
        std::vector<table_config_t::shard_t> *config_shards_out)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, failed_table_op_exc_t,
            admin_op_exc_t) {
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

    validate_params(params, servers_with_tags, server_names);

    yielder.maybe_yield(interruptor);

    /* If `table_id` is not nil, fetch the current config for this table. */
    table_config_and_shards_t old_config;
    if (!table_id.is_nil()) {
        table_meta_client->get_config(table_id, interruptor, &old_config);
    }

    /* Fetch the current configurations for all other tables, and calculate the current
    load on each server. */
    std::map<server_id_t, int> server_usage;
    std::map<namespace_id_t, table_basic_config_t> all_tables;
    table_meta_client->list_names(&all_tables);
    for (const auto &pair : all_tables) {
        if (pair.first == table_id) {
            /* Don't count the table being reconfigured in the load calculation */
            continue;
        }
        table_config_and_shards_t config;
        try {
            table_meta_client->get_config(pair.first, interruptor, &config);
        } catch (const no_such_table_exc_t &) {
            /* The table was just deleted; ignore it. */
            continue;
        } catch (const failed_table_op_exc_t &) {
            /* This can only happen if the table is located entirely on unavailable
            servers, but we won't be using unavailable servers anyway, so ignore it. */
            continue;
        }
        calculate_server_usage(config.config, &server_usage);
    }

    config_shards_out->resize(params.num_shards);

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
            throw admin_op_exc_t(strprintf("Can't put %zu replicas on servers with the "
                "tag `%s` because there are only %zu servers with the tag `%s`. It's "
                "impossible to have more replicas of the data than there are servers.",
                it->second, server_tag.c_str(), num_in_tag, server_tag.c_str()));
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
                if (!table_id.is_nil()) {
                    p.backfill_cost = estimate_backfill_cost(
                        shard_scheme.get_shard_range(shard), old_config, server);
                } else {
                    /* We're creating a new table, so we won't have to backfill no matter
                    which servers we choose. */
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
                    guarantee(config_shards_out->at(shard).primary_replica.is_unset());
                    config_shards_out->at(shard).all_replicas.insert(server);
                    config_shards_out->at(shard).primary_replica = server;
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
                (*config_shards_out)[shard].all_replicas.insert(server);
                if (params.nonvoting_replica_tags.count(it->first) == 1) {
                    (*config_shards_out)[shard].nonvoting_replicas.insert(server);
                }
            });
    }

    for (size_t shard = 0; shard < params.num_shards; ++shard) {
        guarantee(!(*config_shards_out)[shard].primary_replica.is_unset());
        guarantee((*config_shards_out)[shard].all_replicas.size() == total_replicas);
    }
}

