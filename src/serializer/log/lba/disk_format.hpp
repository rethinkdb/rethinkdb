// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef SERIALIZER_LOG_LBA_DISK_FORMAT_HPP_
#define SERIALIZER_LOG_LBA_DISK_FORMAT_HPP_

#include <limits.h>

#include "serializer/serializer.hpp"
#include "config/args.hpp"



// Contains an off64_t, or a "padding" value, or a "unused" value.  I
// don't think we really need separate padding and unused values.

// There used to be some flag bits on the value but now there's only
// one special sentinel value for deletion entries or padding entries.
union flagged_off64_t {
    off64_t the_value_;

    bool has_value() const {
        return the_value_ >= 0;
    }
    off64_t get_value() const {
        rassert(has_value());
        return the_value_;
    }

    static flagged_off64_t make(off64_t off) {
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



struct lba_shard_metablock_t {
    /* Reference to the last lba extent (that's currently being
     * written to). Once the extent is filled, the reference is
     * moved to the lba superblock, and the next block gets a
     * reference to the clean extent. */
    off64_t last_lba_extent_offset;
    int32_t last_lba_extent_entries_count;
    int32_t padding1;

    /* Reference to the LBA superblock and its size */
    off64_t lba_superblock_offset;
    int32_t lba_superblock_entries_count;
    int32_t padding2;
};

struct lba_metablock_mixin_t {
    lba_shard_metablock_t shards[LBA_SHARD_FACTOR];
};




// PADDING_BLOCK_ID and flagged_off64_t::padding() indicate that an entry in the LBA list only exists to fill
// out a DEVICE_BLOCK_SIZE-sized chunk of the extent.

static const block_id_t PADDING_BLOCK_ID = NULL_BLOCK_ID;

struct lba_entry_t {
    block_id_t block_id;

    // TODO: Remove the need for these fields.  Remove the requirement
    // that lba_entry_t be a divisor of DEVICE_BLOCK_SIZE.
    uint32_t zero1;
    uint64_t zero2;

    repli_timestamp_t recency;
    // An offset into the file, with is_delete set appropriately.
    flagged_off64_t offset;

    static inline lba_entry_t make(block_id_t block_id, repli_timestamp_t recency, flagged_off64_t offset) {
        lba_entry_t entry;
        entry.block_id = block_id;
        entry.zero1 = 0;
        entry.zero2 = 0;
        entry.recency = recency;
        entry.offset = offset;
        return entry;
    }

    static inline bool is_padding(const lba_entry_t* entry) {
        return entry->block_id == PADDING_BLOCK_ID  && entry->offset.is_padding();
    }

    static inline lba_entry_t make_padding_entry() {
        return make(PADDING_BLOCK_ID, repli_timestamp_t::invalid, flagged_off64_t::padding());
    }
} __attribute__((__packed__));




#define LBA_MAGIC_SIZE 8
static const char lba_magic[LBA_MAGIC_SIZE] = {'l', 'b', 'a', 'm', 'a', 'g', 'i', 'c'};

struct lba_extent_t {
    // Header needs to be padded to a multiple of sizeof(lba_entry_t)
    struct header_t {
        char magic[LBA_MAGIC_SIZE];
        char padding[sizeof(lba_entry_t) - (1 + (LBA_MAGIC_SIZE - 1) % sizeof(lba_entry_t))];
    } header;
    lba_entry_t entries[0];
};



struct lba_superblock_entry_t {
    off64_t offset;
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

