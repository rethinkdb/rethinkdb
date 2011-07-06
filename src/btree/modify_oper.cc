
#include "btree/modify_oper.hpp"

#include "utils.hpp"
#include <boost/shared_ptr.hpp>

#include "buffer_cache/buf_lock.hpp"
#include "buffer_cache/co_functions.hpp"
#include "btree/internal_node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/operations.hpp"
#include "btree/slice.hpp"

// TODO: consider B#/B* trees to improve space efficiency

// TODO: perhaps allow memory reclamation due to oversplitting? We can
// be smart and only use a limited amount of ram for incomplete nodes
// (doing this efficiently very tricky for high insert
// workloads). Also, if the serializer is log-structured, we can write
// only a small part of each node.

// TODO: change rwi_write to rwi_intent followed by rwi_upgrade where
// relevant.

void insert_root(block_id_t root_id, buf_lock_t& sb_buf) {
    rassert(sb_buf.is_acquired());
    // TODO: WTF is up with this const cast?  This makes NO SENSE.  gtfo.
    sb_buf->set_data(const_cast<block_id_t *>(&reinterpret_cast<const btree_superblock_t *>(sb_buf->get_data_read())->root_block), &root_id, sizeof(root_id));

    sb_buf.release();
}

// Split the node if necessary. If the node is a leaf_node, provide the new
// value that will be inserted; if it's an internal node, provide NULL (we
// split internal nodes proactively).
void check_and_handle_split(value_sizer_t *sizer, transaction_t *txn, buf_lock_t& buf, buf_lock_t& last_buf, buf_lock_t& sb_buf,
                            const btree_key_t *key, value_type_t *new_value, block_size_t block_size) {
    txn->assert_thread();

    const node_t *node = reinterpret_cast<const node_t *>(buf->get_data_read());

    // If the node isn't full, we don't need to split, so we're done.
    if (node::is_leaf(node)) { // This should only be called when update_needed.
        rassert(new_value);
        if (!leaf::is_full(sizer, reinterpret_cast<const leaf_node_t *>(node), key, new_value)) {
            return;
        }
    } else {
        rassert(!new_value);
        if (!internal_node::is_full(reinterpret_cast<const internal_node_t *>(node))) {
            return;
        }
    }

    // Allocate a new node to split into, and some temporary memory to keep
    // track of the median key in the split; then actually split.
    buf_lock_t rbuf;
    rbuf.allocate(txn);
    btree_key_buffer_t median_buffer;
    btree_key_t *median = median_buffer.key();

    node::split(block_size, *buf.buf(), *rbuf.buf(), median);

    // Insert the key that sets the two nodes apart into the parent.
    if (!last_buf.is_acquired()) {
        // We're splitting what was previously the root, so create a new root to use as the parent.
        last_buf.allocate(txn);
        internal_node::init(block_size, *last_buf.buf());

        insert_root(last_buf->get_block_id(), sb_buf);
    }

    bool success __attribute__((unused)) = internal_node::insert(block_size, *last_buf.buf(), median, buf->get_block_id(), rbuf->get_block_id());
    rassert(success, "could not insert internal btree node");

    // We've split the node; now figure out where the key goes and release the other buf (since we're done with it).
    if (0 >= sized_strcmp(key->contents, key->size, median->contents, median->size)) {
        // The key goes in the old buf (the left one).

        // Do nothing.

    } else {
        // The key goes in the new buf (the right one).
        buf.swap(rbuf);
    }
}

// Merge or level the node if necessary.
void check_and_handle_underfull(transaction_t *txn, buf_lock_t& buf, buf_lock_t& last_buf, buf_lock_t& sb_buf,
                                const btree_key_t *key, block_size_t block_size) {
    const node_t *node = reinterpret_cast<const node_t *>(buf->get_data_read());
    if (last_buf.is_acquired() && node::is_underfull(block_size, node)) { // The root node is never underfull.

        const internal_node_t *parent_node = reinterpret_cast<const internal_node_t *>(last_buf->get_data_read());

        // Acquire a sibling to merge or level with.
        block_id_t sib_node_id;
        int nodecmp_node_with_sib = internal_node::sibling(parent_node, key, &sib_node_id);

        // Now decide whether to merge or level.
        buf_lock_t sib_buf(txn, sib_node_id, rwi_write);
        const node_t *sib_node = reinterpret_cast<const node_t *>(sib_buf->get_data_read());

#ifndef NDEBUG
        node::validate(block_size, sib_node);
#endif

        if (node::is_mergable(block_size, node, sib_node, parent_node)) { // Merge.

            // This is the key that we remove.
            btree_key_buffer_t key_to_remove_buffer;
            btree_key_t *key_to_remove = key_to_remove_buffer.key();

            if (nodecmp_node_with_sib < 0) { // Nodes must be passed to merge in ascending order.
                node::merge(block_size, node, *sib_buf.buf(), key_to_remove, parent_node);
                buf->mark_deleted();
                buf.swap(sib_buf);
            } else {
                node::merge(block_size, sib_node, *buf.buf(), key_to_remove, parent_node);
                sib_buf->mark_deleted();
            }

            sib_buf.release();

            if (!internal_node::is_singleton(parent_node)) {
                internal_node::remove(block_size, *last_buf.buf(), key_to_remove);
            } else {
                // The parent has only 1 key after the merge (which means that
                // it's the root and our node is its only child). Insert our
                // node as the new root.
                last_buf->mark_deleted();
                insert_root(buf->get_block_id(), sb_buf);
            }
        } else { // Level
            btree_key_buffer_t key_to_replace_buffer, replacement_key_buffer;
            btree_key_t *key_to_replace = key_to_replace_buffer.key();
            btree_key_t *replacement_key = replacement_key_buffer.key();

            bool leveled = node::level(block_size, *buf.buf(), *sib_buf.buf(), key_to_replace, replacement_key, parent_node);

            if (leveled) {
                internal_node::update_key(*last_buf.buf(), key_to_replace, replacement_key);
            }
        }
    }
}

// Get a root block given a superblock, or make a new root if there isn't one.
void get_root(value_sizer_t *sizer, transaction_t *txn, buf_lock_t& sb_buf, buf_lock_t *buf_out, repli_timestamp timestamp) {
    rassert(!buf_out->is_acquired());

    block_id_t node_id = reinterpret_cast<const btree_superblock_t*>(sb_buf->get_data_read())->root_block;

    if (node_id != NULL_BLOCK_ID) {
        buf_lock_t tmp(txn, node_id, rwi_write);
        buf_out->swap(tmp);
    } else {
        buf_out->allocate(txn);
        leaf::init(sizer, *buf_out->buf(), timestamp);
        insert_root(buf_out->buf()->get_block_id(), sb_buf);
    }
}





// Runs a btree_modify_oper_t.
void run_btree_modify_oper(value_sizer_t *sizer, btree_modify_oper_t *oper, btree_slice_t *slice, const store_key_t &store_key, castime_t castime, order_token_t token) {
    btree_key_buffer_t kbuffer(store_key);
    btree_key_t *key = kbuffer.key();

    block_size_t block_size = slice->cache()->get_block_size();

    {
        got_superblock_t got_superblock;

        get_btree_superblock(slice, rwi_write, oper->compute_expected_change_count(block_size), castime.timestamp, token, &got_superblock);

        // TODO: do_superblock_sidequest is blocking.  It doesn't have
        // to be, but when you fix this, make sure the superblock
        // sidequest is done using the superblock before the
        // superblock gets released.
        oper->do_superblock_sidequest(got_superblock.txn.get(), got_superblock.sb_buf, castime.timestamp, &store_key);

        keyvalue_location_t kv_location;
        find_keyvalue_location_for_write(sizer, &got_superblock, key, castime.timestamp, &kv_location);
        transaction_t *txn = kv_location.txn.get();

        bool key_found = kv_location.value;

        bool expired = key_found && kv_location.value.as<btree_value_t>()->expired();

        // If the value's expired, delete it.
        if (expired) {
            blob_t b(kv_location.value.as<btree_value_t>()->value_ref(), blob::btree_maxreflen);
            b.unappend_region(txn, b.valuesize());
            kv_location.value.reset();
        }

        bool update_needed = oper->operate(txn, kv_location.value);
        update_needed = update_needed || expired;

        // Add a CAS to the value if necessary
        if (kv_location.value) {
            if (kv_location.value.as<btree_value_t>()->has_cas()) {
                rassert(castime.proposed_cas != BTREE_MODIFY_OPER_DUMMY_PROPOSED_CAS);
                kv_location.value.as<btree_value_t>()->set_cas(block_size, castime.proposed_cas);
            }
        }

        // Actually update the leaf, if needed.
        if (update_needed) {
            if (kv_location.value) { // We have a value to insert.
                // Split the node if necessary, to make sure that we have room
                // for the value; This isn't necessary when we're deleting,
                // because the node isn't going to grow.
                check_and_handle_split(sizer, txn, kv_location.buf, kv_location.last_buf, kv_location.sb_buf, key, kv_location.value.get(), block_size);

                repli_timestamp new_value_timestamp = castime.timestamp;

                bool success = leaf::insert(sizer, *kv_location.buf.buf(), key, kv_location.value.get(), new_value_timestamp);
                guarantee(success, "could not insert leaf btree node");
            } else { // Delete the value if it's there.
                if (key_found) {
                    leaf::remove(sizer, *kv_location.buf.buf(), key);
                }
            }

            // Check to see if the leaf is underfull (following a change in
            // size or a deletion), and merge/level if it is.
            check_and_handle_underfull(txn, kv_location.buf, kv_location.last_buf, kv_location.sb_buf, key, block_size);
        }
    }
}
