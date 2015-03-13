// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/backfill_metadata.hpp"

RDB_IMPL_SERIALIZABLE_2_FOR_CLUSTER(
    backfiller_business_card_t, backfill_mailbox, cancel_backfill_mailbox);

RDB_IMPL_EQUALITY_COMPARABLE_2(
    backfiller_business_card_t, backfill_mailbox, cancel_backfill_mailbox);

RDB_IMPL_SERIALIZABLE_2_FOR_CLUSTER(
    replier_business_card_t, synchronize_mailbox, backfiller_bcard);

RDB_IMPL_EQUALITY_COMPARABLE_2(
    replier_business_card_t, synchronize_mailbox, backfiller_bcard);
