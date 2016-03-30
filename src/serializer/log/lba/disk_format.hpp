// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef SERIALIZER_LOG_LBA_DISK_FORMAT_HPP_
#define SERIALIZER_LOG_LBA_DISK_FORMAT_HPP_

#include <limits.h>

#include "serializer/serializer.hpp"
#include "config/args.hpp"


#define LBA_NUM_INLINE_ENTRIES                    (static_cast<int32_t>(LBA_INLINE_SIZE / sizeof(lba_entry_t)))


// Contains an int64_t, or a "padding" value, or a "unused" value.  I
// don't think we really need separate padding and unused values.

// There used to be some flag bits on the value but now there's only
// one special sentinel value for deletion entries or padding entries.
union flagged_off64_t {
    int64_t the_value_;

    bool has_value() const {
        return the_value_ >= 0;
    }
    int64_t get_value() const {
        rassert(has_value());
        return the_value_;
    }

    static flagged_off64_t make(int64_t off) {
        rassert(off >= 0);
        flagged_off64_t ret;
        ret.the_value_ = off;
        return ret;
    }

    static flagged_off64_t padding() {
        flagged_off64_t ret;
        ret.the_value_ = -1;
        return ret;
    }
    bool is_padding() const {
        return the_value_ == -1;
    }

    static flagged_off64_t unused() {
        flagged_off64_t ret;
        ret.the_value_ = -1;
        return ret;
    }
};

inline bool operator==(flagged_off64_t x, flagged_off64_t y) {
    return x.the_value_ == y.the_value_;
}

// PADDING_BLOCK_ID and flagged_off64_t::padding() indicate that an entry in the LBA list only exists to fill
// out a DEVICE_BLOCK_SIZE-sized chunk of the extent.

static const block_id_t PADDING_BLOCK_ID = NULL_BLOCK_ID;

ATTR_PACKED(struct lba_entry_t {
    // Right now there's code that assumes sizeof(lba_entry_t) is a power of two.
    // (It probably assumes that sizeof(lba_entry_t) evenly divides
    // DEVICE_BLOCK_SIZE).

    // Put the zero-padding at the beginning of the LBA entry.  We could use this, or
    // the first 16 bits, perhaps, as a version flag.
    uint32_t zero_reserved;

    // This could be a uint16_t if you wanted it to be, as long as block sizes are
    // all less than or equal to 4K (which is less than 64K).
    uint32_t ser_block_size;

    block_id_t block_id;

    repli_timestamp_t recency;

    // An offset into the file, with is_delete set appropriately.
    flagged_off64_t offset;

    static lba_entry_t make(block_id_t block_id, repli_timestamp_t recency,
                            flagged_off64_t offset, uint32_t ser_block_size) {
        guarantee(ser_block_size != 0 || !offset.has_value());
        lba_entry_t entry;
        entry.zero_reserved = 0;
        entry.ser_block_size = ser_block_size;
        entry.block_id = block_id;
        entry.recency = recency;
        entry.offset = offset;
        return entry;
    }

    static bool is_padding(const lba_entry_t *entry) {
        return entry->block_id == PADDING_BLOCK_ID  && entry->offset.is_padding();
    }

    static lba_entry_t make_padding_entry() {
        return make(PADDING_BLOCK_ID, repli_timestamp_t::invalid, flagged_off64_t::padding(), 0);
    }
});


ATTR_PACKED(struct lba_shard_metablock_t {
    /* Reference to the last lba extent (that's currently being
     * written to). Once the extent is filled, the reference is
     * moved to the lba superblock, and the next block gets a
     * reference to the clean extent. */
    int64_t last_lba_extent_offset;
    int32_t last_lba_extent_entries_count;
    int32_t padding1;

    /* Reference to the LBA superblock and its size */
    int64_t lba_superblock_offset;
    int32_t lba_superblock_entries_count;
    int32_t padding2;
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


#define LBA_MAGIC_SIZE 8
static const char lba_magic[LBA_MAGIC_SIZE] = {'l', 'b', 'a', 'm', 'a', 'g', 'i', 'c'};

ATTR_PACKED(struct lba_extent_t {
    // Header needs to be padded to a multiple of sizeof(lba_entry_t)
    ATTR_PACKED(struct header_t {
        char magic[LBA_MAGIC_SIZE];
        char padding[sizeof(lba_entry_t) - (1 + (LBA_MAGIC_SIZE - 1) % sizeof(lba_entry_t))];
    });
    header_t header;
    lba_entry_t entries[0];
});



struct lba_superblock_entry_t {
    int64_t offset;
    int64_t lba_entries_count;
};

#define LBA_SUPER_MAGIC_SIZE 8
static const char lba_super_magic[LBA_SUPER_MAGIC_SIZE] = {'l', 'b', 'a', 's', 'u', 'p', 'e', 'r'};

struct lba_superblock_t {
    // Header needs to be padded to a multiple of sizeof(lba_superblock_entry_t)
    char magic[LBA_SUPER_MAGIC_SIZE];
    char padding[sizeof(lba_superblock_entry_t) - (1 + (LBA_SUPER_MAGIC_SIZE - 1) % sizeof(lba_superblock_entry_t))];

    /* The superblock contains references to all the extents
     * except the last. The reference to the last extent is
     * maintained in the metablock. This is done in order to be
     * able to store the number of entries in the last extent as
     * it's being filled up without rewriting the superblock. */
    lba_superblock_entry_t entries[0];

    static int entry_count_to_file_size(int nentries) {
        int ret;
        guarantee(safe_entry_count_to_file_size(nentries, &ret));
        return ret;
    }

    // Returns false if this operation would overflow or if nentries
    // is negative.  nentries could still be an obviously invalid
    // value (like if file_size_out was greater than an extent size).
    static bool safe_entry_count_to_file_size(int nentries, int *file_size_out) {
        if (nentries < 0 || nentries > static_cast<int>((INT_MAX - offsetof(lba_superblock_t, entries[0])) / sizeof(lba_superblock_entry_t))) {
            *file_size_out = 0;
            return false;
        } else {
            *file_size_out = sizeof(lba_superblock_entry_t) * nentries + offsetof(lba_superblock_t, entries[0]);
            return true;
        }
    }
};



#endif  // SERIALIZER_LOG_LBA_DISK_FORMAT_HPP_

