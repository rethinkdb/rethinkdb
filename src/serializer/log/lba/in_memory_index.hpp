
#ifndef __SERIALIZER_LOG_LBA_IN_MEMORY_INDEX__
#define __SERIALIZER_LOG_LBA_IN_MEMORY_INDEX__


#include "containers/segmented_vector.hpp"
#include "config/args.hpp"
#include "serializer/serializer.hpp"
#include "serializer/log/lba/disk_format.hpp"


struct in_memory_index_t
{
    // blocks.get_size() == timestamps.get_size().  We use parallel
    // arrays to avoid wasting memory from alignment.
    segmented_vector_t<flagged_off64_t, MAX_BLOCK_ID> blocks;
    segmented_vector_t<repl_timestamp, MAX_BLOCK_ID> timestamps;

public:
    in_memory_index_t();

    // TODO: Rename this function.  It's one greater than the max
    // block id.
    ser_block_id_t max_block_id();

    struct info_t {
        flagged_off64_t offset;
        repl_timestamp recency;
    };

    info_t get_block_info(ser_block_id_t id);
    void set_block_info(ser_block_id_t id, repl_timestamp recency,
                        flagged_off64_t offset);

    void print();
};

#endif /* __SERIALIZER_LOG_LBA_IN_MEMORY_INDEX__ */

