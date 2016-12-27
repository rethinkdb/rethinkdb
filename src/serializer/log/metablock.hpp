#ifndef SERIALIZER_LOG_METABLOCK_HPP_
#define SERIALIZER_LOG_METABLOCK_HPP_

#include "serializer/log/lba/disk_format.hpp"
#include "serializer/log/types.hpp"

// Size of the metablock (in bytes)
#define METABLOCK_SIZE (4 * KILOBYTE)

// How much space to reserve in the metablock to store inline LBA entries.  Make sure
// that it fits into METABLOCK_SIZE, including all other meta data.  (Defines the disk
// format!  Can't change unless you're careful.)
//
// 3.5 KB
#define LBA_INLINE_SIZE (METABLOCK_SIZE - 512)


/* LBA_INLINE_SIZE is 3.5 KB.  You might note, it's a good thing that this value is a
   bit shy of 8 * 512, in terms of LBA usage.  It being 7*512bytes means that when we do
   spill over the inline LBA entries into the actual LBA, we'll use 2 device blocks
   (2*512bytes) in most LBA shards (because LBA_SHARD_FACTOR is 4).  Each shard gets on
   average 28 entries.  If one gets more than 32 entries, we have spillover (into a
   third block).  It's a binomial distribution (right?) with mean 28, stddev 4.5, so I
   suspect we could stand to lower the value of LBA_INLINE_SIZE a tad.  There's some
   trade-off here that minimizes LBA space usage.  (Lowering the spill threshold doesn't
   have to change the disk format, either.) */



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

// 16 bytes.
ATTR_PACKED(struct metablock_filerange_t {
    // A [offset, offset + size) region in the file the fileranges checksum refers to.
    // The value offset=0, size=0 is used as a garbage value -- any range with size zero
    // would work.
    int64_t offset;
    int64_t size;
});

typedef int64_t metablock_version_t;

#define METABLOCK_NUM_CHECKSUMS 8

ATTR_PACKED(struct metablock_fileranges_checksum_t {
    // How many checksum ranges should there be?  Imagine a small write split between
    // two data extents, all four LBA shards, one LBA superblock (which is in a separate
    // LBA superblock extent).  That's 7.  We had 4096-3760 = 336 bytes to work with for
    // this entire struct, with 16 bytes per filerange-checksum.  So, we're stingy, we
    // go with 8 checksums.

    // The checksum computes the checksum of the concatenation of the data in the
    // ranges.  (So, we checksum fileranges[0] ++ fileranges[1] ++ ... ++
    // fileranges[METABLOCK_NUM_CHECKSUMS - 1].)
    serializer_checksum checksum;

    // Offset: 8, size: 128.
    metablock_filerange_t fileranges[METABLOCK_NUM_CHECKSUMS];

    // There's no size field -- we look at filerange values from left to right until we
    // get one with a zero size.

    // Total size: 136.
});


static const char MB_MARKER_MAGIC[8] = {'m', 'e', 't', 'a', 'b', 'l', 'c', 'k'};

// This is stored directly to disk.  Changing it will change the disk format.
ATTR_PACKED(struct crc_metablock_t {
    char magic_marker[sizeof(MB_MARKER_MAGIC)];
    // The version that differs only when the software is upgraded to a newer
    // version.  This field might allow for in-place upgrading of the cluster.
    uint32_t disk_format_version;
    // The CRC checksum of [disk_format_version]+[version]+[metablock].
    uint32_t _crc;
    // The version that increments every time a metablock is written.
    metablock_version_t version;

    // Offset: 24, size: 3736 bytes.
    // The value in the metablock (pointing at LBA superblocks, etc).
    log_serializer_metablock_t metablock;

    // Pre-v2_5 version, this field did not exist.  The space was filled with zeros.
    // Offset: 3760, size: 192.
    metablock_fileranges_checksum_t fileranges_checksum_v2_5;

    // Total size: 3952 bytes.
});


#endif  // SERIALIZER_LOG_METABLOCK_HPP_
