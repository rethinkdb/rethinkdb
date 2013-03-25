// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "errors.hpp"
#include "btree/slice.hpp"
#include "btree/node.hpp"
#include "btree/operations.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "concurrency/cond_var.hpp"

// Run backfilling at a reduced priority
#define BACKFILL_CACHE_PRIORITY 10

void btree_slice_t::create(cache_t *cache, const std::vector<char> &metainfo_key, const std::vector<char> &metainfo_value) {

    // SAMRSI: Should we wait on this signal?  Pass it in as a parameter?
    cond_t disk_ack_signal;

    /* Initialize the btree superblock and the delete queue */
    transaction_t txn(cache, rwi_write, 1, repli_timestamp_t::distant_past, order_token_t::ignore, &disk_ack_signal);

    buf_lock_t superblock(&txn, SUPERBLOCK_ID, rwi_write);

    // Initialize replication time barrier to 0 so that if we are a slave, we will begin by pulling
    // ALL updates from master.
    superblock.touch_recency(repli_timestamp_t::distant_past);

    btree_superblock_t *sb = reinterpret_cast<btree_superblock_t *>(superblock.get_data_write());
    bzero(sb, cache->get_block_size().value());

    // sb->metainfo_blob has been properly zeroed.

    sb->magic = btree_superblock_t::expected_magic;
    sb->root_block = NULL_BLOCK_ID;
    sb->stat_block = NULL_BLOCK_ID;

    set_superblock_metainfo(&txn, &superblock, metainfo_key, metainfo_value);
}

btree_slice_t::btree_slice_t(cache_t *c, perfmon_collection_t *parent)
    : stats(parent),
      cache_(c),
      root_eviction_priority(INITIAL_ROOT_EVICTION_PRIORITY) {
    cache()->create_cache_account(BACKFILL_CACHE_PRIORITY, &backfill_account);

    pre_begin_txn_checkpoint_.set_tagappend("pre_begin_txn");
}

btree_slice_t::~btree_slice_t() { }
