#include "diff_in_core_storage.hpp"

#include <list>
#include <map>
#include "errors.hpp"

diff_core_storage_t::diff_core_storage_t() {
}

// This assumes that patches is properly sorted. It will initialize a block_patch_list_t and store it.
void diff_core_storage_t::load_block_patch_list(const block_id_t block_id, const std::list<buf_patch_t*>& patches) {
    rassert(patch_map.find(block_id) == patch_map.end());
    fprintf(stderr, "Loaded list as follows:\n");
    for (std::list<buf_patch_t*>::const_iterator patch = patches.begin(); patch != patches.end(); ++patch)
        fprintf(stderr, "    TID=%p, PID=%d\n", (void*)(*patch)->get_transaction_id(), (int)(*patch)->get_patch_counter());

    patch_map[block_id].assign(patches.begin(), patches.end());
}

// Removes all patches which are obsolete w.r.t. the given transaction_id
void diff_core_storage_t::filter_applied_patches(const block_id_t block_id, const ser_transaction_id_t transaction_id) {
    patch_map_t::iterator map_entry = patch_map.find(block_id);
    rassert(map_entry != patch_map.end());
    map_entry->second.filter_before_transaction(transaction_id);
    if (map_entry->second.size() == 0)
        patch_map.erase(map_entry);
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
}

// Return NULL if no patches exist for that block
const std::list<buf_patch_t*>* diff_core_storage_t::get_patches(const block_id_t block_id) const {
    patch_map_t::const_iterator map_entry = patch_map.find(block_id);
    if (map_entry == patch_map.end())
        return NULL;
    else
        return &(map_entry->second);
}

void diff_core_storage_t::drop_patches(const block_id_t block_id) {
    patch_map_t::iterator map_entry = patch_map.find(block_id);
    if (map_entry != patch_map.end())
        patch_map.erase(map_entry);
}

// Deletes all stored patches
diff_core_storage_t::block_patch_list_t::~block_patch_list_t() {
    for (iterator patch = begin(); patch != end(); ++patch)
        delete *patch;
}

void diff_core_storage_t::block_patch_list_t::filter_before_transaction(const ser_transaction_id_t transaction_id) {
    for (iterator patch = begin(); patch != end(); ++patch) {
        if ((*patch)->get_transaction_id() < transaction_id) {
            delete *patch;
            erase(patch);
            --patch;
        }
    }
}
