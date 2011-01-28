#include "diff_in_core_storage.hpp"

#include <list>
#include <map>
#include "errors.hpp"

diff_core_storage_t::diff_core_storage_t() {
}

// This assumes that patches is properly sorted. It will initialize a block_patch_list_t and store it.
void diff_core_storage_t::load_block_patch_list(const block_id_t block_id, const std::list<buf_patch_t*>& patches) {
    rassert(patch_map.find(block_id) == patch_map.end());
    patch_map[block_id].assign(patches.begin(), patches.end());
}

// Removes all patches up to (including) the given patch id
void diff_core_storage_t::truncate_applied_patches(const block_id_t block_id, const patch_counter_t patch_counter) {
    patch_map_t::iterator map_entry = patch_map.find(block_id);
    rassert(map_entry != patch_map.end());
    map_entry->second.truncate_up_to_patch(patch_counter);
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

void diff_core_storage_t::store_patch(const block_id_t block_id, buf_patch_t &patch) {
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

void diff_core_storage_t::block_patch_list_t::truncate_up_to_patch(const patch_counter_t patch_counter) {
    // Find last patch to delete and delete patches
    iterator delete_up_to_here = begin();
    while (delete_up_to_here != end() && (*delete_up_to_here)->get_patch_counter() < patch_counter) {
        delete *delete_up_to_here;
        ++delete_up_to_here;
    }

    // Remove pointers from list
    erase(begin(), delete_up_to_here);
}