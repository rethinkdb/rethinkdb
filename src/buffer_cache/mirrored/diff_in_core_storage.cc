#include "diff_in_core_storage.hpp"

#include <list>
#include <map>
#include "errors.hpp"

diff_core_storage_t::diff_core_storage_t() {
}

// This assumes that patches is properly sorted. It will initialize a block_patch_list_t and store it.
void diff_core_storage_t::load_block_patch_list(const block_id_t block_id, const std::list<buf_patch_t*>& patches) {
    rassert(patch_map.find(block_id) == patch_map.end());

    // Verify patches list
    ser_transaction_id_t previous_transaction = 0;
    patch_counter_t previous_patch_counter = 0;
    for(std::list<buf_patch_t*>::const_iterator p = patches.begin(); p != patches.end(); ++p) {
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

    patch_map[block_id].assign(patches.begin(), patches.end());
    patch_map[block_id].affected_data_size = 0;

    for(std::list<buf_patch_t*>::const_iterator p = patches.begin(); p != patches.end(); ++p) {
        patch_map[block_id].affected_data_size += (*p)->get_affected_data_size();
    }
}

// Removes all patches which are obsolete w.r.t. the given transaction_id
void diff_core_storage_t::filter_applied_patches(const block_id_t block_id, const ser_transaction_id_t transaction_id) {
    patch_map_t::iterator map_entry = patch_map.find(block_id);
    rassert(map_entry != patch_map.end());
    map_entry->second.filter_before_transaction(transaction_id);
    if (map_entry->second.size() == 0)
        patch_map.erase(map_entry);
#ifndef NDEBUG
    else {
        // Verify patches list (this time with strict start patch counter)
        ser_transaction_id_t previous_transaction = 0;
        patch_counter_t previous_patch_counter = 0;
        for(std::vector<buf_patch_t*>::const_iterator p = map_entry->second.begin(); p != map_entry->second.end(); ++p) {
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
#endif
}

// Returns true iff any changes have been made to the buf
bool diff_core_storage_t::apply_patches(const block_id_t block_id, char *buf_data) const {
    patch_map_t::const_iterator map_entry = patch_map.find(block_id);
    if (map_entry == patch_map.end())
        return false;

    const block_patch_list_t& patches = map_entry->second;

    for (block_patch_list_t::const_iterator patch = patches.begin(); patch != patches.end(); ++patch)
        (*patch)->apply_to_buf(buf_data);

    return true;
}

void diff_core_storage_t::store_patch(buf_patch_t &patch) {
    const block_id_t block_id = patch.get_block_id();
    patch_map[block_id].push_back(&patch);
    patch_map[block_id].affected_data_size += patch.get_affected_data_size();
}

// Return NULL if no patches exist for that block
const std::vector<buf_patch_t*>* diff_core_storage_t::get_patches(const block_id_t block_id) const {
    patch_map_t::const_iterator map_entry = patch_map.find(block_id);
    if (map_entry == patch_map.end())
        return NULL;
    else
        return &(map_entry->second);
}

size_t diff_core_storage_t::get_affected_data_size(const block_id_t block_id) const {
    patch_map_t::const_iterator map_entry = patch_map.find(block_id);
    if (map_entry == patch_map.end())
        return 0;
    else
        return map_entry->second.affected_data_size;
}

void diff_core_storage_t::drop_patches(const block_id_t block_id) {
    patch_map_t::iterator map_entry = patch_map.find(block_id);
    if (map_entry != patch_map.end())
        patch_map.erase(map_entry);
}

diff_core_storage_t::block_patch_list_t::block_patch_list_t() : std::vector<buf_patch_t*>() {
    reserve(32);
    affected_data_size = 0;
}

// Deletes all stored patches
diff_core_storage_t::block_patch_list_t::~block_patch_list_t() {
    for (iterator patch = begin(); patch != end(); ++patch)
        delete *patch;
}

void diff_core_storage_t::block_patch_list_t::filter_before_transaction(const ser_transaction_id_t transaction_id) {
    iterator first_patch_to_keep = end();
    for (iterator patch = begin(); patch != end(); ++patch) {
        if ((*patch)->get_transaction_id() < transaction_id) {
            affected_data_size -= (*patch)->get_affected_data_size();
            delete *patch;
        } else {
            first_patch_to_keep = patch;
            break;
        }
    }
    erase(begin(), first_patch_to_keep);
}
