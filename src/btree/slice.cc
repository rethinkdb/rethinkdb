// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "errors.hpp"

#include "btree/node.hpp"
#include "btree/operations.hpp"
#include "btree/secondary_operations.hpp"
#include "btree/slice.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "concurrency/cond_var.hpp"
#include "repli_timestamp.hpp"

// Run backfilling at a reduced priority
#define BACKFILL_CACHE_PRIORITY 10

#if SLICE_ALT
void btree_slice_t::create(alt_cache_t *cache,
                           const std::vector<char> &metainfo_key,
                           const std::vector<char> &metainfo_value) {
#else
void btree_slice_t::create(cache_t *cache, const std::vector<char> &metainfo_key, const std::vector<char> &metainfo_value) {
#endif

#if SLICE_ALT
    alt_txn_t txn(cache);
#else
    /* Initialize the btree superblock and the delete queue */
    transaction_t txn(cache,
                      rwi_write,
                      1,
                      repli_timestamp_t::distant_past,
                      order_token_t::ignore,
                      WRITE_DURABILITY_HARD);
#endif

    create(cache, SUPERBLOCK_ID, &txn, metainfo_key, metainfo_value);
}

#if SLICE_ALT
void btree_slice_t::create(alt_cache_t *cache,
                           block_id_t superblock_id,
                           alt_txn_t *txn,
                           const std::vector<char> &metainfo_key,
                           const std::vector<char> &metainfo_value) {
#else
void btree_slice_t::create(cache_t *cache, block_id_t superblock_id, transaction_t *txn,
        const std::vector<char> &metainfo_key, const std::vector<char> &metainfo_value) {
#endif
#if SLICE_ALT
    alt_buf_lock_t superblock(txn, superblock_id, alt_access_t::write);
#else
    buf_lock_t superblock(txn, superblock_id, rwi_write);
#endif

    // Initialize the replication time barrier to 0 so that if we are a slave,
    // we will begin by pulling ALL updates from master.
    superblock.touch_recency(repli_timestamp_t::distant_past);

#if SLICE_ALT
    alt_buf_write_t sb_write(&superblock);
    auto sb = static_cast<btree_superblock_t *>(sb_write.get_data_write());
#else
    btree_superblock_t *sb = static_cast<btree_superblock_t *>(superblock.get_data_write());
#endif
    bzero(sb, cache->get_block_size().value());

    // sb->metainfo_blob has been properly zeroed.
    sb->magic = btree_superblock_t::expected_magic;
    sb->root_block = NULL_BLOCK_ID;
    sb->stat_block = NULL_BLOCK_ID;
    sb->sindex_block = NULL_BLOCK_ID;

    set_superblock_metainfo(txn, &superblock, metainfo_key, metainfo_value);

#if SLICE_ALT
    alt_buf_lock_t sindex_block(&superblock, alt_access_t::write);
#else
    buf_lock_t sindex_block(txn);
#endif
    initialize_secondary_indexes(txn, &sindex_block);
    sb->sindex_block = sindex_block.get_block_id();
}

#if SLICE_ALT
btree_slice_t::btree_slice_t(alt_cache_t *c, perfmon_collection_t *parent,
                             const std::string &identifier,
                             block_id_t _superblock_id)
#else
btree_slice_t::btree_slice_t(cache_t *c, perfmon_collection_t *parent,
                             const std::string &identifier,
                             block_id_t _superblock_id)
#endif
    : stats(parent, identifier),
      cache_(c),
      superblock_id_(_superblock_id),
      root_eviction_priority(INITIAL_ROOT_EVICTION_PRIORITY) {
    cache()->create_cache_account(BACKFILL_CACHE_PRIORITY, &backfill_account);

    pre_begin_txn_checkpoint_.set_tagappend("pre_begin_txn");
}

btree_slice_t::~btree_slice_t() { }
