#include "buffer_cache/alt/free_list.hpp"

#include "serializer/serializer.hpp"

namespace alt {

free_list_t::free_list_t(serializer_t *serializer) {
    on_thread_t th(serializer->home_thread());

    next_new_block_id_ = serializer->max_block_id();
    for (block_id_t i = 0; i < next_new_block_id_; ++i) {
        if (serializer->get_delete_bit(i)) {
            free_ids_.push_back(i);
        }
    }
}

free_list_t::~free_list_t() { }

block_id_t free_list_t::acquire_block_id() {
    if (free_ids_.empty()) {
        block_id_t ret = next_new_block_id_;
        ++next_new_block_id_;
        return ret;
    } else {
        block_id_t ret = free_ids_.back();
        free_ids_.pop_back();
        return ret;
    }
}

void free_list_t::acquire_chosen_block_id(block_id_t block_id) {
    if (block_id >= next_new_block_id_) {
        const block_id_t old = next_new_block_id_;
        next_new_block_id_ = block_id + 1;
        for (block_id_t i = old; i < block_id; ++i) {
            free_ids_.push_back(i);
        }
    } else {
        for (size_t i = 0, e = free_ids_.size(); i < e; ++i) {
            if (free_ids_[i] == block_id) {
                free_ids_[i] = free_ids_.back();
                free_ids_.pop_back();
                return;
            }
        }

        crash("acquire_chosen_block_id tried to use %" PR_BLOCK_ID
              ", but it was taken.", block_id);
    }
}

void free_list_t::release_block_id(block_id_t block_id) {
    free_ids_.push_back(block_id);
}




}  // namespace alt
