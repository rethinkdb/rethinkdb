#include "serializer/log/lba/in_memory_index.hpp"
#include "disk_format.hpp"
#include "in_memory_index.hpp"

in_memory_index_t::in_memory_index_t() { }

block_id_t in_memory_index_t::end_block_id() {
    return blocks.get_size();
}

in_memory_index_t::info_t in_memory_index_t::get_block_info(block_id_t id) {
    if (id >= blocks.get_size()) {
        info_t ret = { flagged_off64_t::unused(), repli_timestamp_t::invalid };
        return ret;
    } else {
        info_t ret = { blocks[id], timestamps[id] };
        return ret;
    }
}

void in_memory_index_t::set_block_info(block_id_t id, repli_timestamp_t recency,
                                     flagged_off64_t offset) {
    if (id >= blocks.get_size()) {
        blocks.set_size(id + 1, flagged_off64_t::unused());
        timestamps.set_size(id + 1, repli_timestamp_t::invalid);
    }

    blocks[id] = offset;
    timestamps[id] = recency;
}

#ifndef NDEBUG
void in_memory_index_t::print() {
    printf("LBA:\n");
    for (unsigned int i = 0; i < blocks.get_size(); i++) {
        printf("%d %.8lx\n", i, (unsigned long int)blocks[i].whole_value);
    }
}
#endif
