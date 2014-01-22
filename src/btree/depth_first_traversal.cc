// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "btree/depth_first_traversal.hpp"

#include "btree/operations.hpp"
#include "rdb_protocol/profile.hpp"

/* Returns `true` if we reached the end of the subtree or range, and `false` if
`cb->handle_value()` returned `false`. */
bool btree_depth_first_traversal(btree_slice_t *slice,
                                 counted_t<counted_buf_lock_t> block,
                                 const key_range_t &range,
                                 depth_first_traversal_callback_t *cb,
                                 direction_t direction);

bool btree_depth_first_traversal(btree_slice_t *slice, superblock_t *superblock,
                                 const key_range_t &range,
                                 depth_first_traversal_callback_t *cb,
                                 direction_t direction) {
    block_id_t root_block_id = superblock->get_root_block_id();
    if (root_block_id == NULL_BLOCK_ID) {
        superblock->release();
        return true;
    } else {
        counted_t<counted_buf_lock_t> root_block;
        {
            // RSI: This profile information might be wrong, because before it seems
            // to measure how long it takes to acquire the block, but now (I think)
            // it just measures how long it takes to "get in line" i.e. no time at
            // all.
            profile::starter_t starter("Acquire block for read.", cb->get_trace());
            root_block = make_counted<counted_buf_lock_t>(superblock->expose_buf(),
                                                          root_block_id,
                                                          alt_access_t::read);
        }
        superblock->release();
        return btree_depth_first_traversal(slice, std::move(root_block), range, cb,
                                           direction);
    }
}

bool btree_depth_first_traversal(btree_slice_t *slice,
                                 counted_t<counted_buf_lock_t> block,
                                 const key_range_t &range,
                                 depth_first_traversal_callback_t *cb,
                                 direction_t direction) {
    buf_read_t read(block.get());
    const node_t *node = static_cast<const node_t *>(read.get_data_read());
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
            counted_t<counted_buf_lock_t> lock;
            {
                profile::starter_t starter("Acquire block for read.", cb->get_trace());
                lock = make_counted<counted_buf_lock_t>(block.get(), pair->lnode,
                                                        alt_access_t::read);
            }
            if (!btree_depth_first_traversal(slice,
                                             std::move(lock),
                                             range, cb, direction)) {
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
                if (!cb->handle_pair(scoped_key_value_t(key, (*it).second,
                                                        movable_t<counted_buf_lock_t>(block)))) {
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

                if (!cb->handle_pair(scoped_key_value_t(key, (*it).second,
                                                        movable_t<counted_buf_lock_t>(block)))) {
                    return false;
                }
            }
        }
        return true;
    }
}
