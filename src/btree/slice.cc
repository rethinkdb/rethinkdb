// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "errors.hpp"

#include "btree/node.hpp"
#include "btree/operations.hpp"
#include "btree/secondary_operations.hpp"
#include "btree/slice.hpp"
#include "concurrency/cond_var.hpp"
#include "repli_timestamp.hpp"

// Run backfilling at a reduced priority
#define BACKFILL_CACHE_PRIORITY 10

void btree_slice_t::create(cache_t *cache,
                           const std::vector<char> &metainfo_key,
                           const std::vector<char> &metainfo_value) {

    txn_t txn(cache, write_durability_t::HARD, repli_timestamp_t::distant_past, 1);

    create(SUPERBLOCK_ID, alt_buf_parent_t(&txn), metainfo_key, metainfo_value);
}

void btree_slice_t::create(block_id_t superblock_id,
                           alt_buf_parent_t parent,
                           const std::vector<char> &metainfo_key,
                           const std::vector<char> &metainfo_value) {
    // The superblock was already created.
    // RSI: Make this be the thing that creates the block.
    buf_lock_t superblock(parent, superblock_id, alt_access_t::write);

    buf_write_t sb_write(&superblock);
    auto sb = static_cast<btree_superblock_t *>(sb_write.get_data_write());
    bzero(sb, parent.cache()->get_block_size().value());

    // sb->metainfo_blob has been properly zeroed.
    sb->magic = btree_superblock_t::expected_magic;
    sb->root_block = NULL_BLOCK_ID;
    sb->stat_block = NULL_BLOCK_ID;
    sb->sindex_block = NULL_BLOCK_ID;

    set_superblock_metainfo(&superblock, metainfo_key, metainfo_value);

    buf_lock_t sindex_block(&superblock, alt_create_t::create);
    initialize_secondary_indexes(&sindex_block);
    sb->sindex_block = sindex_block.get_block_id();
}

btree_slice_t::btree_slice_t(cache_t *c, perfmon_collection_t *parent,
                             const std::string &identifier,
                             block_id_t _superblock_id)
    : stats(parent, identifier),
      cache_(c),
      superblock_id_(_superblock_id) {
    cache()->create_cache_account(BACKFILL_CACHE_PRIORITY, &backfill_account);
}

btree_slice_t::~btree_slice_t() { }
