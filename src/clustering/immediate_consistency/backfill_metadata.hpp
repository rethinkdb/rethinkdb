// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILL_METADATA_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILL_METADATA_HPP_

#include "clustering/immediate_consistency/history.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rpc/mailbox/typed.hpp"

/* `backfiller_bcard_t` represents a thing that is willing to serve backfills over the
network. It appears in the directory. */

class backfiller_bcard_t {
public:
};

RDB_DECLARE_SERIALIZABLE(backfiller_business_card_t);
RDB_DECLARE_EQUALITY_COMPARABLE(backfiller_business_card_t);

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILL_METADATA_HPP_ */

