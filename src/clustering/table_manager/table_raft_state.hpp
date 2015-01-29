// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_MANAGER_TABLE_RAFT_STATE_HPP_
#define CLUSTERING_TABLE_MANAGER_TABLE_RAFT_STATE_HPP_

#include "clustering/administration/tables/table_metadata.hpp"
#include "clustering/generic/raft_core.hpp"

namespace table_raft {

/* `table_raft::state_t` is the type of the state that is stored in each table's Raft
instance. The most important part is a collection of `contract_t`s, which describe the
current state of the different shards. The table's replicas watch the `contract_t`s and
perform backfills, accept queries, etc. in response to what the `contract_t`s say. In
addition, they send `contract_ack_t`s and some other metadata back to the table's Raft
leader, which updates the `contract_t`s as necessary to perform auto-failover, implement
the user's configuration changes, and so on. */

class state_t;
class contract_t;
enum class contract_ack_t;

/* We provide the following consistency guarantee: Suppose the client sends a write W
which affects some region R of the table, and the cluster acknowledges that W has been
performed. Then every future read will see W, even if servers die or auto-failover
happens, unless the administrator issues a manual override. (This assumes that the reads
and writes are run with the highest level of consistency guarantees.)

To provide this guarantee, we maintain a bunch of invariants. Let V be the `version_t`
of the shard after W has been performed. Let `c` be the `contract_t` in the Raft state
for some region R' which overlaps R. Then the following are always true unless the user
issues a manual override:
1. A majority of `c.voters` have versions on disk which descend from V (or are V) in the
    intersection of R and R'.
2. If `c.primary` is present, then `c.primary->server` has a version on disk which
    descends from V (or is V) in the intersection of R and R'.
3. The latest version on `c.branch` is descended from V (or is V) in the intersection of
    R and R'.
*/

class contract_t {
public:
    class primary_t {
    public:
        server_id_t server;
        bool warm_shutdown;
        server_id_t warm_shutdown_for;
    };
    void sanity_check() const {
        if (static_cast<bool>(primary)) {
            guarantee(replicas.count(primary->server) == 1);
            guarantee(warm_shutdown || warm_shutdown_for.is_nil());
            if (!warm_shutdown_for.is_nil()) {
                guarantee(replicas.count(warm_shutdown_for) == 1);
            }
        }
        for (const server_id_t &s : voters) {
            guarantee(replicas.count(s) == 1);
        }
        
    }
    std::set<server_id_t> replicas;
    std::set<server_id_t> voters;
    boost::optional<std::set<server_id_t> > temp_voters;
    boost::optional<primary_t> primary;
    branch_id_t branch;
};

/* Each replica looks at what each `contract_t` says about its server ID, and reacts
according to the following rules:

If `server_id == contract.primary->server`:
- Serve backfills to servers that request them
- If we restarted since `contract.branch` was created, don't handle any queries, and ack
    `primary_need_new_branch`. This is because another replica might have more up-to-date
    data on `contract.branch` than we do, so it's not safe to continue adding writes to
    `contract.branch`.
- If `contract.primary->warm_shutdown`, then don't handle any queries. If
    `contract.primary->hand_over_to` is up to date, or it's nil and any member of
    `voters` is up to date, then ack `primary_ready`. Otherwise, ack
    `primary_in_progress`.
- Otherwise, do handle queries. Ack writes to the client when a majority of `voters` and
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

If the replica acks `primary_need_new_branch` or `secondary_need_primary`, it sends with
the ack a `region_map_t<version_t>` describing the data it has on disk, as well as branch
history for the `version_t`s in the map. */

enum class contract_ack_t {
    primary_need_new_branch,
    primary_in_progress,
    primary_ready,
    secondary_need_primary,
    secondary_backfilling,
    secondary_streaming,
    nothing
};

/* Each contract is tagged with a `contract_id_t`. If the contract changes in any way, it
gets a new ID. All the `contract_ack_t`s are tagged with the contract ID that they are
responding to. This way, the leader knows exactly which contract the follower is acking.
*/

typedef uuid_u contract_id_t;

class state_t {
public:
    class change_t {
    public:
        class set_table_config_t {
        public:
            table_config_and_shards_t new_config;
        };

        class new_contracts_t {
        public:
            std::set<contract_id_t> to_remove;
            std::map<constract_id_t, std::pair<key_range_t, contract_t> > to_add;
        };

        change_t() { }
        template<class T>
        change_t(T &&t) : v(t) { }

        boost::variant<set_table_config_t, new_contracts_t> v;
    };

    void apply_change(const change_t &c);
    bool operator==(const table_raft_state_t &other) const {
        return config == other.config && member_ids == other.member_ids;
    }

    table_config_and_shards_t config;
    std::map<contract_id_t, std::pair<key_range_t, contract_t> > contracts;
    std::map<server_id_t, raft_member_id_t> member_ids;
};

RDB_DECLARE_SERIALIZABLE(table_raft_state_t::change_t::set_table_config_t);
RDB_DECLARE_SERIALIZABLE(table_raft_state_t::change_t);
RDB_DECLARE_SERIALIZABLE(table_raft_state_t);

} /* namespace table_raft */

#endif /* CLUSTERING_TABLE_MANAGER_TABLE_RAFT_STATE_HPP_ */

