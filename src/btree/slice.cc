// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "btree/node.hpp"
#include "btree/operations.hpp"
#include "btree/slice.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "concurrency/cond_var.hpp"
#include "errors.hpp"

// Run backfilling at a reduced priority
#define BACKFILL_CACHE_PRIORITY 10

void btree_slice_t::create(cache_t *cache) {
    transaction_t txn(cache, rwi_write, 1, repli_timestamp_t::distant_past, order_token_t::ignore);
    create(cache, SUPERBLOCK_ID, &txn);
}

void btree_slice_t::create(cache_t *cache, block_id_t superblock_id, transaction_t *txn) {
    buf_lock_t superblock(txn, superblock_id, rwi_write);

    // Initialize replication time barrier to 0 so that if we are a slave, we will begin by pulling
    // ALL updates from master.
    superblock.touch_recency(repli_timestamp_t::distant_past);

    btree_superblock_t *sb = reinterpret_cast<btree_superblock_t *>(superblock.get_data_major_write());
    bzero(sb, cache->get_block_size().value());

    // sb->metainfo_blob has been properly zeroed.

    sb->magic = btree_superblock_t::expected_magic;
    sb->root_block = NULL_BLOCK_ID;
    sb->stat_block = NULL_BLOCK_ID;

    buf_lock_t sindex_block(txn);
    initialize_secondary_indexes(txn, &sindex_block);
    sb->sindex_block = sindex_block.get_block_id();
}

btree_slice_t::btree_slice_t(cache_t *c, perfmon_collection_t *parent, const std::string &identifier, block_id_t _superblock_id)
    : stats(parent, identifier),
      cache_(c),
      superblock_id_(_superblock_id),
      root_eviction_priority(INITIAL_ROOT_EVICTION_PRIORITY) {
    cache()->create_cache_account(BACKFILL_CACHE_PRIORITY, &backfill_account);

    pre_begin_txn_checkpoint_.set_tagappend("pre_begin_txn");
}

btree_slice_t::~btree_slice_t() { }
