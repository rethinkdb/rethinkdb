#include "extract/block_registry.hpp"

#include "logger.hpp"

block_registry::block_registry() { }

void block_registry::tell_block(off64_t offset, const ls_buf_data_t& buf_data) {
    block_id_t block_id = buf_data.block_id;
    if (block_id < MAX_BLOCK_ID) {
        block_sequence_id_t block_sequence_id = buf_data.block_sequence_id;

        block_sequence_id_t *curr_block_sequence_id = &block_sequence_ids[block_id];
        if (*curr_block_sequence_id < block_sequence_id) {
            *curr_block_sequence_id = block_sequence_id;
            offsets[block_id] = offset;
        }
    }
}

bool block_registry::has_block(block_id_t block_id, off64_t offset) {
    return block_id < offsets.size() && offsets[block_id] == offset;
}

const std::map<size_t, off64_t>& block_registry::destroy_block_sequence_ids() {
    block_sequence_ids.clear();
    return offsets;
}

