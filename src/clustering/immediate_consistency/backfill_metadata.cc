// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/backfill_metadata.hpp"

RDB_IMPL_SERIALIZABLE_2_FOR_CLUSTER(
    backfiller_bcard_t, ...);

RDB_IMPL_EQUALITY_COMPARABLE_2(
    backfiller_bcard_t, ...);

