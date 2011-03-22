#include "serializer/log/lba/in_memory_index.hpp"
#include "disk_format.hpp"
#include "in_memory_index.hpp"

in_memory_index_t::in_memory_index_t(size_t block_size) : block_size(block_size) { }


ser_block_id_t in_memory_index_t::end_block_id() {
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

    // If there is an old offset for this block id, we remove it from the reverse LBA first.
    // TODO! Instead of just checking has_value, do we also explicitly have to check is_delete? (also at another place)
    if (flagged_off64_t::has_value(blocks[id.value])) {
        rassert(blocks[id.value].parts.value % block_size == 0);
        rassert(block_ids.get_size() > blocks[id.value].parts.value / block_size);
        block_ids[blocks[id.value].parts.value / block_size] = ser_block_id_t::null();
    }

    blocks[id.value] = offset;
    timestamps[id.value] = recency;

    // Store reverse entry
    if (flagged_off64_t::has_value(offset)) {
        rassert(offset.parts.value % block_size == 0);
        const size_t offset_block = offset.parts.value / block_size;
        if (offset_block >= block_ids.get_size()) {
            block_ids.set_size(offset_block + 1, ser_block_id_t::null());
        }
        block_ids[offset_block] = id;
    }
}

bool in_memory_index_t::is_offset_indexed(off64_t offset) {
    rassert(offset % block_size == 0);
    const size_t offset_block = offset / block_size;
    return block_ids.get_size() > offset_block && block_ids[offset_block] != ser_block_id_t::null();
}

ser_block_id_t in_memory_index_t::get_block_id(off64_t offset) {
    rassert(is_offset_indexed(offset));
    rassert(offset % block_size == 0);
    const size_t offset_block = offset / block_size;

    // Just verify that stuff is consistent...
    rassert(get_block_info(block_ids[offset_block]).offset.parts.value == offset);
    rassert(!get_block_info(block_ids[offset_block]).offset.parts.is_delete);

    return block_ids[offset_block];
}

#ifndef NDEBUG
void in_memory_index_t::print() {
    printf("LBA:\n");
    for (unsigned int i = 0; i < blocks.get_size(); i++) {
        printf("%d %.8lx\n", i, (unsigned long int)blocks[i].whole_value);
    }
}
#endif
