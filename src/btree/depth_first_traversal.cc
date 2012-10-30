// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "btree/depth_first_traversal.hpp"

#include "btree/operations.hpp"

bool btree_depth_first_traversal(btree_slice_t *slice, transaction_t *transaction, superblock_t *superblock, const key_range_t &range, depth_first_traversal_callback_t *cb) {
    block_id_t root_block_id = superblock->get_root_block_id();
    if (root_block_id == NULL_BLOCK_ID) {
        superblock->release();
        return true;
    } else {
        buf_lock_t root_block(transaction, root_block_id, rwi_read);
        superblock->release();
        return btree_depth_first_traversal(slice, transaction, &root_block, range, cb);
    }
}

bool btree_depth_first_traversal(btree_slice_t *slice, transaction_t *transaction, buf_lock_t *block, const key_range_t &range, depth_first_traversal_callback_t *cb) {
    const node_t *node = reinterpret_cast<const node_t *>(block->get_data_read());
    if (node::is_internal(node)) {
        const internal_node_t *inode = reinterpret_cast<const internal_node_t *>(node);
        int start_index = internal_node::get_offset_index(inode, range.left.btree_key());
        int end_index;
        if (range.right.unbounded) {
            end_index = inode->npairs;
        } else {
            store_key_t r = range.right.key;
            r.decrement();
            end_index = internal_node::get_offset_index(inode, r.btree_key()) + 1;
        }
        for (int i = start_index; i < end_index; i++) {
            const btree_internal_pair *pair = internal_node::get_pair_by_index(inode, i);
            buf_lock_t lock(transaction, pair->lnode, rwi_read);
            if (!btree_depth_first_traversal(slice, transaction, &lock, range, cb)) {
                return false;
            }
        }
        return true;
    } else {
        const leaf_node_t *lnode = reinterpret_cast<const leaf_node_t *>(node);
        const btree_key_t *key;
        for (leaf::live_iter_t it = leaf::iter_for_inclusive_lower_bound(lnode, range.left.btree_key());
                (key = it.get_key(lnode)) && (range.right.unbounded || sized_strcmp(key->contents, key->size, range.right.key.contents(), range.right.key.size()) < 0);
                it.step(lnode)) {
            if (!cb->handle_pair(key, it.get_value(lnode))) {
                return false;
            }
        }
        return true;
    }
}
