// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "serializer/log/lba/in_memory_index.hpp"

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>

#include "serializer/log/lba/disk_format.hpp"

in_memory_index_t::in_memory_index_t() : end_block_id_(0) { }

block_id_t in_memory_index_t::end_block_id() {
    return end_block_id_;
}

index_block_info_t in_memory_index_t::get_block_info(block_id_t id) {
    return infos_.get(id);
}

void in_memory_index_t::set_block_info(block_id_t id, repli_timestamp_t recency,
                                       flagged_off64_t offset, uint32_t ser_block_size) {
    if (id >= end_block_id_) {
        end_block_id_ = id + 1;
    }

    index_block_info_t info(offset, recency, ser_block_size);
    infos_.set(id, info);
}

