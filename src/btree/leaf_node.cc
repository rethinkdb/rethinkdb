#include "btree/leaf_node.hpp"

namespace leaf {

// Returns an iterator that starts at the first entry whose key is
// greater than or equal to key.
live_iter_t iter_for_inclusive_lower_bound(const leaf_node_t *node, const btree_key_t *key) {
    int index;
    find_key(node, key, &index);
    while (index < node->num_pairs && !entry_is_live(get_entry(node, node->pair_offsets[index]))) {
        ++index;
    }
    return live_iter_t(index);
}

// Returns an iterator that starts at the smallest key.
live_iter_t iter_for_whole_leaf(const leaf_node_t *node) {
    int index = 0;
    while (index < node->num_pairs && !entry_is_live(get_entry(node, node->pair_offsets[index]))) {
        ++index;
    }
    return live_iter_t(index);
}



}  // namespace leaf


void leaf_patched_remove(buf_lock_t *node, const btree_key_t *key, repli_timestamp_t tstamp, UNUSED key_modification_proof_t km_proof) {
    // rassert(!km_proof.is_fake());
    node->apply_patch(new leaf_remove_patch_t(node->get_block_id(), node->get_next_patch_counter(), tstamp, key->size, key->contents));
}

void leaf_patched_erase_presence(buf_lock_t *node, const btree_key_t *key, UNUSED key_modification_proof_t km_proof) {
    // TODO: Maybe we don't need key modification proof here.
    // rassert(!km_proof.is_fake());
    node->apply_patch(new leaf_erase_presence_patch_t(node->get_block_id(), node->get_next_patch_counter(), key->size, key->contents));
}

