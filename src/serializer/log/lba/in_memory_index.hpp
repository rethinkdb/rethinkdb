// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef SERIALIZER_LOG_LBA_IN_MEMORY_INDEX_HPP_
#define SERIALIZER_LOG_LBA_IN_MEMORY_INDEX_HPP_

#include "arch/compiler.hpp"
#include "containers/two_level_array.hpp"
#include "config/args.hpp"
#include "serializer/serializer.hpp"
#include "serializer/log/lba/disk_format.hpp"

ATTR_PACKED(struct index_block_info_t {
    index_block_info_t()
        : offset(flagged_off64_t::unused()),
          recency(repli_timestamp_t::invalid),
          ser_block_size(0) { }

    index_block_info_t(flagged_off64_t _offset,
                       repli_timestamp_t _recency,
                       uint32_t _ser_block_size)
        : offset(_offset),
          recency(_recency),
          ser_block_size(_ser_block_size) { }

    // For two_level_array_t.
    bool operator==(const index_block_info_t &other) const {
        return offset == other.offset &&
            recency == other.recency &&
            ser_block_size == other.ser_block_size;
    }

    flagged_off64_t offset;
    repli_timestamp_t recency;
    uint32_t ser_block_size;
});



class in_memory_index_t {
    two_level_array_t<index_block_info_t> infos_;
    block_id_t end_block_id_;

public:
    in_memory_index_t();

    // end_block_id is one greater than the max block id.
    block_id_t end_block_id();

    index_block_info_t get_block_info(block_id_t id);
    void set_block_info(block_id_t id, repli_timestamp_t recency,
                        flagged_off64_t offset, uint32_t ser_block_size);

};

#endif  // SERIALIZER_LOG_LBA_IN_MEMORY_INDEX_HPP_

