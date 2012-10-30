// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef SERIALIZER_LOG_LBA_IN_MEMORY_INDEX_HPP_
#define SERIALIZER_LOG_LBA_IN_MEMORY_INDEX_HPP_


#include "containers/segmented_vector.hpp"
#include "config/args.hpp"
#include "serializer/serializer.hpp"
#include "serializer/log/lba/disk_format.hpp"


class in_memory_index_t
{
    // blocks.get_size() == timestamps.get_size().  We use parallel
    // arrays to avoid wasting memory from alignment.
    segmented_vector_t<flagged_off64_t, MAX_BLOCK_ID> blocks;
    segmented_vector_t<repli_timestamp_t, MAX_BLOCK_ID> timestamps;

public:
    in_memory_index_t();

    // end_block_id is one greater than the max block id.
    block_id_t end_block_id();

    struct info_t {
        flagged_off64_t offset;
        repli_timestamp_t recency;
    };

    info_t get_block_info(block_id_t id);
    void set_block_info(block_id_t id, repli_timestamp_t recency,
                        flagged_off64_t offset);

    bool is_offset_indexed(off64_t offset);
    block_id_t get_block_id(off64_t offset);

    // Rebuild the reverse mapping offset -> block id. Can become necessary on startup, when the LBA contains outdaited entries with offset collisions to recent ones
    void rebuild_reverse_index();

#ifndef NDEBUG
    void print();
#endif
};

#endif  // SERIALIZER_LOG_LBA_IN_MEMORY_INDEX_HPP_

