// Copyright 2010-2012 RethinkDB, all rights reserved.
#define __STDC_FORMAT_MACROS
#include "buffer_cache/mirrored/patch_memory_storage.hpp"

#include <inttypes.h>
#include <list>
#include <map>
#include "errors.hpp"

patch_memory_storage_t::patch_memory_storage_t() {
}

// Removes all patches which are obsolete w.r.t. the given block sequence_id
void patch_memory_storage_t::filter_applied_patches(UNUSED block_id_t block_id, UNUSED block_sequence_id_t block_sequence_id) {
    guarantee(false);
}

// Returns true iff any changes have been made to the buf
bool patch_memory_storage_t::apply_patches(UNUSED block_id_t block_id, UNUSED char *buf_data, UNUSED block_size_t bs) const {
    guarantee(patch_map.empty());
    return false;
}

bool patch_memory_storage_t::has_patches_for_block(UNUSED block_id_t block_id) const {
    guarantee(patch_map.empty());
    return false;
}

patch_counter_t patch_memory_storage_t::last_patch_materialized_or_zero(UNUSED block_id_t block_id) const {
    guarantee(patch_map.empty());
    return 0;
}

std::pair<patch_memory_storage_t::const_patch_iterator, patch_memory_storage_t::const_patch_iterator>
patch_memory_storage_t::patches_for_block(UNUSED block_id_t block_id) const {
    crash("can't call this");
}

void patch_memory_storage_t::drop_patches(UNUSED block_id_t block_id) {
    guarantee(patch_map.empty());
}

patch_memory_storage_t::block_patch_list_t::block_patch_list_t() {
    patches_.reserve(32);
    patches_serialized_size_ = 0;
}

// Deletes all stored patches
patch_memory_storage_t::block_patch_list_t::~block_patch_list_t() {
    rassert(patches_serialized_size_ >= 0);
    for (std::vector<buf_patch_t *>::iterator p = patches_.begin(), e = patches_.end(); p != e; ++p) {
        delete *p;
    }
}

void patch_memory_storage_t::block_patch_list_t::add_patch(buf_patch_t *patch) {
    patches_.push_back(patch);
    rassert(patches_serialized_size_ >= 0);
    patches_serialized_size_ += patch->get_serialized_size();
}

void patch_memory_storage_t::block_patch_list_t::filter_before_block_sequence(block_sequence_id_t block_sequence_id) {
    std::vector<buf_patch_t *>::iterator first_patch_to_keep = patches_.end();
    for (std::vector<buf_patch_t *>::iterator p = patches_.begin(), e = patches_.end(); p != e; ++p) {
        if ((*p)->get_block_sequence_id() < block_sequence_id) {
            patches_serialized_size_ -= (*p)->get_serialized_size();
            delete *p;
        } else {
            first_patch_to_keep = p;
            break;
        }
    }
    patches_.erase(patches_.begin(), first_patch_to_keep);

    rassert(patches_serialized_size_ >= 0);
}

#ifndef NDEBUG
void patch_memory_storage_t::block_patch_list_t::verify_patches_list(block_sequence_id_t block_sequence_id) const {
    // Verify patches list (with strict start patch counter)
    block_sequence_id_t previous_block_sequence = 0;
    patch_counter_t previous_patch_counter = 0;
    for (std::vector<buf_patch_t*>::const_iterator p = patches_.begin(), e = patches_.end(); p != e; ++p) {
        rassert((*p)->get_block_sequence_id() >= block_sequence_id || (*p)->get_block_sequence_id() == 0);
        rassert(previous_block_sequence <= (*p)->get_block_sequence_id(),
                "Non-sequential patch list: Block sequence id %" PRIu64 " follows %" PRIu64,
                (*p)->get_block_sequence_id(), previous_block_sequence);
        if (previous_block_sequence == 0 || (*p)->get_block_sequence_id() != previous_block_sequence) {
            previous_patch_counter = 0;
        }
        guarantee((*p)->get_patch_counter() == previous_patch_counter + 1, "Non-sequential patch list: Patch counter %d follows %d", (*p)->get_patch_counter(), previous_patch_counter);
        ++previous_patch_counter;
        previous_block_sequence = (*p)->get_block_sequence_id();
    }
    guarantee(patches_serialized_size_ >= 0);
}
#endif
