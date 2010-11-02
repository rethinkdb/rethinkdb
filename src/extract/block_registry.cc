#include "extract/block_registry.hpp"

#include "logger.hpp"

typedef data_block_manager_t::buf_data_t buf_data_t;

block_registry::block_registry() { }


void block_registry::tell_block(off64_t offset, buf_data_t *buf_data) {
    ser_block_id_t block_id = buf_data->block_id;
    ser_transaction_id_t transaction_id = buf_data->transaction_id;
    
    if (block_id >= transaction_ids.get_size()) {
        transaction_ids.set_size(block_id + 1, NULL_SER_TRANSACTION_ID);
        offsets.set_size(block_id + 1, null);
    }

    ser_transaction_id_t *curr_transaction_id = &transaction_ids[block_id];
    if (*curr_transaction_id < transaction_id) {
        *curr_transaction_id = transaction_id;
        offsets[block_id] = offset;
    }
}

bool block_registry::check_block_id_contiguity() const {
    ser_block_id_t n = transaction_ids.get_size();

    for (ser_block_id_t i = 0; i < n; ++i) {
        if (transaction_ids[i] == NULL_SER_TRANSACTION_ID) {
            return false;
        }
    }

    return true;
}

const segmented_vector_t<off64_t, MAX_BLOCK_ID>& block_registry::destroy_transaction_ids() {
    transaction_ids.set_size(0);
    return offsets;
}

