// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_manager/table_raft_state.hpp"

void table_raft_state_t::apply_change(const table_raft_state_t::change_t &change) {
    class visitor_t : public boost::static_visitor<void> {
    public:
        void operator()(const change_t::set_table_config_t &set_config_change) {
            state->config = set_config_change.new_config;
        }
        table_raft_state_t *state;
    } visitor;
    visitor.state = this;
    boost::apply_visitor(visitor, change.v);
}

/* RSI(raft): This should be `SINCE_v1_N`, where `N` is the version number at which Raft
is released */
RDB_IMPL_SERIALIZABLE_1_SINCE_v1_16(
    table_raft_state_t::change_t::set_table_config_t, new_config);
RDB_IMPL_SERIALIZABLE_1_SINCE_v1_16(table_raft_state_t::change_t, v);
RDB_IMPL_SERIALIZABLE_2_SINCE_v1_16(table_raft_state_t, config, member_ids);

