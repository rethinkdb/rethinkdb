#include "serializer/log/lba/in_memory_index.hpp"

in_memory_index_t::in_memory_index_t() { }


ser_block_id_t in_memory_index_t::max_block_id() {
    return ser_block_id_t::make(blocks.get_size());
}

in_memory_index_t::info_t in_memory_index_t::get_block_info(ser_block_id_t id) {
    if (id.value >= blocks.get_size()) {
        info_t ret = { flagged_off64_t::unused(), repli_timestamp::invalid };
        return ret;
    } else {
        info_t ret = { blocks[id.value], timestamps[id.value] };
        return ret;
    }
}

void in_memory_index_t::set_block_info(ser_block_id_t id, repli_timestamp recency,
                                     flagged_off64_t offset) {
    if (id.value >= blocks.get_size()) {
        blocks.set_size(id.value + 1, flagged_off64_t::unused());
        timestamps.set_size(id.value + 1, repli_timestamp::invalid);
    }

    blocks[id.value] = offset;
    timestamps[id.value] = recency;
}

#ifndef NDEBUG
void in_memory_index_t::print() {
    printf("LBA:\n");
    for (unsigned int i = 0; i < blocks.get_size(); i++) {
        printf("%d %.8lx\n", i, (unsigned long int)blocks[i].whole_value);
    }
}
#endif
