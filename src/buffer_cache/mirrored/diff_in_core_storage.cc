#include "diff_in_core_storage.hpp"

#include <list>
#include "errors.hpp"

diff_core_storage_t::diff_core_storage_t() {
    // TODO!
}

// This assumes that patches is properly sorted. It will initialize a block_patch_list_t and store it.
void diff_core_storage_t::load_block_patch_list(const block_id_t block_id, const std::list<buf_patch_t*>& patches) {
    rassert(patch_map.find(block_id) == patch_map.end());
    patch_map[block_id].assign(patches.begin(), patches.end());
}

    // Returns true iff any changes have been made to the buf
    bool flush_patches(const block_id_t block_id, char* buf_data);

// Clears the patch list and deletes all stored patches
void diff_core_storage_t::block_patch_list_t::truncate() {
    for (iterator patch = begin(); patch != end(); ++patch)
        delete *patch;
    clear();
}