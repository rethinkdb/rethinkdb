#include "extract/block_registry.hpp"

#include "logger.hpp"

block_registry::block_registry() { }


void block_registry::tell_block(off64_t offset, const buf_data_t& buf_data) {
    ser_block_id_t block_id = buf_data.block_id;
    if (block_id.value < MAX_BLOCK_ID) {
        ser_transaction_id_t transaction_id = buf_data.transaction_id;

        ser_transaction_id_t *curr_transaction_id = &transaction_ids[block_id.value];
        if (*curr_transaction_id < transaction_id) {
            *curr_transaction_id = transaction_id;
            offsets[block_id.value] = offset;
        }
    }
}

bool block_registry::has_block(ser_block_id_t block_id, off64_t offset) {
    return block_id.value < offsets.size() && offsets[block_id.value] == offset;
}

const std::map<size_t, off64_t>& block_registry::destroy_transaction_ids() {
    transaction_ids.clear();
    return offsets;
}

