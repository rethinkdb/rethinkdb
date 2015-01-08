// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_manager/table_metadata.hpp"

RDB_IMPL_SERIALIZABLE_2_FOR_CLUSTER(
    table_meta_manager_business_card_t::action_timestamp_t::epoch_t, timestamp, id);
RDB_IMPL_SERIALIZABLE_2_FOR_CLUSTER(
    table_meta_manager_business_card_t::action_timestamp_t, epoch, log_index);
RDB_IMPL_SERIALIZABLE_2_FOR_CLUSTER(
    table_meta_manager_business_card_t, action_mailbox, server_id);
RDB_IMPL_SERIALIZABLE_4_FOR_CLUSTER(
    table_meta_business_card_t, epoch, raft_member_id, raft_business_card, server_id);
