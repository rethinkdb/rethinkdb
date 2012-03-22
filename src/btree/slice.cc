#include "errors.hpp"
#include <boost/archive/binary_oarchive.hpp>
#include "containers/vector_stream.hpp"

#include "btree/backfill.hpp"
#include "btree/erase_range.hpp"
#include "btree/slice.hpp"
#include "btree/node.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "concurrency/cond_var.hpp"
#include "btree/get.hpp"
#include "btree/rget.hpp"
#include "btree/set.hpp"
#include "btree/incr_decr.hpp"
#include "btree/append_prepend.hpp"
#include "btree/delete.hpp"
#include "btree/get_cas.hpp"

namespace arc = boost::archive;

void btree_slice_t::create(cache_t *cache) {

    /* Initialize the btree superblock and the delete queue */
    transaction_t txn(cache, rwi_write, 1, repli_timestamp_t::distant_past);

    buf_lock_t superblock(&txn, SUPERBLOCK_ID, rwi_write);

    // Initialize replication time barrier to 0 so that if we are a slave, we will begin by pulling
    // ALL updates from master.
    superblock.touch_recency(repli_timestamp_t::distant_past);

    btree_superblock_t *sb = reinterpret_cast<btree_superblock_t *>(superblock.get_data_major_write());
    bzero(sb, cache->get_block_size().value());

    // sb->metainfo_blob has been properly zeroed.

    sb->magic = btree_superblock_t::expected_magic;
    sb->root_block = NULL_BLOCK_ID;
}

btree_slice_t::btree_slice_t(cache_t *c)
    : cache_(c), backfill_account(cache()->create_account(BACKFILL_CACHE_PRIORITY)),
    root_eviction_priority(INITIAL_ROOT_EVICTION_PRIORITY) {
    order_checkpoint_.set_tagappend("slice");
    post_begin_transaction_checkpoint_.set_tagappend("post");
}

btree_slice_t::~btree_slice_t() {
    // Cache's destructor handles flushing and stuff
}
