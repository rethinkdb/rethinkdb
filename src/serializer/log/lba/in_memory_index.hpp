// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef SERIALIZER_LOG_LBA_IN_MEMORY_INDEX_HPP_
#define SERIALIZER_LOG_LBA_IN_MEMORY_INDEX_HPP_


#include "containers/segmented_vector.hpp"
#include "config/args.hpp"
#include "serializer/serializer.hpp"
#include "serializer/log/lba/disk_format.hpp"


class in_memory_index_t
{
    // {blocks,timestamps,block_sizes}.get_size() are all equal.
    segmented_vector_t<flagged_off64_t, MAX_BLOCK_ID> blocks;
    segmented_vector_t<repli_timestamp_t, MAX_BLOCK_ID> timestamps;
    segmented_vector_t<uint32_t, MAX_BLOCK_ID> block_sizes;

public:
    in_memory_index_t();

    // end_block_id is one greater than the max block id.
    block_id_t end_block_id();

    struct info_t {
        flagged_off64_t offset;
        repli_timestamp_t recency;
        block_size_t block_size;
    };

    // RSI: Force callers of this function to handle block_size.  (Change the type signature and
    // figure out who's calling this.  Right now nobody actually uses the block size.)
    info_t get_block_info(block_id_t id);
    void set_block_info(block_id_t id, repli_timestamp_t recency,
                        flagged_off64_t offset,
                        block_size_t block_size);

#ifndef NDEBUG
    void print();
#endif
};

#endif  // SERIALIZER_LOG_LBA_IN_MEMORY_INDEX_HPP_

