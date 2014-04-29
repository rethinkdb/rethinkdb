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

void btree_slice_t::init_superblock(buf_lock_t *superblock,
                                    const std::vector<char> &metainfo_key,
                                    const std::vector<char> &metainfo_value) {
    buf_write_t sb_write(superblock);
    auto sb = static_cast<btree_superblock_t *>(sb_write.get_data_write());

    // Properly zero the superblock, zeroing sb->metainfo_blob, in particular.
    memset(sb, 0, superblock->cache()->max_block_size().value());

    sb->magic = btree_superblock_t::expected_magic;
    sb->root_block = NULL_BLOCK_ID;
    sb->stat_block = NULL_BLOCK_ID;
    sb->sindex_block = NULL_BLOCK_ID;

    set_superblock_metainfo(superblock, metainfo_key, metainfo_value);

    buf_lock_t sindex_block(superblock, alt_create_t::create);
    initialize_secondary_indexes(&sindex_block);
    sb->sindex_block = sindex_block.block_id();
}

btree_slice_t::btree_slice_t(cache_t *c, perfmon_collection_t *parent,
                             const std::string &identifier)
    : stats(parent, identifier),
      cache_(c),
      backfill_account_(cache()->create_cache_account(BACKFILL_CACHE_PRIORITY)) { }

btree_slice_t::~btree_slice_t() { }
