// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_MANAGER_TABLE_RAFT_STATE_HPP_
#define CLUSTERING_TABLE_MANAGER_TABLE_RAFT_STATE_HPP_

#include "clustering/administration/tables/table_metadata.hpp"
#include "clustering/generic/raft_core.hpp"

class table_raft_state_t {
public:
    class change_t {
    public:
        class set_table_config_t {
        public:
            table_config_and_shards_t new_config_and_shards;
        };

        change_t() { }
        template<class T>
        change_t(T &&t) : v(t) { }

        boost::variant<set_table_config_t> v;
    };

    void apply_change(const change_t &c);
    bool operator==(const table_raft_state_t &other) const {
        return config == other.config && member_ids == other.member_ids;
    }

    table_config_and_shards_t config_and_shards;
    std::map<server_id_t, raft_member_id_t> member_ids;
};

RDB_DECLARE_SERIALIZABLE(table_raft_state_t::change_t::set_table_config_t);
RDB_DECLARE_SERIALIZABLE(table_raft_state_t::change_t);
RDB_DECLARE_SERIALIZABLE(table_raft_state_t);

#endif /* CLUSTERING_TABLE_MANAGER_TABLE_RAFT_STATE_HPP_ */

