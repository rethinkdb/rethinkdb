// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "serializer/log/lba/in_memory_index.hpp"

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>

#include "serializer/log/lba/disk_format.hpp"

in_memory_index_t::in_memory_index_t() { }

block_id_t in_memory_index_t::end_block_id() {
    return blocks.get_size();
}

in_memory_index_t::info_t in_memory_index_t::get_block_info(block_id_t id) {
    if (id >= blocks.get_size()) {
        info_t ret = { flagged_off64_t::unused(),
                       repli_timestamp_t::invalid,
                       block_size_t::make_undefined() };
        return ret;
    } else {
        info_t ret = { blocks[id], timestamps[id],
                       block_size_t::make_from_serializer(block_sizes[id]) };
        return ret;
    }
}

void in_memory_index_t::set_block_info(block_id_t id, repli_timestamp_t recency,
                                       flagged_off64_t offset,
                                       block_size_t block_size) {
    if (id >= blocks.get_size()) {
        blocks.set_size(id + 1, flagged_off64_t::unused());
        timestamps.set_size(id + 1, repli_timestamp_t::invalid);
        block_sizes.set_size(id + 1, 0);
    }

    blocks[id] = offset;
    timestamps[id] = recency;
    block_sizes[id] = block_size.ser_value();
}

#ifndef NDEBUG
void in_memory_index_t::print() {
    printf("LBA:\n");
    for (unsigned int i = 0; i < blocks.get_size(); i++) {
        printf("%d %" PRId64 "\n", i, int64_t(blocks[i].the_value_));
    }
}
#endif
