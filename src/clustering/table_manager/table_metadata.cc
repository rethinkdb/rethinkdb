// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_manager/table_metadata.hpp"

RDB_IMPL_SERIALIZABLE_2_FOR_CLUSTER(
    table_meta_manager_bcard_t::timestamp_t::epoch_t, timestamp, id);
RDB_IMPL_SERIALIZABLE_2_FOR_CLUSTER(
    table_meta_manager_bcard_t::timestamp_t, epoch, log_index);
RDB_IMPL_SERIALIZABLE_2_FOR_CLUSTER(
    table_meta_manager_bcard_t, action_mailbox, server_id);
RDB_IMPL_SERIALIZABLE_7_FOR_CLUSTER(
    table_meta_bcard_t, timestamp, database, name, raft_member_id, raft_business_card,
    is_leader, server_id);
