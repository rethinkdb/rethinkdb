// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/backfill_metadata.hpp"

RDB_IMPL_SERIALIZABLE_5_FOR_CLUSTER(backfiller_bcard_t::intro_2_t,
    common_version, pre_atoms_mailbox, begin_session_mailbox, end_session_mailbox,
    ack_atoms_mailbox);

RDB_IMPL_SERIALIZABLE_6_FOR_CLUSTER(backfiller_bcard_t::intro_1_t,
    initial_version, initial_version_history, intro_mailbox, atoms_mailbox,
    ack_end_session_mailbox, ack_pre_atoms_mailbox);

RDB_IMPL_SERIALIZABLE_2_FOR_CLUSTER(backfiller_bcard_t, region, registrar);
RDB_IMPL_EQUALITY_COMPARABLE_2(backfiller_bcard_t, region, registrar);

RDB_IMPL_SERIALIZABLE_3_FOR_CLUSTER(replica_bcard_t,
    synchronize_mailbox, branch_id, backfiller_bcard);
RDB_IMPL_EQUALITY_COMPARABLE_3(replica_bcard_t,
    synchronize_mailbox, branch_id, backfiller_bcard);
