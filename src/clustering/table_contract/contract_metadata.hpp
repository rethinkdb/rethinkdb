// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_CONTRACT_CONTRACT_METADATA_HPP_
#define CLUSTERING_TABLE_CONTRACT_CONTRACT_METADATA_HPP_

#include <map>
#include <set>
#include <utility>

#include "errors.hpp"
#include <boost/optional.hpp>

#include "clustering/administration/tables/table_metadata.hpp"
#include "clustering/generic/raft_core.hpp"
#include "clustering/immediate_consistency/history.hpp"
#include "region/region_map.hpp"
#include "rpc/semilattice/joins/macros.hpp"   /* for EQUALITY_COMPARABLE macros */

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
overlaps R, and let `b` be the value of `current_branches` in the Raft state for some
region R'' that overlaps R. Then the following are always true unless the user issues
a manual override:
1. A majority of `c.voters` have versions on disk which descend from V (or are V) in the
    intersection of R and R'.
2. If `c.primary` is present, then `c.primary->server` has a version on disk which
    descends from V (or is V) in the intersection of R and R'.
3. The latest version on `b` descends from V (or is V) in the intersection of R and R''.
*/

class contract_t {
public:
    class primary_t {
    public:
        /* The server that's supposed to be primary. */
        server_id_t server;
        /* If we're switching to another primary, then `hand_over` is the server ID of
        the server we're switching to. */
        boost::optional<server_id_t> hand_over;
    };

    contract_t() : after_emergency_repair(false) { }

#ifndef NDEBUG
    void sanity_check() const;
#endif /* NDEBUG */

    bool is_voter(const server_id_t &s) const {
        return voters.count(s) == 1 ||
            (static_cast<bool>(temp_voters) && temp_voters->count(s) == 1);
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

    /* `after_emergency_repair` is set to `true` when we conduct an emergency repair.
    When it's `true` we'll use a different algorithm for choosing primary replicas. Once
    the table has stabilized after the repair, we'll set it back to `false`. */
    bool after_emergency_repair;
};

RDB_DECLARE_EQUALITY_COMPARABLE(contract_t::primary_t);
RDB_DECLARE_EQUALITY_COMPARABLE(contract_t);
RDB_DECLARE_SERIALIZABLE(contract_t::primary_t);
RDB_DECLARE_SERIALIZABLE(contract_t);

/* Each contract is tagged with a `contract_id_t`. If the contract changes in any way, it
gets a new ID. All the `contract_ack_t`s are tagged with the contract ID that they are
responding to. This way, the coordinator knows exactly which contract the executor is
acking.

In order to facilitiate CPU sharding, each contract's region must apply to exactly one
CPU shard. */

typedef uuid_u contract_id_t;

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
- Delete all data.
- Don't send an ack.
*/

class contract_ack_t {
public:
    enum class state_t {
        primary_need_branch,
        primary_in_progress,
        primary_ready,
        secondary_need_primary,
        secondary_backfilling,
        secondary_streaming
    };

    contract_ack_t() { }
    explicit contract_ack_t(state_t s) : state(s) { }

#ifndef NDEBUG
    void sanity_check(
        const server_id_t &server,
        const contract_id_t &contract_id,
        const table_raft_state_t &raft_state) const;
#endif

    state_t state;

    /* This is non-empty if `state` is `secondary_need_primary`. */
    boost::optional<region_map_t<version_t> > version;

    /* This is non-empty if `state` is `primary_need_branch`.
    When a new primary is first instantiated in response to a new contract_t, it
    generates a new branch ID and sends it to the coordinator through this field.
    The coordinator will then registers the branch and update the `current_branches`
    field of the Raft state. */
    boost::optional<branch_id_t> branch;

    /* This contains information about all branches mentioned in `version` or `branch` */
    branch_history_t branch_history;
};
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
    contract_ack_t::state_t, int8_t,
    contract_ack_t::state_t::primary_need_branch,
    contract_ack_t::state_t::secondary_streaming);
RDB_DECLARE_SERIALIZABLE(contract_ack_t);
RDB_DECLARE_EQUALITY_COMPARABLE(contract_ack_t);

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
            std::map<region_t, branch_id_t> register_current_branches;
            branch_history_t add_branches;
            std::set<branch_id_t> remove_branches;
            std::set<server_id_t> remove_server_names;
            server_name_map_t add_server_names;
        };

        class new_member_ids_t {
        public:
            std::set<server_id_t> remove_member_ids;
            std::map<server_id_t, raft_member_id_t> add_member_ids;
        };

        class new_server_names_t {
        public:
            server_name_map_t config_and_shards;
            server_name_map_t raft_state;
        };

        change_t() { }
        template<class T> change_t(T &&t) : v(std::move(t)) { }

        boost::variant<
            set_table_config_t,
            new_contracts_t,
            new_member_ids_t,
            new_server_names_t> v;
    };

    table_raft_state_t()
        : current_branches(region_t::universe(), nil_uuid()) { }

    void apply_change(const change_t &c);

#ifndef NDEBUG
    void sanity_check() const;
#endif /* NDEBUG */

    /* `config` is the latest user-specified config. The user can freely read and modify
    this at any time. */
    table_config_and_shards_t config;

    /* `contracts` is the `contract_t`s for the table, along with the region each one
    applies to. `contract_coordinator_t` reads and writes it; `contract_executor_t` reads
    it. */
    std::map<contract_id_t, std::pair<region_t, contract_t> > contracts;

    /* `current_branches` contains the most recently acked branch IDs for a given
    region. It gets updated by the coordinator when a new primary registers its
    branch with the coordinator through a `contract_ack_t`.
    The coordinator uses this to find out which replica has the most up to date
    data version for a given range (see `break_ack_into_fragments()` in
    coordinator.cc). */
    region_map_t<branch_id_t> current_branches;

    /* `branch_history` contains all of the branches that appear in `current_branches`,
    plus their ancestors going back to the common ancestor with the data version stored
    on the replicas. The idea is that if a replica sends us its current version and its
    branch history, we can combine the replica's branch history with this branch history
    to form a complete picture of the relationship of the replica's current data version
    to the contract's `branch`. */
    branch_history_t branch_history;

    /* `member_ids` assigns a Raft member ID to each server that's supposed to be part of
    the Raft cluster for this table. `contract_coordinator_t` writes it;
    `multi_table_manager_t` reads it and uses that information to add and remove servers
    to the Raft cluster. */
    std::map<server_id_t, raft_member_id_t> member_ids;

    /* `server_names` contains the server name of every server that's mentioned in a
    contract. This is used to display `server_status`. */
    server_name_map_t server_names;
};

RDB_DECLARE_EQUALITY_COMPARABLE(table_raft_state_t::change_t::set_table_config_t);
RDB_DECLARE_EQUALITY_COMPARABLE(table_raft_state_t::change_t::new_contracts_t);
RDB_DECLARE_EQUALITY_COMPARABLE(table_raft_state_t::change_t::new_member_ids_t);
RDB_DECLARE_EQUALITY_COMPARABLE(table_raft_state_t::change_t::new_server_names_t);
RDB_DECLARE_EQUALITY_COMPARABLE(table_raft_state_t::change_t);
RDB_DECLARE_EQUALITY_COMPARABLE(table_raft_state_t);
RDB_DECLARE_SERIALIZABLE(table_raft_state_t::change_t::set_table_config_t);
RDB_DECLARE_SERIALIZABLE(table_raft_state_t::change_t::new_contracts_t);
RDB_DECLARE_SERIALIZABLE(table_raft_state_t::change_t::new_member_ids_t);
RDB_DECLARE_SERIALIZABLE(table_raft_state_t::change_t::new_server_names_t);
RDB_DECLARE_SERIALIZABLE(table_raft_state_t::change_t);
RDB_DECLARE_SERIALIZABLE(table_raft_state_t);

/* Returns a `table_raft_state_t` for a newly-created table with the given configuration.
*/
table_raft_state_t make_new_table_raft_state(
    const table_config_and_shards_t &config);

/* `table_shard_status_t` describes the current state of the server with respect to some
range of the key-space. It's used for producing `rethinkdb.table_status`. It's designed
so that `table_shard_status_t`s from different hash-shards or different ranges can be
meaningfully combined. */
class table_shard_status_t {
public:
    table_shard_status_t() : primary(false), secondary(false), need_primary(false),
        need_quorum(false), backfilling(false), transitioning(false) { }
    void merge(const table_shard_status_t &other) {
        primary |= other.primary;
        secondary |= other.secondary;
        need_primary |= other.need_primary;
        need_quorum |= other.need_quorum;
        backfilling |= other.backfilling;
        transitioning |= other.transitioning;
    }
    bool primary;   /* server is primary */
    bool secondary;   /* server is secondary */
    bool need_primary;   /* server is secondary and waiting for primary */
    bool need_quorum;   /* server is primary and waiting for branch */
    bool backfilling;   /* server is receiving a backfill */
    bool transitioning;   /* server is in a contract, but has no ack */
};
RDB_DECLARE_EQUALITY_COMPARABLE(table_shard_status_t);
RDB_DECLARE_SERIALIZABLE(table_shard_status_t);

void debug_print(printf_buffer_t *buf, const contract_t::primary_t &primary);
void debug_print(printf_buffer_t *buf, const contract_t &contract);
void debug_print(printf_buffer_t *buf, const contract_ack_t &ack);
void debug_print(printf_buffer_t *buf, const table_raft_state_t &state);

#endif /* CLUSTERING_TABLE_CONTRACT_CONTRACT_METADATA_HPP_ */

