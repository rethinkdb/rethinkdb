#include "btree/operations.hpp"

#include "btree/modify_oper.hpp"
#include "btree/internal_node.hpp"
#include "btree/leaf_node.hpp"

void find_keyvalue_location_for_write(value_sizer_t *sizer, transaction_t *txn, buf_lock_t& sb_buf, btree_key_t *key, repli_timestamp_t tstamp, keyvalue_location_t *keyvalue_location_out) {
    buf_lock_t last_buf;
    buf_lock_t buf;
    get_root(sizer, txn, sb_buf, &buf, tstamp);

    // Walk down the tree to the leaf.
    while (node::is_internal(reinterpret_cast<const node_t *>(buf->get_data_read()))) {
        // Check if the node is overfull and proactively split it if it is (since this is an internal node).
        check_and_handle_split(sizer, txn, buf, last_buf, sb_buf, key, NULL, txn->get_cache()->get_block_size());
        // Check if the node is underfull, and merge/level if it is.
        check_and_handle_underfull(txn, buf, last_buf, sb_buf, key, txn->get_cache()->get_block_size());

        // Release the superblock, if we've gone past the root (and haven't
        // already released it). If we're still at the root or at one of
        // its direct children, we might still want to replace the root, so
        // we can't release the superblock yet.
        if (sb_buf.is_acquired() && last_buf.is_acquired()) {
            sb_buf.release();
        }

        // Release the old previous node (unless we're at the root), and set
        // the next previous node (which is the current node).

        // Look up and acquire the next node.
        block_id_t node_id = internal_node::lookup(reinterpret_cast<const internal_node_t *>(buf->get_data_read()), key);
        rassert(node_id != NULL_BLOCK_ID && node_id != SUPERBLOCK_ID);

        buf_lock_t tmp(txn, node_id, rwi_write);
        last_buf.swap(tmp);
        buf.swap(last_buf);
    }

    {
        scoped_malloc<value_type_t> tmp(sizer->max_possible_size());

        // We've gone down the tree and gotten to a leaf. Now look up the key.
        bool key_found = leaf::lookup(sizer, reinterpret_cast<const leaf_node_t *>(buf->get_data_read()), key, tmp.get());

        if (key_found) {
            keyvalue_location_out->value.swap(tmp);
        }
    }

    keyvalue_location_out->last_buf.swap(last_buf);
    keyvalue_location_out->buf.swap(buf);
}



