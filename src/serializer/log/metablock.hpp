#ifndef SERIALIZER_LOG_METABLOCK_HPP_
#define SERIALIZER_LOG_METABLOCK_HPP_

#include "serializer/log/lba/disk_format.hpp"

// How much space to reserve in the metablock to store inline LBA entries Make sure that
// it fits into METABLOCK_SIZE, including all other meta data.  (Defines the disk
// format!  Can't change unless you're careful.)
#define LBA_INLINE_SIZE                           (METABLOCK_SIZE - 512)

#define LBA_NUM_INLINE_ENTRIES                    (static_cast<int32_t>(LBA_INLINE_SIZE / sizeof(lba_entry_t)))


ATTR_PACKED(struct dbm_metablock_mixin_t {
    int64_t active_extent;
});


ATTR_PACKED(struct lba_metablock_mixin_t {
    lba_shard_metablock_t shards[LBA_SHARD_FACTOR];

    /* Note that inline_lba_entries is not sharded into LBA_SHARD_FACTOR shards.
     * Instead it contains entries from all shards. Sharding is not necessary
     * for the inlined entries, because we do not perform any blocking operations
     * on those (especially no garbage collection).
     * You can assign an entry from the inline LBA to its respective LBA shard
     * by taking the LBA_SHARD_FACTOR modulo of its block id.
     */
    lba_entry_t inline_lba_entries[LBA_NUM_INLINE_ENTRIES];
    int32_t inline_lba_entries_count;
    int32_t padding;
});

ATTR_PACKED(struct extent_manager_metablock_mixin_t {
    int64_t padding;
});




//  Data to be serialized to disk with each block.  Changing this changes the disk
//  format!
ATTR_PACKED(struct log_serializer_metablock_t {
    extent_manager_metablock_mixin_t extent_manager_part;
    lba_metablock_mixin_t lba_index_part;
    dbm_metablock_mixin_t data_block_manager_part;
});


#endif  // SERIALIZER_LOG_METABLOCK_HPP_
