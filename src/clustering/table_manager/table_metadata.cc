// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_manager/table_metadata.hpp"

RDB_IMPL_SERIALIZABLE_2_SINCE_v2_1(
    multi_table_manager_bcard_t::timestamp_t::epoch_t, timestamp, id);
RDB_IMPL_SERIALIZABLE_2_SINCE_v2_1(
    multi_table_manager_bcard_t::timestamp_t, epoch, log_index);
RDB_IMPL_SERIALIZABLE_3_FOR_CLUSTER(
    multi_table_manager_bcard_t,
    action_mailbox, get_status_mailbox, server_id);
RDB_IMPL_SERIALIZABLE_3_FOR_CLUSTER(
    table_manager_bcard_t::leader_bcard_t,
    uuid, set_config_mailbox, contract_ack_minidir_bcard);
RDB_IMPL_SERIALIZABLE_6_FOR_CLUSTER(
    table_manager_bcard_t,
    leader, timestamp, raft_member_id, raft_business_card,
    execution_bcard_minidir_bcard, server_id);
RDB_IMPL_SERIALIZABLE_6_FOR_CLUSTER(table_status_request_t,
    want_config, want_sindexes, want_raft_state, want_contract_acks, want_shard_status,
    want_all_replicas_ready);
RDB_IMPL_SERIALIZABLE_7_FOR_CLUSTER(table_status_response_t,
    config, sindexes, raft_state, raft_state_timestamp, contract_acks, shard_status,
    all_replicas_ready);

RDB_IMPL_SERIALIZABLE_2_SINCE_v2_1(table_active_persistent_state_t,
    epoch, raft_member_id);
RDB_IMPL_SERIALIZABLE_2_SINCE_v2_1(table_inactive_persistent_state_t,
    second_hand_config, timestamp);

