// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_MANAGER_TABLE_RAFT_STATE_HPP_
#define CLUSTERING_TABLE_MANAGER_TABLE_RAFT_STATE_HPP_

#include "clustering/administration/tables/table_metadata.hpp"
#include "clustering/generic/raft_core.hpp"

typedef uuid_u table_msg_id_t;

/* We provide the following consistency guarantee: Suppose the client sends a write W
which affects some region R of the table, and the cluster acknowledges that W has been
performed. Then every future read will see W, even if servers die or auto-failover
happens, unless the administrator issues a manual override. (This assumes that the reads
and writes are run with the highest level of consistency guarantees.)

To provide this guarantee, we maintain a bunch of invariants. Let V be the `version_t`
of the shard after W has been performed. Let `l2f` be the `table_l2f_msg_t` in the Raft
state for some region R' which overlaps R. Then the following are always true unless the
user issues a manual override:
1. A majority of `l2f.voters` have versions on disk which descend from V (or are V) in
    in the intersection of R and R'.
2. If `l2f.primary` is present, then `l2f.primary->server` has a version on disk which
    descends from V (or is V) in the intersection of R and R'.
3. The latest version on `l2f.branch` is descended from V (or is V) in the intersection
    of R and R'.
*/

class table_l2f_msg_t {
public:
    class primary_t {
    public:
        server_id_t server;
        bool warm_shutdown;
    };
    void sanity_check() const {
        guarantee(!static_cast<bool>(primary) || replicas.count(primary->server) == 1);
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

class table_f2l_msg_t {
public:
    enum class state_t {
        primary_running,
        primary_switching_voters,
        primary_did_warm_shutdown,
        secondary_need_primary,
        secondary_backfilling,
        secondary_streaming,
        nothing
    } state;

    /* non-empty for `primary_did_warm_shutdown` and `secondary_need_primary` */
    boost::optional<region_map_t<version_t> > version;
};

class table_raft_state_t {
public:
    class change_t {
    public:
        class set_table_config_t {
        public:
            table_config_and_shards_t new_config;
        };

        class new_leader_msg_t {
        public:
            key_range_t range;
            table_raft_leader_msg_t msg;
        };

        change_t() { }
        template<class T>
        change_t(T &&t) : v(t) { }

        boost::variant<set_table_config_t, new_instructions_t> v;
    };

    void apply_change(const change_t &c);
    bool operator==(const table_raft_state_t &other) const {
        return config == other.config && member_ids == other.member_ids;
    }

    table_config_and_shards_t config;
    std::map<table_msg_id_t, std::pair<key_range_t, table_l2f_msg_t> > leader_msgs;
    std::map<server_id_t, raft_member_id_t> member_ids;
};

RDB_DECLARE_SERIALIZABLE(table_raft_state_t::change_t::set_table_config_t);
RDB_DECLARE_SERIALIZABLE(table_raft_state_t::change_t);
RDB_DECLARE_SERIALIZABLE(table_raft_state_t);

#endif /* CLUSTERING_TABLE_MANAGER_TABLE_RAFT_STATE_HPP_ */

