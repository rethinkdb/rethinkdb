// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "btree/depth_first_traversal.hpp"

#include "btree/operations.hpp"

/* Returns `true` if we reached the end of the subtree or range, and `false` if
`cb->handle_value()` returned `false`. */
bool btree_depth_first_traversal(btree_slice_t *slice, transaction_t *transaction,
        buf_lock_t *block, const key_range_t &range,
        depth_first_traversal_callback_t *cb, direction_t direction);

bool btree_depth_first_traversal(btree_slice_t *slice, transaction_t *transaction, superblock_t *superblock, const key_range_t &range, depth_first_traversal_callback_t *cb, direction_t direction) {
    block_id_t root_block_id = superblock->get_root_block_id();
    if (root_block_id == NULL_BLOCK_ID) {
        superblock->release();
        return true;
    } else {
        buf_lock_t root_block(transaction, root_block_id, rwi_read);
        superblock->release();
        return btree_depth_first_traversal(slice, transaction, &root_block, range, cb, direction);
    }
}

bool btree_depth_first_traversal(btree_slice_t *slice, transaction_t *transaction, buf_lock_t *block, const key_range_t &range, depth_first_traversal_callback_t *cb, direction_t direction) {
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
        for (int i = 0; i < end_index - start_index; ++i) {
            int true_index = (direction == FORWARD ? start_index + i : (end_index - 1) - i);
            const btree_internal_pair *pair = internal_node::get_pair_by_index(inode, true_index);
            buf_lock_t lock(transaction, pair->lnode, rwi_read);
            if (!btree_depth_first_traversal(slice, transaction, &lock, range, cb, direction)) {
                return false;
            }
        }
        return true;
    } else {
        const leaf_node_t *lnode = reinterpret_cast<const leaf_node_t *>(node);
        const btree_key_t *key;

        if (direction == FORWARD) {
            for (auto it = leaf::inclusive_lower_bound(range.left.btree_key(), *lnode);
                 it != leaf::end(*lnode); ++it) {
                key = (*it).first;
                if (!range.right.unbounded &&
                    btree_key_cmp(key, range.right.key.btree_key()) >= 0) {
                    break;
                }
                if (!cb->handle_pair(key, (*it).second)) {
                    return false;
                }
            }
        } else {
            leaf_node_t::reverse_iterator it;
            if (range.right.unbounded) {
                it = leaf::rbegin(*lnode);
            } else {
                it = leaf::inclusive_upper_bound(range.right.key.btree_key(), *lnode);
            }
            for (/* assignment above */; it != leaf::rend(*lnode); ++it) {
                key = (*it).first;

                if (btree_key_cmp(key, range.left.btree_key()) <= 0) {
                    break;
                }

                if (!cb->handle_pair(key, (*it).second)) {
                    return false;
                }
            }
        }
        return true;
    }
}
