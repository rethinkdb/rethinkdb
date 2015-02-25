// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_CONTRACT_CONTRACT_METADATA_HPP_
#define CLUSTERING_TABLE_CONTRACT_CONTRACT_METADATA_HPP_

#include "clustering/administration/tables/table_metadata.hpp"
#include "clustering/generic/raft_core.hpp"
#include "clustering/immediate_consistency/history.hpp"
#include "region/region_map.hpp"

/* `table_raft_state_t` is the type of the state that is stored in each table's Raft
instance. The most important part is a collection of `contract_t`s, which describe the
current state of the different shards. Each replica has a `contract_executor_t` for each
table, which watches the `contract_t`s and performs backfills, accepts queries, etc. in
response to what the `contract_t`s say. In addition, the `contract_executor_t` sends
`contract_ack_t`s back to the table's `contract_coordinator_t`, which initiates Raft
transactions to update the `contract_t`s as necessary to perform auto-failover, implement
the user's configuration changes, and so on.

The name `contract_t` comes from the fact that it controls the future behavior of the
`contract_coordinator_t` and the `contract_executor_t`, so it's like a "contract" between
them. The user can also be thought of as a party to the "contract", because the contract
guarantees that writes won't be discarded once they've been acked to the user. The
analogy is a bit of a stretch. */

class contract_t;
class contract_ack_t;
class table_raft_state_t;

/* We provide the following consistency guarantee: Suppose the client sends a write W
which affects some region R of the table, and the cluster acknowledges that W has been
performed. Then every future read will see W, even if servers die or auto-failover
happens, unless the administrator issues a manual override. (This assumes that the reads
and writes are not acked to the client until a majority of voters have replied.)

To provide this guarantee, we maintain a bunch of invariants. Let V be the `version_t`
which W creates. Let `c` be the `contract_t` in the Raft state for some region R' which
overlaps R. Then the following are always true unless the user issues a manual override:
1. A majority of `c.voters` have versions on disk which descend from V (or are V) in the
    intersection of R and R'.
2. If `c.primary` is present, then `c.primary->server` has a version on disk which
    descends from V (or is V) in the intersection of R and R'.
3. The latest version on `c.branch` descends from V (or is V) in the intersection of R
    and R'.
*/

class contract_t {
public:
    class primary_t {
    public:
        bool operator==(const primary_t &x) const {
            return server == x.server && hand_over == x.hand_over;
        }
        /* The server that's supposed to be primary. */
        server_id_t server;
        /* If we're switching to another primary, then `hand_over` is the server ID of
        the server we're switching to. */
        boost::optional<server_id_t> hand_over;
    };
    void sanity_check() const {
        if (static_cast<bool>(primary)) {
            guarantee(replicas.count(primary->server) == 1);
            if (static_cast<bool>(primary->hand_over) && !primary->hand_over->is_nil()) {
                guarantee(replicas.count(*primary->hand_over) == 1);
            }
        }
        for (const server_id_t &s : voters) {
            guarantee(replicas.count(s) == 1);
        }
        
    }
    bool operator==(const contract_t &x) const {
        return replicas == x.replicas && voters == x.voters &&
            temp_voters == x.temp_voters && primary == x.primary && branch == x.branch;
    }

    /* `replicas` is all the servers that are replicas for this table, whether voting or
    non-voting. `voters` is a subset of `replicas` that just contains the voting
    replicas. If we're in the middle of a transition between two sets of voters, then
    `temp_voters` will contain the new set. */
    std::set<server_id_t> replicas;
    std::set<server_id_t> voters;
    boost::optional<std::set<server_id_t> > temp_voters;

    /* `primary` contains the server that's supposed to be primary. If we're in the
    middle of a transition between two primaries, then `primary` will be empty. */
    boost::optional<primary_t> primary;

    /* `branch` tracks what's the "canonical" version of the data. */
    branch_id_t branch;
};

RDB_DECLARE_SERIALIZABLE(contract_t::primary_t);
RDB_DECLARE_SERIALIZABLE(contract_t);

/* The `contract_executor_t` looks at what each `contract_t` says about its server ID,
and reacts according to the following rules:

If `server_id == contract.primary->server`:
- Serve backfills to servers that request them
- If `contract.primary->hand_over`, then don't handle any queries. If
    `*contract.primary->hand_over` is up to date, then ack `primary_ready`. Otherwise,
    ack `primary_in_progress`.
- If `contract.branch` isn't a branch ID that we just now created, then don't handle any
    queries and report `primary_need_branch` with a branch ID.
- If `contract.primary->hand_over` is empty and `contract.branch` is the branch that we
    created, do handle queries. Ack writes to the client when a majority of `voters` and
    `temp_voters` (if present) have them on disk. If every write that happened before the
    last time the contract changed has been replicated to a majority of `voters` and
    `temp_voters` (if present), then ack `primary_ready`. Otherwise, ack
    `primary_in_progress`.

Otherwise, if `server_id` is in `contract.replicas`:
- If `contract.primary` is empty or we can't contact `contract.primary->server`, then ack
    `secondary_need_primary`.
- Otherwise, backfill data and then stream writes from `contract.primary->server`. While
    backfilling, ack `secondary_backfilling`; when done backfilling, ack
    `secondary_streaming`.

If `server_id` is not in `contract.replicas`:
- Delete all data and ack `nothing`.
*/

class contract_ack_t {
public:
    enum class state_t {
        primary_need_branch,
        primary_in_progress,
        primary_ready,
        secondary_need_primary,
        secondary_backfilling,
        secondary_streaming,
        nothing
    };

    contract_ack_t() { }
    explicit contract_ack_t(state_t s) : state(s) { }

    state_t state;

    /* This is non-empty if `state` is `secondary_need_primary`. */
    boost::optional<region_map_t<version_t> > version;

    /* This is non-empty if `state` is `primary_need_branch` */
    boost::optional<branch_id_t> branch;

    /* This contains information about all branches mentioned in `version` or `branch` */
    branch_history_t branch_history;
};

/* Each contract is tagged with a `contract_id_t`. If the contract changes in any way, it
gets a new ID. All the `contract_ack_t`s are tagged with the contract ID that they are
responding to. This way, the coordinator knows exactly which contract the executor is
acking.

In order to facilitiate CPU sharding, each contract's region must apply to exactly one
CPU shard. */

typedef uuid_u contract_id_t;

/* `table_raft_state_t` is the datum that each table's Raft cluster manages. The
`raft_member_t` type is templatized on a template paramter called `state_t`, and
`table_raft_state_t` is the concrete value that it's instantiated to. */

class table_raft_state_t {
public:
    class change_t {
    public:
        class set_table_config_t {
        public:
            table_config_and_shards_t new_config;
        };

        class new_contracts_t {
        public:
            std::set<contract_id_t> remove_contracts;
            std::map<contract_id_t, std::pair<region_t, contract_t> > add_contracts;
            std::set<branch_id_t> remove_branches;
            branch_history_t add_branches;
        };

        change_t() { }
        template<class T> change_t(T &&t) : v(t) { }

        boost::variant<set_table_config_t, new_contracts_t> v;
    };

    void apply_change(const change_t &c);
    bool operator==(const table_raft_state_t &other) const {
        return config == other.config && member_ids == other.member_ids;
    }

    /* `config` is the latest user-specified config. The user can freely read and modify
    this at any time. */
    table_config_and_shards_t config;

    /* `contracts` is the `contract_t`s for the table, along with the region each one
    applies to. `contract_coordinator_t` reads and writes it; `contract_executor_t` reads
    it. */
    std::map<contract_id_t, std::pair<region_t, contract_t> > contracts;

    /* `branch_history` contains branch history information for any branch that appears
    in the `branch` field of any contract in `contracts`.
    RSI(raft): We should prune branches if they no longer meet this condition. */
    branch_history_t branch_history;

    /* `member_ids` assigns a Raft member ID to each server that's supposed to be part of
    the Raft cluster for this table. `contract_coordinator_t` writes it;
    `table_meta_manager_t` reads it and uses that information to add and remove servers
    to the Raft cluster. */
    std::map<server_id_t, raft_member_id_t> member_ids;
};

RDB_DECLARE_SERIALIZABLE(table_raft_state_t::change_t::set_table_config_t);
RDB_DECLARE_SERIALIZABLE(table_raft_state_t::change_t::new_contracts_t);
RDB_DECLARE_SERIALIZABLE(table_raft_state_t::change_t);
RDB_DECLARE_SERIALIZABLE(table_raft_state_t);

/* Returns a `table_raft_state_t` for a newly-created table with the given configuration.
*/
table_raft_state_t make_new_table_raft_state(
    const table_config_and_shards_t &config);

#endif /* CLUSTERING_TABLE_CONTRACT_CONTRACT_METADATA_HPP_ */

