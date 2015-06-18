// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/backfill_metadata.hpp"

backfill_config_t::backfill_config_t() :
    item_queue_mem_size(4 * MEGABYTE),
    item_chunk_mem_size(100 * KILOBYTE),
    pre_item_queue_mem_size(4 * MEGABYTE),
    pre_item_chunk_mem_size(100 * KILOBYTE),
    write_queue_count(1000),
    write_queue_trickle_fraction(0.5)
    { }

RDB_IMPL_SERIALIZABLE_6_FOR_CLUSTER(backfill_config_t,
    item_queue_mem_size, item_chunk_mem_size, pre_item_queue_mem_size,
    pre_item_chunk_mem_size, write_queue_count, write_queue_trickle_fraction);

RDB_IMPL_SERIALIZABLE_6_FOR_CLUSTER(backfiller_bcard_t::intro_2_t,
    common_version, final_version_history, pre_items_mailbox, begin_session_mailbox,
    end_session_mailbox, ack_items_mailbox);

RDB_IMPL_SERIALIZABLE_7_FOR_CLUSTER(backfiller_bcard_t::intro_1_t,
    config, initial_version, initial_version_history, intro_mailbox, items_mailbox,
    ack_end_session_mailbox, ack_pre_items_mailbox);

RDB_IMPL_SERIALIZABLE_2_FOR_CLUSTER(backfiller_bcard_t, region, registrar);
RDB_IMPL_EQUALITY_COMPARABLE_2(backfiller_bcard_t, region, registrar);

RDB_IMPL_SERIALIZABLE_3_FOR_CLUSTER(replica_bcard_t,
    synchronize_mailbox, branch_id, backfiller_bcard);
RDB_IMPL_EQUALITY_COMPARABLE_3(replica_bcard_t,
    synchronize_mailbox, branch_id, backfiller_bcard);
