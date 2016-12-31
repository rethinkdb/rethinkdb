#ifndef SERIALIZER_LOG_METABLOCK_HPP_
#define SERIALIZER_LOG_METABLOCK_HPP_

#include "serializer/log/lba/disk_format.hpp"

// Size of the metablock (in bytes)
#define METABLOCK_SIZE (4 * KILOBYTE)

// How much space to reserve in the metablock to store inline LBA entries.  Make sure
// that it fits into METABLOCK_SIZE, including all other meta data.  (Defines the disk
// format!  Can't change unless you're careful.)

// LBA_INLINE_SIZE is 3.5 KB.  You might note, it's a good thing that this value is a
// bit shy of 8 * 512, in terms of LBA usage.  It being 7*512bytes means that when we do
// spill over the inline LBA entries into the actual LBA, we'll use 2 device blocks
// (2*512bytes) in most LBA shards (because LBA_SHARD_FACTOR is 4).  Each shard gets on
// average 28 entries.  If one gets more than 32 entries, we have spillover (into a
// third block).  It's a binomial distribution (right?) with mean 28, stddev 4.5, so I
// suspect we could stand to lower the value of LBA_INLINE_SIZE a tad.  There's some
// trade-off here that minimizes LBA space usage.  (Lowering the spill threshold doesn't
// have to change the disk format, either.)

// 3.5 KB
#define LBA_INLINE_SIZE (METABLOCK_SIZE - 512)

// sizeof(lba_entry_t) is 32, meaning this is 7*2^9/2^5 = 7*2^4 = 112
// Value is 112.
#define LBA_NUM_INLINE_ENTRIES (static_cast<int32_t>(LBA_INLINE_SIZE / sizeof(lba_entry_t)))


ATTR_PACKED(struct dbm_metablock_mixin_t {
    int64_t active_extent;
});

ATTR_PACKED(struct lba_metablock_mixin_t {
    // LBA_SHARD_FACTOR is 4, sizeof(lba_shard_metablock_t) is 32.
    // Total size of shards: 128 bytes.
    lba_shard_metablock_t shards[LBA_SHARD_FACTOR];

    /* Note that inline_lba_entries is not sharded into LBA_SHARD_FACTOR shards.
     * Instead it contains entries from all shards. Sharding is not necessary
     * for the inlined entries, because we do not perform any blocking operations
     * on those (especially no garbage collection).
     * You can assign an entry from the inline LBA to its respective LBA shard
     * by taking the LBA_SHARD_FACTOR modulo of its block id.
     */
    // 3584 bytes.  (3584 = 32 * 112 = 7 * 512.)
    lba_entry_t inline_lba_entries[LBA_NUM_INLINE_ENTRIES];
    int32_t inline_lba_entries_count;
    int32_t padding;
    // 3720 bytes total
});

ATTR_PACKED(struct extent_manager_metablock_mixin_t {
    int64_t padding;
});




//  Data to be serialized to disk with each block.  Changing this changes the disk
//  format!
ATTR_PACKED(struct log_serializer_metablock_t {
    // 8 bytes.  (A "padding" field set to 0.)
    extent_manager_metablock_mixin_t extent_manager_part;
    // offset: 8, size: 3720.
    lba_metablock_mixin_t lba_index_part;
    // offset: 3728, size: 8.
    dbm_metablock_mixin_t data_block_manager_part;
    // Size: 3736 bytes.
});


#endif  // SERIALIZER_LOG_METABLOCK_HPP_
