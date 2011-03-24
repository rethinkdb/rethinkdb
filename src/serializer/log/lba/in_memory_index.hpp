
#ifndef __SERIALIZER_LOG_LBA_IN_MEMORY_INDEX__
#define __SERIALIZER_LOG_LBA_IN_MEMORY_INDEX__


#include "containers/segmented_vector.hpp"
#include "config/args.hpp"
#include "serializer/serializer.hpp"
#include "serializer/log/lba/disk_format.hpp"


class in_memory_index_t
{
    // blocks.get_size() == timestamps.get_size().  We use parallel
    // arrays to avoid wasting memory from alignment.
    segmented_vector_t<flagged_off64_t, MAX_BLOCK_ID> blocks;
    segmented_vector_t<repli_timestamp, MAX_BLOCK_ID> timestamps;

    // TODO: Make the max size a constant in args.hpp or something?
    // (on the other hand: 128 TB should be enough for everyone...)
    // TODO! Test actual memory usage of this... (should be 16 MB initially and about 1 GB per TB database size with 4 KB blocks)
    // We store only offsets which are block-size aligned, i.e. block_ids[5] contains the block_id for the offset 5 * block_size
    segmented_vector_t<ser_block_id_t, TERABYTE / DEVICE_BLOCK_SIZE * 128ul> block_ids;

    size_t block_size;

public:
    in_memory_index_t(size_t block_size);

    // end_block_id is one greater than the max block id.
    ser_block_id_t end_block_id();

    struct info_t {
        flagged_off64_t offset;
        repli_timestamp recency;
    };

    info_t get_block_info(ser_block_id_t id);
    void set_block_info(ser_block_id_t id, repli_timestamp recency,
                        flagged_off64_t offset);

    bool is_offset_indexed(off64_t offset);
    ser_block_id_t get_block_id(off64_t offset);

    // Rebuild the reverse mapping offset -> block id. Can become necessary on startup, when the LBA contains outdaited entries with offset collisions to recent ones
    void rebuild_reverse_index(); 

#ifndef NDEBUG
    void print();
#endif
};

#endif /* __SERIALIZER_LOG_LBA_IN_MEMORY_INDEX__ */

