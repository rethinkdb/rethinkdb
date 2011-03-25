#include "extract/block_registry.hpp"

#include "logger.hpp"

block_registry::block_registry() { }


void block_registry::tell_block(UNUSED off64_t offset, UNUSED const ls_buf_data_t& buf_data) {
    ser_block_id_t block_id = buf_data.block_id;
    if (block_id.value < MAX_BLOCK_ID) {
        ser_block_sequence_id_t block_sequence_id = buf_data.block_sequence_id;

        if (block_id.value >= block_sequence_ids.get_size()) {
            block_sequence_ids.set_size(block_id.value + 1, NULL_SER_BLOCK_SEQUENCE_ID);
            offsets.set_size(block_id.value + 1, null);
        }

        ser_block_sequence_id_t *curr_block_sequence_id = &block_sequence_ids[block_id.value];
        if (*curr_block_sequence_id < block_sequence_id) {
            *curr_block_sequence_id = block_sequence_id;
            offsets[block_id.value] = offset;
        }
    }
}

bool block_registry::has_block(ser_block_id_t block_id, off64_t offset) const {
    return block_id.value < offsets.get_size() && offsets[block_id.value] == offset;
}

bool block_registry::check_block_id_contiguity() const {
    ser_block_id_t::number_t n = block_sequence_ids.get_size();

    for (ser_block_id_t::number_t i = 0; i < n; ++i) {
        if (block_sequence_ids[i] == NULL_SER_BLOCK_SEQUENCE_ID) {
            return false;
        }
    }

    return true;
}

const segmented_vector_t<off64_t, MAX_BLOCK_ID>& block_registry::destroy_block_sequence_ids() {
    block_sequence_ids.set_size(0);
    return offsets;
}

