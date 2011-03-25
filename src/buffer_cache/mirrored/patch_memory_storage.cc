#include "patch_memory_storage.hpp"

#include <list>
#include <map>
#include "errors.hpp"

patch_memory_storage_t::patch_memory_storage_t() {
}

// This assumes that patches is properly sorted. It will initialize a block_patch_list_t and store it.
void patch_memory_storage_t::load_block_patch_list(block_id_t block_id, const std::list<buf_patch_t*>& patches) {
    rassert(patch_map.find(block_id) == patch_map.end());

    // Verify patches list
    ser_transaction_id_t previous_transaction = 0;
    patch_counter_t previous_patch_counter = 0;
    for (std::list<buf_patch_t*>::const_iterator p = patches.begin(); p != patches.end(); ++p) {
        if ((*p)->get_transaction_id() != previous_transaction) {
            rassert((*p)->get_transaction_id() > previous_transaction, "Non-sequential patch list: Transaction id %ll follows %ll", (*p)->get_transaction_id(), previous_transaction);
        }
        if (previous_transaction == 0 || (*p)->get_transaction_id() != previous_transaction) {
            previous_patch_counter = 0;
        }
        guarantee(previous_patch_counter == 0 || (*p)->get_patch_counter() > previous_patch_counter, "Non-sequential patch list: Patch counter %d follows %d", (*p)->get_patch_counter(), previous_patch_counter);
        previous_patch_counter = (*p)->get_patch_counter();
        previous_transaction = (*p)->get_transaction_id();
    }

    block_patch_list_t& summarizing_patch_list = patch_map[block_id];

    for (std::list<buf_patch_t*>::const_iterator p = patches.begin(), e = patches.end(); p != e; ++p) {
        summarizing_patch_list.add_patch(*p);
    }
}

// Removes all patches which are obsolete w.r.t. the given transaction_id
void patch_memory_storage_t::filter_applied_patches(block_id_t block_id, ser_transaction_id_t transaction_id) {
    patch_map_t::iterator map_entry = patch_map.find(block_id);
    rassert(map_entry != patch_map.end());
    map_entry->second.filter_before_transaction(transaction_id);
    if (map_entry->second.empty()) {
        patch_map.erase(map_entry);
    }
#ifndef NDEBUG
    else {
        map_entry->second.verify_patches_list(transaction_id);
    }
#endif
}

// Returns true iff any changes have been made to the buf
bool patch_memory_storage_t::apply_patches(block_id_t block_id, char *buf_data) const {
    patch_map_t::const_iterator map_entry = patch_map.find(block_id);
    if (map_entry == patch_map.end()) {
        return false;
    }

    for (block_patch_list_t::const_iterator p = map_entry->second.patches_begin(), e = map_entry->second.patches_end();
         p != e;
         ++p) {
        (*p)->apply_to_buf(buf_data);
    }

    return true;
}

bool patch_memory_storage_t::has_patches_for_block(block_id_t block_id) const {
    patch_map_t::const_iterator map_entry = patch_map.find(block_id);
    if (map_entry == patch_map.end()) {
        return false;
    }
    return !map_entry->second.empty();
}

patch_counter_t patch_memory_storage_t::last_patch_materialized_or_zero(block_id_t block_id) const {
    patch_map_t::const_iterator map_entry = patch_map.find(block_id);
    if (map_entry == patch_map.end() || map_entry->second.empty()) {
        // Nothing of relevance is materialized (only obsolete patches if any).
        return 0;
    } else {
        // TODO: ugh.
        return (*(map_entry->second.patches_end() - 1))->get_patch_counter();
    }
}

buf_patch_t *patch_memory_storage_t::first_patch(block_id_t block_id) const {
    return *patches_for_block(block_id).first;
}

std::pair<patch_memory_storage_t::const_patch_iterator, patch_memory_storage_t::const_patch_iterator>
patch_memory_storage_t::patches_for_block(block_id_t block_id) const {
    patch_map_t::const_iterator map_entry = patch_map.find(block_id);
    rassert(map_entry != patch_map.end());
    rassert(!map_entry->second.empty());
    return std::make_pair(map_entry->second.patches_begin(), map_entry->second.patches_end());
}

void patch_memory_storage_t::drop_patches(block_id_t block_id) {
    patch_map_t::iterator map_entry = patch_map.find(block_id);
    if (map_entry != patch_map.end())
        patch_map.erase(map_entry);
}

patch_memory_storage_t::block_patch_list_t::block_patch_list_t() {
    patches_.reserve(32);
    affected_data_size_ = 0;
}

// Deletes all stored patches
patch_memory_storage_t::block_patch_list_t::~block_patch_list_t() {
    for (std::vector<buf_patch_t *>::iterator p = patches_.begin(), e = patches_.end(); p != e; ++p) {
        delete *p;
    }
}

void patch_memory_storage_t::block_patch_list_t::add_patch(buf_patch_t *patch) {
    patches_.push_back(patch);
    affected_data_size_ += patch->get_affected_data_size();
}

void patch_memory_storage_t::block_patch_list_t::filter_before_transaction(const ser_transaction_id_t transaction_id) {
    std::vector<buf_patch_t *>::iterator first_patch_to_keep = patches_.end();
    for (std::vector<buf_patch_t *>::iterator p = patches_.begin(), e = patches_.end(); p != e; ++p) {
        if ((*p)->get_transaction_id() < transaction_id) {
            affected_data_size_ -= (*p)->get_affected_data_size();
            delete *p;
        } else {
            first_patch_to_keep = p;
            break;
        }
    }
    patches_.erase(patches_.begin(), first_patch_to_keep);
}

void patch_memory_storage_t::block_patch_list_t::verify_patches_list(ser_transaction_id_t transaction_id) const {
    // Verify patches list (with strict start patch counter)
    ser_transaction_id_t previous_transaction = 0;
    patch_counter_t previous_patch_counter = 0;
    for (std::vector<buf_patch_t*>::const_iterator p = patches_.begin(), e = patches_.end(); p != e; ++p) {
        rassert((*p)->get_transaction_id() >= transaction_id || (*p)->get_transaction_id() == 0);
        if ((*p)->get_transaction_id() != previous_transaction) {
            guarantee((*p)->get_transaction_id() > previous_transaction, "Non-sequential patch list: Transaction id %ll follows %ll", (*p)->get_transaction_id(), previous_transaction);
        }
        if (previous_transaction == 0 || (*p)->get_transaction_id() != previous_transaction) {
            previous_patch_counter = 0;
        }
        guarantee((*p)->get_patch_counter() == previous_patch_counter + 1, "Non-sequential patch list: Patch counter %d follows %d", (*p)->get_patch_counter(), previous_patch_counter);
        ++previous_patch_counter;
        previous_transaction = (*p)->get_transaction_id();
    }
}
