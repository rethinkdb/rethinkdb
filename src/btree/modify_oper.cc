#include <boost/shared_ptr.hpp>

#include "btree/modify_oper.hpp"

#include "utils.hpp"
#include "buffer_cache/buf_lock.hpp"
#include "buffer_cache/co_functions.hpp"
#include "buffer_cache/large_buf.hpp"
#include "buffer_cache/large_buf_lock.hpp"
#include "buffer_cache/transactor.hpp"
#include "btree/leaf_node.hpp"
#include "btree/internal_node.hpp"

#include "buffer_cache/co_functions.hpp"
#include "buffer_cache/transactor.hpp"

// TODO: consider B#/B* trees to improve space efficiency

// TODO: perhaps allow memory reclamation due to oversplitting? We can
// be smart and only use a limited amount of ram for incomplete nodes
// (doing this efficiently very tricky for high insert
// workloads). Also, if the serializer is log-structured, we can write
// only a small part of each node.

// TODO: change rwi_write to rwi_intent followed by rwi_upgrade where
// relevant.

perfmon_counter_t pm_btree_depth("btree_depth");

void insert_root(block_id_t root_id, buf_lock_t& sb_buf) {
    rassert(sb_buf.is_acquired());
    sb_buf.buf()->set_data(&ptr_cast<btree_superblock_t>(sb_buf.buf()->get_data_read())->root_block, &root_id, sizeof(root_id));

    sb_buf.release();
}

// Split the node if necessary. If the node is a leaf_node, provide the new
// value that will be inserted; if it's an internal node, provide NULL (we
// split internal nodes proactively).
void check_and_handle_split(transactor_t& txor, buf_lock_t& buf, buf_lock_t& last_buf, buf_lock_t& sb_buf,
                                                const btree_key_t *key, btree_value *new_value, block_size_t block_size) {
    const node_t *node = ptr_cast<node_t>(buf.buf()->get_data_read());

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
    buf_lock_t rbuf;
    rbuf.allocate(txor);
    btree_key_buffer_t median_buffer;
    btree_key_t *median = median_buffer.key();

    node::split(block_size, *buf.buf(), *rbuf.buf(), median);

    // Insert the key that sets the two nodes apart into the parent.
    if (!last_buf.is_acquired()) {
        // We're splitting what was previously the root, so create a new root to use as the parent.
        last_buf.allocate(txor);
        internal_node::init(block_size, *last_buf.buf());

        insert_root(last_buf.buf()->get_block_id(), sb_buf);
        pm_btree_depth++;
    }

    bool success __attribute__((unused)) = internal_node::insert(block_size, *last_buf.buf(), median, buf.buf()->get_block_id(), rbuf.buf()->get_block_id());
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
void check_and_handle_underfull(transactor_t& txor, buf_lock_t& buf, buf_lock_t& last_buf, buf_lock_t& sb_buf,
                                                    const btree_key_t *key, block_size_t block_size) {
    const node_t *node = ptr_cast<node_t>(buf.buf()->get_data_read());
    if (last_buf.is_acquired() && node::is_underfull(block_size, node)) { // The root node is never underfull.

        const internal_node_t *parent_node = ptr_cast<internal_node_t>(last_buf.buf()->get_data_read());

        // Acquire a sibling to merge or level with.
        block_id_t sib_node_id;
        int nodecmp_node_with_sib = internal_node::sibling(parent_node, key, &sib_node_id);

        // Now decide whether to merge or level.
        buf_lock_t sib_buf(txor, sib_node_id, rwi_write);
        const node_t *sib_node = ptr_cast<node_t>(sib_buf.buf()->get_data_read());

#ifndef NDEBUG
        node::validate(block_size, sib_node);
#endif

        if (node::is_mergable(block_size, node, sib_node, parent_node)) { // Merge.

            // This is the key that we remove.
            btree_key_buffer_t key_to_remove_buffer;
            btree_key_t *key_to_remove = key_to_remove_buffer.key();

            if (nodecmp_node_with_sib < 0) { // Nodes must be passed to merge in ascending order.
                node::merge(block_size, node, *sib_buf.buf(), key_to_remove, parent_node);
                buf.buf()->mark_deleted();
                buf.swap(sib_buf);
            } else {
                node::merge(block_size, sib_node, *buf.buf(), key_to_remove, parent_node);
                sib_buf.buf()->mark_deleted();
            }

            sib_buf.release();

            if (!internal_node::is_singleton(parent_node)) {
                internal_node::remove(block_size, *last_buf.buf(), key_to_remove);
            } else {
                // The parent has only 1 key after the merge (which means that
                // it's the root and our node is its only child). Insert our
                // node as the new root.
                last_buf.buf()->mark_deleted();
                insert_root(buf.buf()->get_block_id(), sb_buf);
                pm_btree_depth--;
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
void get_root(transactor_t& txor, buf_lock_t& sb_buf, block_size_t block_size, buf_lock_t *buf_out, repli_timestamp timestamp) {
    rassert(!buf_out->is_acquired());

    block_id_t node_id = reinterpret_cast<const btree_superblock_t*>(sb_buf.buf()->get_data_read())->root_block;

    if (node_id != NULL_BLOCK_ID) {
        buf_lock_t tmp(txor, node_id, rwi_write);
        buf_out->swap(tmp);
    } else {
        buf_out->allocate(txor);
        leaf::init(block_size, *buf_out->buf(), timestamp);
        insert_root(buf_out->buf()->get_block_id(), sb_buf);
        pm_btree_depth++;
    }
}

// Runs a btree_modify_oper_t.
void run_btree_modify_oper(btree_modify_oper_t *oper, btree_slice_t *slice, const store_key_t &store_key, castime_t castime) {
    union {
        byte old_value_memory[MAX_BTREE_VALUE_SIZE];
        btree_value old_value;
    };
    (void) old_value_memory;

    btree_key_buffer_t kbuffer(store_key);
    btree_key_t *key = kbuffer.key();

    oper->slice = slice; // TODO: Figure out a way to do this more nicely -- it's only used for generating a CAS value.
    block_size_t block_size = slice->cache().get_block_size();

    {
        // temporary sanity-check
        rassert(get_thread_id() == slice->home_thread);
        on_thread_t mover(slice->home_thread); // Move to the slice's thread.
        boost::shared_ptr<transactor_t> txor(new transactor_t(&slice->cache(), rwi_write));

        buf_lock_t sb_buf(*txor, SUPERBLOCK_ID, rwi_write);
        buf_lock_t last_buf;
        buf_lock_t buf;
        get_root(*txor, sb_buf, block_size, &buf, castime.timestamp);

        // Walk down the tree to the leaf.
        while (node::is_internal(ptr_cast<node_t>(buf.buf()->get_data_read()))) {
            // Check if the node is overfull and proactively split it if it is (since this is an internal node).
            check_and_handle_split(*txor, buf, last_buf, sb_buf, key, NULL, block_size);
            // Check if the node is underfull, and merge/level if it is.
            check_and_handle_underfull(*txor, buf, last_buf, sb_buf, key, block_size);

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
            block_id_t node_id = internal_node::lookup(ptr_cast<internal_node_t>(buf.buf()->get_data_read()), key);
            rassert(node_id != NULL_BLOCK_ID && node_id != SUPERBLOCK_ID);

            buf_lock_t tmp(*txor, node_id, rwi_write);
            last_buf.swap(tmp);
            buf.swap(last_buf);
        }

        // We've gone down the tree and gotten to a leaf. Now look up the key.
        bool key_found = leaf::lookup(ptr_cast<leaf_node_t>(buf.buf()->get_data_read()), key, &old_value);

        // If there's a large value, acquire that too.
        large_buf_lock_t old_large_buflock;

        if (key_found && old_value.is_large()) {
            old_large_buflock.set(new large_buf_t(txor->transaction()));
            // We don't know whether we want to acquire all of the large value or
            // just part of it, so we let the oper acquire it for us.
            oper->actually_acquire_large_value(old_large_buflock.lv(), old_value.lb_ref());
            rassert(old_large_buflock.lv()->state == large_buf_t::loaded);
        }

        // Check whether the value is expired. If it is, we tell operate() that
        // the value wasn't found. Then, if it tells us to make a change, we'll
        // replace/delete the value as usual; if it tells us to do nothing,
        // we'll silently delete the key.
        bool expired = key_found && old_value.expired();
        if (expired) key_found = false;

        // Now we actually run the operation to compute the new value.
        btree_value *new_value;
        large_buf_lock_t new_large_buflock;
        bool update_needed = oper->operate(txor, key_found ? &old_value : NULL, old_large_buflock, &new_value, new_large_buflock);

        // Make sure that the new_value and new_large_buf returned by operate() are consistent.
        if (update_needed) {
            if (new_value && new_value->is_large()) {
                rassert(new_large_buflock.has_lv() && new_value->lb_ref().block_id() == new_large_buflock.lv()->get_root_ref().block_id());
            } else {
                rassert(!new_large_buflock.has_lv());
            }
        }

#ifndef NDEBUG
        if (!update_needed) {
            rassert(!new_large_buflock.has_lv());
        }
#endif

        // If the value is expired and operate() decided not to make any
        // change, we'll silently delete the key.
        if (!update_needed && expired) {
            new_value = NULL;
            update_needed = true;
        }

        // Actually update the leaf, if needed.
        if (update_needed) {
            // TODO make sure we're updating leaf node timestamps.
            if (new_value) { // We have a value to insert.
                // Split the node if necessary, to make sure that we have room
                // for the value; This isn't necessary when we're deleting,
                // because the node isn't going to grow.
                check_and_handle_split(*txor, buf, last_buf, sb_buf, key, new_value, block_size);

                // Add a CAS to the value if necessary (this won't change its size).
                if (new_value->has_cas() && !oper->cas_already_set) {
                    rassert(castime.proposed_cas != BTREE_MODIFY_OPER_DUMMY_PROPOSED_CAS);
                    new_value->set_cas(castime.proposed_cas);
                }

                repli_timestamp new_value_timestamp = castime.timestamp;

                bool success = leaf::insert(block_size, *buf.buf(), key, new_value, new_value_timestamp);
                guarantee(success, "could not insert leaf btree node");
            } else { // Delete the value if it's there.
                if (key_found || expired) {
                    leaf::remove(block_size, *buf.buf(), key);
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
            check_and_handle_underfull(*txor, buf, last_buf, sb_buf, key, block_size);
        }

        // Release bufs as necessary.
        sb_buf.release_if_acquired();
        rassert(buf.is_acquired());
        buf.release();
        last_buf.release_if_acquired();

        // Release all the large bufs that we used.
        if (update_needed) {
            if (old_large_buflock.has_lv() && new_large_buflock.lv() != old_large_buflock.lv()) {
                // operate() switched to a new large buf, so we need to delete the old one.
                rassert(old_value.is_large());
                rassert(old_value.lb_ref().block_id() == old_large_buflock.lv()->get_root_ref().block_id());
                old_large_buflock.lv()->mark_deleted();
            }
        }

        // Committing the transaction and moving back to the home thread are
        // handled automatically with RAII.
    }
}
