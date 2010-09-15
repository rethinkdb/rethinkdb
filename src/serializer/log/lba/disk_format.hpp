
#ifndef __SERIALIZER_LOG_LBA_DISK_FORMAT__
#define __SERIALIZER_LOG_LBA_DISK_FORMAT__



#define NULL_OFFSET off64_t(-1)

struct lba_metablock_mixin_t {
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



// PADDING_BLOCK_ID and PADDING_OFFSET indicate that an entry in the LBA list only exists to fill
// out a DEVICE_BLOCK_SIZE-sized chunk of the extent.
#define PADDING_BLOCK_ID block_id_t(-1)
#define PADDING_OFFSET off64_t(-1)

#define DELETE_BLOCK off64_t(-1)

struct lba_entry_t {
    block_id_t block_id;
    off64_t offset;   // Is either an offset into the file or DELETE_BLOCK
};

#define LBA_MAGIC_SIZE 8
static const char lba_magic[LBA_MAGIC_SIZE] = {'l', 'b', 'a', 'm', 'a', 'g', 'i', 'c'};

struct lba_extent_t {
    // Header needs to be padded to a multiple of sizeof(lba_entry_t)
    char magic[LBA_MAGIC_SIZE];
    char padding[sizeof(lba_entry_t) - sizeof(magic) % sizeof(lba_entry_t)];
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
    char padding[sizeof(lba_superblock_entry_t) - sizeof(magic) % sizeof(lba_superblock_entry_t)];

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

