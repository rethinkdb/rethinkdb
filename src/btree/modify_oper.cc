#include "btree/modify_oper.hpp"
#include "utils.hpp"
#include "buffer_cache/large_buf.hpp"
#include "btree/leaf_node.hpp"
#include "btree/internal_node.hpp"
#include "btree/replicate.hpp"

#include "buffer_cache/co_functions.hpp"
#include "buffer_cache/transactor.hpp"
#include "btree/coro_wrappers.hpp"

// TODO: consider B#/B* trees to improve space efficiency

// TODO: perhaps allow memory reclamation due to oversplitting? We can
// be smart and only use a limited amount of ram for incomplete nodes
// (doing this efficiently very tricky for high insert
// workloads). Also, if the serializer is log-structured, we can write
// only a small part of each node.

// TODO: change rwi_write to rwi_intent followed by rwi_upgrade where
// relevant.

perfmon_counter_t pm_btree_depth("btree_depth");

void insert_root(block_id_t root_id, buf_t **sb_buf) {
    rassert(*sb_buf);
    ptr_cast<btree_superblock_t>((*sb_buf)->get_data_write())->root_block = root_id;
    (*sb_buf)->release();
    *sb_buf = NULL;
}

// Split the node if necessary. If the node is a leaf_node, provide the new
// value that will be inserted; if it's an internal node, provide NULL (we
// split internal nodes proactively).
void check_and_handle_split(transaction_t *txn, buf_t **buf, buf_t **last_buf, buf_t **sb_buf,
                                                const btree_key *key, btree_value *new_value, block_size_t block_size) {
    const node_t *node = ptr_cast<node_t>((*buf)->get_data_read());

    // If the node isn't full, we don't need to split, so we're done.
    if (node::is_leaf(node)) { // This should only be called when update_needed.
        rassert(new_value);
        if (!leaf::is_full(ptr_cast<leaf_node_t>(node), key, new_value)) return;
    } else {
        rassert(!new_value);
        if (!internal_node::is_full(ptr_cast<internal_node_t>(node))) return;
    }

    // Allocate a new node to split into, and some temporary memory to keep
    // track of the median key in the split; then actually split.
    buf_t *rbuf = txn->allocate();
    byte median_memory[sizeof(btree_key) + MAX_KEY_SIZE];
    btree_key *median = reinterpret_cast<btree_key *>(median_memory);

    node::split(block_size, ptr_cast<node_t>((*buf)->get_data_write()), ptr_cast<node_t>(rbuf->get_data_write()), median);

    // Insert the key that sets the two nodes apart into the parent.
    internal_node_t *last_node;
    if (*last_buf == NULL) { // We're splitting what was previously the root, so create a new root to use as the parent.
        *last_buf = txn->allocate();
        last_node = ptr_cast<internal_node_t>((*last_buf)->get_data_write());
        internal_node::init(block_size, last_node);

        insert_root((*last_buf)->get_block_id(), sb_buf);
        pm_btree_depth++;
    } else {
        last_node = ptr_cast<internal_node_t>((*last_buf)->get_data_write());
    }

    bool success __attribute__((unused)) = internal_node::insert(block_size, last_node, median, (*buf)->get_block_id(), rbuf->get_block_id());
    rassert(success, "could not insert internal btree node");

    // We've split the node; now figure out where the key goes and release the other buf (since we're done with it).
    if (sized_strcmp(key->contents, key->size, median->contents, median->size) <= 0) {
        // The key goes in the old buf (the left one).
        rbuf->release();
    } else {
        // The key goes in the new buf (the right one).
        (*buf)->release();
        *buf = rbuf;
    }
}

// Merge or level the node if necessary.
void check_and_handle_underfull(transaction_t *txn, buf_t **buf, buf_t **last_buf, buf_t **sb_buf,
                                                    const btree_key *key, block_size_t block_size) {
    const node_t *node = ptr_cast<node_t>((*buf)->get_data_read());
    if (*last_buf && node::is_underfull(block_size, node)) { // The root node is never underfull.

        node_t *node = ptr_cast<node_t>((*buf)->get_data_write());
        internal_node_t *parent_node = ptr_cast<internal_node_t>((*last_buf)->get_data_write());

        // Acquire a sibling to merge or level with.
        block_id_t sib_node_id;
        int nodecmp_node_with_sib = internal_node::sibling(parent_node, key, &sib_node_id);

        // Now decide whether to merge or level.
        buf_t *sib_buf = co_acquire_block(txn, sib_node_id, rwi_write);
        node_t *sib_node = ptr_cast<node_t>(sib_buf->get_data_write());

#ifndef NDEBUG
        node::validate(block_size, sib_node);
#endif

        if (node::is_mergable(block_size, node, sib_node, parent_node)) { // Merge.

            // This is the key that we remove.
            char key_to_remove_buf[sizeof(btree_key) + MAX_KEY_SIZE];
            btree_key *key_to_remove = ptr_cast<btree_key>(key_to_remove_buf);

            if (nodecmp_node_with_sib < 0) { // Nodes must be passed to merge in ascending order.
                node::merge(block_size, node, sib_node, key_to_remove, parent_node);
                (*buf)->mark_deleted();
                (*buf)->release();
                *buf = sib_buf;
            } else {
                node::merge(block_size, sib_node, node, key_to_remove, parent_node);
                sib_buf->mark_deleted();
                sib_buf->release();
            }

            if (!internal_node::is_singleton(parent_node)) {
                internal_node::remove(block_size, parent_node, key_to_remove);
            } else {
                // The parent has only 1 key after the merge (which means that
                // it's the root and our node is its only child). Insert our
                // node as the new root.
                (*last_buf)->mark_deleted();
                insert_root((*buf)->get_block_id(), sb_buf);
                pm_btree_depth--;
            }
        } else { // Level
            byte key_to_replace_buf[sizeof(btree_key) + MAX_KEY_SIZE];
            btree_key *key_to_replace = ptr_cast<btree_key>(key_to_replace_buf);
            byte replacement_key_buf[sizeof(btree_key) + MAX_KEY_SIZE];
            btree_key *replacement_key = ptr_cast<btree_key>(replacement_key_buf);

            bool leveled = node::level(block_size, node, sib_node, key_to_replace, replacement_key, parent_node);

            if (leveled) {
                internal_node::update_key(parent_node, key_to_replace, replacement_key);
            }
            sib_buf->release();
        }
    }
}

// Get a root block given a superblock, or make a new root if there isn't one.
buf_t *get_root(transaction_t *txn, buf_t **sb_buf, block_size_t block_size) {
    block_id_t node_id = reinterpret_cast<const btree_superblock_t*>((*sb_buf)->get_data_read())->root_block;

    if (node_id != NULL_BLOCK_ID) {
        return co_acquire_block(txn, node_id, rwi_write);
    } else { // Make a new block.
        buf_t *buf = txn->allocate();
        leaf::init(block_size, ptr_cast<leaf_node_t>(buf->get_data_write()), current_time());
        insert_root(buf->get_block_id(), sb_buf);
        pm_btree_depth++;
        return buf;
    }
}

// Runs a btree_modify_oper_t.
void run_btree_modify_oper(btree_modify_oper_t *oper, btree_key_value_store_t *store, const btree_key *key) {
    union {
        byte old_value_memory[MAX_BTREE_VALUE_SIZE];
        btree_value old_value;
    };
    (void) old_value_memory;

    btree_slice_t *slice = store->slice_for_key(key);
    oper->slice = slice; // TODO: Figure out a way to do this more nicely -- it's only used for generating a CAS value.
    block_size_t block_size = slice->cache.get_block_size();

    store->started_a_query(); // TODO: This should probably use RAII too.

    {
        on_thread_t mover(slice->home_thread); // Move to the slice's thread.
        transactor_t transactor(&slice->cache, rwi_write);
        transaction_t *txn = transactor.transaction();

        // Acquire the superblock.
        buf_t *sb_buf = co_acquire_block(txn, SUPERBLOCK_ID, rwi_write);
        buf_t *last_buf = NULL;
        buf_t *buf = get_root(txn, &sb_buf, block_size);

        // Walk down the tree to the leaf.
        while (node::is_internal(ptr_cast<node_t>(buf->get_data_read()))) {
            // Check if the node is overfull and proactively split it if it is (since this is an internal node).
            check_and_handle_split(txn, &buf, &last_buf, &sb_buf, key, NULL, block_size);
            // Check if the node is underfull, and merge/level if it is.
            check_and_handle_underfull(txn, &buf, &last_buf, &sb_buf, key, block_size);

            // Release the superblock, if we've gone past the root (and haven't
            // already released it). If we're still at the root or at one of
            // its direct children, we might still want to replace the root, so
            // we can't release the superblock yet.
            if (sb_buf && last_buf) {
                sb_buf->release();
                sb_buf = NULL;
            }

            // Release the old previous node (unless we're at the root), and set
            // the next previous node (which is the current node).
            if (last_buf) last_buf->release();
            last_buf = buf;

            // Look up and acquire the next node.
            block_id_t node_id = internal_node::lookup(ptr_cast<internal_node_t>(buf->get_data_read()), key);
            rassert(node_id != NULL_BLOCK_ID && node_id != SUPERBLOCK_ID);
            buf = co_acquire_block(txn, node_id, rwi_write);
        }

        // We've gone down the tree and gotten to a leaf. Now look up the key.
        bool key_found = leaf::lookup(ptr_cast<leaf_node_t>(buf->get_data_read()), key, &old_value);

        // If there's a large value, acquire that too.
        large_buf_t *old_large_buf = NULL;
        if (key_found && old_value.is_large()) {
            old_large_buf = new large_buf_t(txn);
            // We don't know whether we want to acquire all of the large value or
            // just part of it, so we let the oper acquire it for us.
            oper->actually_acquire_large_value(old_large_buf, old_value.lb_ref());
            rassert(old_large_buf->state == large_buf_t::loaded);
        }

        // Check whether the value is expired. If it is, we tell operate() that
        // the value wasn't found. Then, if it tells us to make a change, we'll
        // replace/delete the value as usual; if it tells us to do nothing,
        // we'll silently delete the key.
        bool expired = key_found && old_value.expired();
        if (expired) key_found = false;

        // Now we actually run the operation to compute the new value.
        btree_value *new_value;
        large_buf_t *new_large_buf;
        bool update_needed = oper->operate(txn, key_found ? &old_value : NULL, old_large_buf, &new_value, &new_large_buf);

        // Make sure that the new_value and new_large_buf returned by operate() are consistent.
        if (update_needed) {
            if (new_value && new_value->is_large()) {
                rassert(new_large_buf && new_value->lb_ref().block_id == new_large_buf->get_root_ref().block_id);
            } else {
                rassert(!new_large_buf);
            }
        }

        // If the value is expired and operate() decided not to make any
        // change, we'll silently delete the key.
        if (!update_needed && expired) {
            new_value = NULL;
            new_large_buf = NULL;
            update_needed = true;
        }

        // Actually update the leaf, if needed.
        if (update_needed) {
            // TODO make sure we're updating leaf node timestamps.
            if (new_value) { // We have a value to insert.
                // Split the node if necessary, to make sure that we have room
                // for the value; This isn't necessary when we're deleting,
                // because the node isn't going to grow.
                check_and_handle_split(txn, &buf, &last_buf, &sb_buf, key, new_value, block_size);

                // Add a CAS to the value if necessary (this won't change its size).
                if (new_value->has_cas() && !oper->cas_already_set) {
                    new_value->set_cas(slice->gen_cas());
                }

                repli_timestamp new_value_timestamp = current_time(); // TODO: When the replication code is put back in this'll probably need to be changed.

                bool success = leaf::insert(block_size, ptr_cast<leaf_node_t>(buf->get_data_write()), key, new_value, new_value_timestamp);
                guarantee(success, "could not insert leaf btree node");
            } else { // Delete the value if it's there.
                if (key_found || expired) {
                    leaf::remove(block_size, ptr_cast<leaf_node_t>(buf->get_data_write()), key);
                } else {
                     // operate() told us to delete a value (update_needed && !new_value), but the
                     // key wasn't in the node (!key_found && !expired), so we do nothing.
                }
            }

            // XXX: Previously this was checked whether or not update_needed,
            // but I'm pretty sure a leaf node can only be underfull
            // immediately following a split or an update. Double check this.

            // Check to see if the leaf is underfull (following a change in
            // size or a deletion), and merge/level if it is.
            check_and_handle_underfull(txn, &buf, &last_buf, &sb_buf, key, block_size);
        }

        // Release bufs as necessary.
        if (sb_buf) sb_buf->release();
        rassert(buf);
        buf->release();
        if (last_buf) last_buf->release();
        buf = last_buf = sb_buf = NULL;

        // Release all the large bufs that we used.
        if (update_needed) {
            if (old_large_buf && new_large_buf != old_large_buf) {
                // operate() switched to a new large buf, so we need to delete the old one.
                rassert(old_value.is_large());
                rassert(old_value.lb_ref().block_id == old_large_buf->get_root_ref().block_id);
                old_large_buf->mark_deleted();
                old_large_buf->release();
                delete old_large_buf;
            }
            if (new_large_buf) {
                new_large_buf->release();
                delete new_large_buf;
            }
        } else {
            if (old_large_buf) {
                old_large_buf->release();
                delete old_large_buf;
            }
        }

        // Committing the transaction and moving back to the home thread are
        // handled automatically with RAII.
    }

    // Report the result of the operation and delete the oper.
    oper->call_callback();
    delete oper;

    // We're done!
    store->finished_a_query();
}
