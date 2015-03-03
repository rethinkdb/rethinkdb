// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_manager/table_metadata.hpp"

/* RSI(raft): These should be `SINCE_v2_N`, where `N` is the version when Raft is
released. */
RDB_IMPL_SERIALIZABLE_2_SINCE_v1_16(
    table_meta_manager_bcard_t::timestamp_t::epoch_t, timestamp, id);
RDB_IMPL_SERIALIZABLE_2_SINCE_v1_16(
    table_meta_manager_bcard_t::timestamp_t, epoch, log_index);
RDB_IMPL_SERIALIZABLE_3_FOR_CLUSTER(
    table_meta_manager_bcard_t,
    action_mailbox, get_config_mailbox, server_id);
RDB_IMPL_SERIALIZABLE_3_FOR_CLUSTER(
    table_meta_bcard_t::leader_bcard_t,
    uuid, set_config_mailbox, contract_ack_minidir_bcard);
RDB_IMPL_SERIALIZABLE_9_FOR_CLUSTER(
    table_meta_bcard_t,
    leader, timestamp, database, name, primary_key, raft_member_id, raft_business_card,
    execution_bcard_minidir_bcard, server_id);

