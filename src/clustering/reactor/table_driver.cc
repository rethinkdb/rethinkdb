// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/reactor/table_driver.hpp"

table_driver_business_card_t table_driver_t::get_table_driver_business_card() {
    table_driver_business_card_t b;
    b.force = force_mailbox.get_address();
    b.recruit = recruit_mailbox.get_address();
    return b;
}

void table_driver_t::on_force(
        UNUSED signal_t *interruptor,
        const namespace_id_t &table_id,
        const table_raft_instance_id_t &instance_id,
        const table_raft_state_t &new_state,
        const raft_config_t &new_config) {
    tables.erase(table_id);
    tables[table_id] = make_scoped<table_t>(
        instance_id,
        raft_persistent_state_t<table_raft_state_t>::make_initial(
            new_state, new_config));
}

void table_driver_t::on_recruit(
        UNUSED signal_t *interruptor,
        const namespace_id_t &table_id,
        const table_raft_instance_id_t &instance_id) {
    if (tables.count(table_id) == 0) {
        tables[table_id] = make_scoped<table_t>(
            instance_id,
            raft_persistent_state_t<table_raft_state_t>::make_join());
    }
}
