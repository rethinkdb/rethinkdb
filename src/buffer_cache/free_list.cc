#include "buffer_cache/free_list.hpp"

#include "serializer/serializer.hpp"

namespace alt {

free_list_t::free_list_t(serializer_t *serializer) {
    on_thread_t th(serializer->home_thread());

    next_new_block_id_ = serializer->end_block_id();
    next_new_aux_block_id_ = serializer->end_aux_block_id();
    for (block_id_t i = 0; i < next_new_block_id_; ++i) {
        if (serializer->get_delete_bit(i)) {
            free_ids_.push_back(i);
        }
    }
    for (block_id_t i = FIRST_AUX_BLOCK_ID; i < next_new_aux_block_id_; ++i) {
        if (serializer->get_delete_bit(i)) {
            free_aux_ids_.push_back(i);
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

block_id_t free_list_t::acquire_aux_block_id() {
    if (free_aux_ids_.empty()) {
        block_id_t ret = next_new_aux_block_id_;
        ++next_new_aux_block_id_;
        return ret;
    } else {
        block_id_t ret = free_aux_ids_.back();
        free_aux_ids_.pop_back();
        return ret;
    }
}

void free_list_t::acquire_chosen_block_id(block_id_t block_id) {
    block_id_t *next_new_id = is_aux_block_id(block_id)
                              ? &next_new_aux_block_id_
                              : &next_new_block_id_;
    segmented_vector_t<block_id_t> *free_ids = is_aux_block_id(block_id)
                                               ? &free_aux_ids_
                                               : &free_ids_;
    if (block_id >= *next_new_id) {
        const block_id_t old = *next_new_id;
        *next_new_id = block_id + 1;
        for (block_id_t i = old; i < block_id; ++i) {
            free_ids->push_back(i);
        }
    } else {
        for (size_t i = 0, e = free_ids->size(); i < e; ++i) {
            if ((*free_ids)[i] == block_id) {
                (*free_ids)[i] = free_ids->back();
                free_ids->pop_back();
                return;
            }
        }

        crash("acquire_chosen_block_id tried to use %" PR_BLOCK_ID
              ", but it was taken.", block_id);
    }
}

void free_list_t::release_block_id(block_id_t block_id) {
    if (is_aux_block_id(block_id)) {
        free_aux_ids_.push_back(block_id);
    } else {
        free_ids_.push_back(block_id);
    }
}




}  // namespace alt
