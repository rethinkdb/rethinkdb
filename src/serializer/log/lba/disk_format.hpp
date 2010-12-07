
#ifndef __SERIALIZER_LOG_LBA_DISK_FORMAT__
#define __SERIALIZER_LOG_LBA_DISK_FORMAT__



#include "serializer/serializer.hpp"



// An off64_t with the highest bit saying that a block is deleted.
// Deleted blocks still have an offset, since there's a zero block
// sitting in place for them.  The remaining 63 bits give the offset
// to the block (whether it is deleted or not).
union flagged_off64_t {
    off64_t whole_value;
    struct {
        // The actual offset into the file.
        off64_t value : 63;
    
        // This block id was deleted, and the offset points to a zeroed
        // out buffer.
        int is_delete : 1;
    } parts;

    static inline flagged_off64_t unused() {
        flagged_off64_t ret;
        ret.whole_value = -1;
        return ret;
    }

    static inline flagged_off64_t padding() {
        flagged_off64_t ret;
        ret.whole_value = -1;
        return ret;
    }

    static inline bool is_padding(flagged_off64_t offset) {
        return offset.whole_value == -1;
    }

    static inline flagged_off64_t real(off64_t offset) {
        flagged_off64_t ret;
        ret.parts.value = offset;
        ret.parts.is_delete = 0;
        return ret;
    }

    static inline flagged_off64_t deleteblock(off64_t offset) {
        flagged_off64_t ret;
        ret.parts.value = offset;
        ret.parts.is_delete = 1;
        return ret;
    }
    
    static bool has_value(flagged_off64_t offset) {
        offset.parts.is_delete = 1;
        return offset.whole_value != off64_t(-1);
    }
    
    static inline bool can_be_gced(flagged_off64_t offset) {
        return has_value(offset);
    }
};



struct lba_shard_metablock_t {
    /* Reference to the last lba extent (that's currently being
     * written to). Once the extent is filled, the reference is
     * moved to the lba superblock, and the next block gets a
     * reference to the clean extent. */
    off64_t last_lba_extent_offset;
    int last_lba_extent_entries_count;

    /* Reference to the LBA superblock and its size */
    off64_t lba_superblock_offset;
    int lba_superblock_entries_count;
};

struct lba_metablock_mixin_t {
    
    lba_shard_metablock_t shards[LBA_SHARD_FACTOR];
};




// PADDING_BLOCK_ID and flagged_off64_t::padding() indicate that an entry in the LBA list only exists to fill
// out a DEVICE_BLOCK_SIZE-sized chunk of the extent.

static const ser_block_id_t PADDING_BLOCK_ID = ser_block_id_t::null();

struct lba_entry_t {
    ser_block_id_t block_id;
    uint32_t timestamp;  // reserved
    // An offset into the file, with is_delete set appropriately.
    flagged_off64_t offset;

    static inline lba_entry_t make(ser_block_id_t block_id, flagged_off64_t offset) {
        lba_entry_t entry;
        entry.block_id = block_id;
        entry.timestamp = 0;
        entry.offset = offset;
        return entry;
    }

    static inline bool is_padding(const lba_entry_t* entry) {
        return entry->block_id == PADDING_BLOCK_ID  && flagged_off64_t::is_padding(entry->offset);
    }

    static inline lba_entry_t make_padding_entry() {
        return make(PADDING_BLOCK_ID, flagged_off64_t::padding());
    }
} __attribute((__packed__));

    
    

#define LBA_MAGIC_SIZE 8
static const char lba_magic[LBA_MAGIC_SIZE] = {'l', 'b', 'a', 'm', 'a', 'g', 'i', 'c'};

struct lba_extent_t {
    // Header needs to be padded to a multiple of sizeof(lba_entry_t)
    struct header_t {
        char magic[LBA_MAGIC_SIZE];
        char padding[sizeof(lba_entry_t) - (1 + (sizeof(magic) - 1) % sizeof(lba_entry_t))];
    } header;
    lba_entry_t entries[0];
};



struct lba_superblock_entry_t {
    off64_t offset;
    int lba_entries_count;
};

#define LBA_SUPER_MAGIC_SIZE 8
static const char lba_super_magic[LBA_SUPER_MAGIC_SIZE] = {'l', 'b', 'a', 's', 'u', 'p', 'e', 'r'};

struct lba_superblock_t {
    // Header needs to be padded to a multiple of sizeof(lba_superblock_entry_t)
    char magic[LBA_SUPER_MAGIC_SIZE];
    char padding[sizeof(lba_superblock_entry_t) - (1 + (sizeof(magic) - 1) % sizeof(lba_superblock_entry_t))];

    /* The superblock contains references to all the extents
     * except the last. The reference to the last extent is
     * maintained in the metablock. This is done in order to be
     * able to store the number of entries in the last extent as
     * it's being filled up without rewriting the superblock. */
    lba_superblock_entry_t entries[0];

    static int entry_count_to_file_size(int nentries) {
        return sizeof(lba_superblock_entry_t) * nentries + offsetof(lba_superblock_t, entries[0]);
    }
};



#endif /* __SERIALIZER_LOG_LBA_DISK_FORMAT__ */

