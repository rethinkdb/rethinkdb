// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/metadata.hpp"


RDB_IMPL_SERIALIZABLE_3_FOR_CLUSTER(
        listener_business_card_t,
        intro_mailbox, write_mailbox, server_id);

RDB_IMPL_SERIALIZABLE_4_SINCE_v1_13(
        listener_intro_t, broadcaster_begin_timestamp, upgrade_mailbox,
        downgrade_mailbox, listener_id);

RDB_IMPL_SERIALIZABLE_2_FOR_CLUSTER(
        backfiller_business_card_t, backfill_mailbox, cancel_backfill_mailbox);

RDB_IMPL_EQUALITY_COMPARABLE_2(backfiller_business_card_t,
                               backfill_mailbox,
                               cancel_backfill_mailbox);

RDB_IMPL_SERIALIZABLE_3_FOR_CLUSTER(
        broadcaster_business_card_t, branch_id, region, registrar);
RDB_IMPL_EQUALITY_COMPARABLE_3(broadcaster_business_card_t,
                               branch_id,
                               region,
                               registrar);

RDB_IMPL_SERIALIZABLE_2_FOR_CLUSTER(
        replier_business_card_t, synchronize_mailbox, backfiller_bcard);

RDB_IMPL_EQUALITY_COMPARABLE_2(replier_business_card_t,
                               synchronize_mailbox, backfiller_bcard);
