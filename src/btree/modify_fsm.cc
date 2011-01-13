#include "btree/modify_fsm.hpp"
#include "utils.hpp"
#include "buffer_cache/large_buf.hpp"
#include "btree/leaf_node.hpp"
#include "btree/internal_node.hpp"
#include "btree/replicate.hpp"

#include "buffer_cache/co_functions.hpp"
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

void btree_modify_fsm_t::insert_root(block_id_t root_id, buf_t **sb_buf) {
    assert(sb_buf);
    reinterpret_cast<btree_superblock_t *>((*sb_buf)->get_data_write())->root_block = root_id;
    (*sb_buf)->release();
    *sb_buf = NULL;
}

void btree_modify_fsm_t::check_and_handle_split(const node_t **node, const btree_key *key, buf_t **sb_buf, btree_value *new_value, block_size_t block_size) {
    // Split the node if necessary
    bool full;
    if (node_handler::is_leaf(*node)) { // This should only be called when update_needed.
        assert(new_value);
        full = leaf_node_handler::is_full(ptr_cast<leaf_node_t>(*node), key, new_value);
    } else {
        assert(!new_value);
        full = internal_node_handler::is_full(ptr_cast<internal_node_t>(*node));
    }

    if (full) {
        char memory[sizeof(btree_key) + MAX_KEY_SIZE];
        btree_key *median = reinterpret_cast<btree_key *>(memory);
        buf_t *rbuf;
        internal_node_t *last_node;
        split_node(buf, &rbuf, median, block_size);
        if (last_buf == NULL) { // We're splitting a root, so create a new root.
            last_buf = transaction->allocate();
            last_node = ptr_cast<internal_node_t>(last_buf->get_data_write());
            internal_node_handler::init(block_size, last_node);

            insert_root(last_buf->get_block_id(), sb_buf);
            pm_btree_depth++;
        } else {
            last_node = ptr_cast<internal_node_t>(last_buf->get_data_write());
        }

        bool success __attribute__((unused)) = internal_node_handler::insert(block_size, last_node, median, buf->get_block_id(), rbuf->get_block_id());
        assert(success, "could not insert internal btree node");

        // Figure out where the key goes
        if (sized_strcmp(key->contents, key->size, median->contents, median->size) <= 0) {
            // Left node and node are the same thing
            rbuf->release();
        } else {
            buf->release();
            buf = rbuf;
            *node = ptr_cast<node_t>(buf->get_data_read());
        }
    }
}

void btree_modify_fsm_t::actually_acquire_large_value(large_buf_t *lb, const large_buf_ref& lbref) {
    co_acquire_large_value(lb, lbref, rwi_write);
}

void btree_modify_fsm_t::check_and_handle_underfull(const node_t **node, const btree_key *key, buf_t **sb_buf, block_size_t block_size) {
    if (last_buf && node_handler::is_underfull(block_size, *node)) { // the root node is never underfull
        node_t *parent_node = ptr_cast<node_t>(last_buf->get_data_write());

        // Acquire a sibling to merge or level with
        internal_node_t *last_node = ptr_cast<internal_node_t>(parent_node);

        block_id_t sib_node_id;
        internal_node_handler::sibling(last_node, key, &sib_node_id); // TODO: This returns nodecmp(node, sib), but we compare them again when we merge.

        buf_t *sib_buf = co_acquire_block(transaction, sib_node_id, rwi_write);

        // Sibling acquired, now decide whether to merge or level
        node_t *sib_node = ptr_cast<node_t>(sib_buf->get_data_write());

#ifndef NDEBUG
        node_handler::validate(block_size, sib_node);
#endif

        if (node_handler::is_mergable(block_size, *node, sib_node, parent_node)) { // Merge
            char key_to_remove_buf[sizeof(btree_key) + MAX_KEY_SIZE];
            btree_key *key_to_remove = ptr_cast<btree_key>(key_to_remove_buf);

            if (node_handler::nodecmp(*node, sib_node) < 0) { // Nodes must be passed to merge in ascending order
                node_handler::merge(block_size, ptr_cast<node_t>(buf->get_data_write()), sib_node, key_to_remove, parent_node);
                buf->mark_deleted();
                buf->release();
                buf = sib_buf;
                *node = sib_node;
            } else {
                node_handler::merge(block_size, sib_node, ptr_cast<node_t>(buf->get_data_write()), key_to_remove, parent_node);
                sib_buf->mark_deleted();
                sib_buf->release();
            }

            if (!internal_node_handler::is_singleton(ptr_cast<internal_node_t>(parent_node))) {
                internal_node_handler::remove(block_size, ptr_cast<internal_node_t>(parent_node), key_to_remove);
            } else {
                // parent has only 1 key (which means it is also the root), replace it with the node
                // when we get here node_id should be the id of the new root
                last_buf->mark_deleted();
                insert_root(buf->get_block_id(), sb_buf);
                pm_btree_depth--;
            }
        } else { // Level
            char key_to_replace_buf[sizeof(btree_key) + MAX_KEY_SIZE];
            btree_key *key_to_replace = ptr_cast<btree_key>(key_to_replace_buf);
            char replacement_key_buf[sizeof(btree_key) + MAX_KEY_SIZE];
            btree_key *replacement_key = ptr_cast<btree_key>(replacement_key_buf);
            bool leveled = node_handler::level(block_size,
                                               ptr_cast<node_t>(buf->get_data_write()),
                                               sib_node, key_to_replace, replacement_key, parent_node);

            if (leveled) {
                internal_node_handler::update_key(ptr_cast<internal_node_t>(parent_node), key_to_replace, replacement_key);
            }
            sib_buf->release();
        }
    }
}

buf_t *btree_modify_fsm_t::get_root(buf_t **sb_buf, block_size_t block_size) {
    block_id_t node_id = reinterpret_cast<const btree_superblock_t*>((*sb_buf)->get_data_read())->root_block;

    if (node_id != NULL_BLOCK_ID) {
        return co_acquire_block(transaction, node_id, rwi_write);
    } else { // Make a new block.
        buf_t *buf = transaction->allocate();
        leaf_node_handler::init(block_size, ptr_cast<leaf_node_t>(buf->get_data_write()), current_time());
        insert_root(buf->get_block_id(), sb_buf);
        pm_btree_depth++;
        return buf;
    }
}

void btree_modify_fsm_t::run(btree_key_value_store_t *store, btree_key *_key) {
    // TODO: Use RAII for these.

    union {
        byte old_value_memory[MAX_TOTAL_NODE_CONTENTS_SIZE+sizeof(btree_value)];
        btree_value old_value;
    };
    (void) old_value_memory;

    union {
        byte key_memory[MAX_KEY_SIZE + sizeof(btree_key)];
        btree_key key;
    };
    (void) key_memory;

    keycpy(&key, _key);

    slice = store->slice_for_key(&key); // XXX
    cache_t *cache = &slice->cache;
    block_size_t block_size = cache->get_block_size();

    store->started_a_query(); // TODO: This should use RAII too.

    coro_t::move_to_thread(slice->home_thread);
    transaction = co_begin_transaction(cache, rwi_write);
    buf_t *sb_buf = co_acquire_block(transaction, SUPERBLOCK_ID, rwi_write);

    buf = get_root(&sb_buf, block_size);

    repli_timestamp new_value_timestamp = repli_timestamp::invalid;

    const node_t *node;

    node = ptr_cast<node_t>(buf->get_data_read());

    while (node_handler::is_internal(node)) {

        // Check if it's overfull. If so we would need to do proactive split (since this is an internal node).
        check_and_handle_split(&node, &key, &sb_buf, NULL, block_size);

        // Check to see if it's underfull, and merge/level if it is.
        check_and_handle_underfull(&node, &key, &sb_buf, block_size);

        // Release the superblock, if we've gone past the root (and haven't already).
        if (sb_buf && last_buf) {
            sb_buf->release();
            sb_buf = NULL;
        }

        // Release the old previous node, and set the new previous node.
        if (last_buf) {
            last_buf->release();
        }
        last_buf = buf;

        // Look up the next node.
        block_id_t node_id = internal_node_handler::lookup(ptr_cast<internal_node_t>(node), &key);
        assert(node_id != NULL_BLOCK_ID && node_id != SUPERBLOCK_ID);

        buf = co_acquire_block(transaction, node_id, rwi_write);
        node = ptr_cast<node_t>(buf->get_data_read());
    }

    bool key_found = leaf_node_handler::lookup(ptr_cast<leaf_node_t>(node), &key, &old_value);

    large_buf_t *old_large_buf = NULL;
    if (key_found && old_value.is_large()) {
        old_large_buf = new large_buf_t(transaction);
        actually_acquire_large_value(old_large_buf, old_value.lb_ref());
        assert(old_large_buf->state == large_buf_t::loaded);
    }

    bool expired = key_found && old_value.expired();
    if (expired) {
        // We tell operate() that the key wasn't found. If it tells us to make
        // a change, we'll replace/delete the value as usual; if it tells us to
        // do nothing, we'll silently delete the key.
        key_found = false;
    }

    // We've found the old value, or determined that it is not present or
    // expired; now compute the new value.

    btree_value *new_value;
    large_buf_t *new_large_buf;

    bool update_needed = operate(key_found ? &old_value : NULL, old_large_buf, &new_value, &new_large_buf);

    if (update_needed) {
        if (new_value && new_value->is_large()) {
            assert(new_large_buf && new_value->lb_ref().block_id == new_large_buf->get_root_ref().block_id);
        } else {
            assert(!new_large_buf);
        }
    }

    if (!update_needed && expired) { // Silently delete the key.
        new_value = NULL;
        new_large_buf = NULL;
        update_needed = true;
    }

    if (update_needed && new_value != NULL) { // If we're deleting a value, there's no need to split.
        check_and_handle_split(&node, &key, &sb_buf, new_value, block_size);
    }

    // Update if operate() told us to.
    if (update_needed) {
        // TODO make sure we're updating leaf node timestamps.
       if (new_value) {
           if (new_value->has_cas() && !cas_already_set) {
               new_value->set_cas(slice->gen_cas());
           }
           repli_timestamp new_value_timestamp = current_time(); // TODO: When the replication code is put back in this'll probably need to be changed.
           bool success = leaf_node_handler::insert(block_size, ptr_cast<leaf_node_t>(buf->get_data_write()), &key, new_value, new_value_timestamp);
           guarantee(success, "could not insert leaf btree node");
       } else { // Delete
           if (key_found || expired) {
               leaf_node_handler::remove(block_size, ptr_cast<leaf_node_t>(buf->get_data_write()), &key);
           } else {
                // operate() told us to delete a value (update_needed && !new_value), but the
                // key wasn't in the node (!key_found && !expired), so we do nothing.
           }
       }
    }

    // Check to see if it's underfull, and merge/level if it is.
    check_and_handle_underfull(&node, &key, &sb_buf, block_size);

    // Release bufs as necessary.
    if (sb_buf) {
        sb_buf->release();
        sb_buf = NULL;
    }

    assert(buf);
    buf->release();
    buf = NULL;

    if (last_buf) {
        last_buf->release();
        last_buf = NULL;
    }

    if (update_needed) {
        if (old_large_buf && new_large_buf != old_large_buf) {
            assert(old_value.is_large());
            assert(old_value.lb_ref().block_id == old_large_buf->get_root_ref().block_id);
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

    co_commit_transaction(transaction);
    coro_t::move_to_thread(home_thread);

    call_callback_and_delete();

    store->finished_a_query();
}

// TODO: All these functions should move into node.hpp etc.; better yet, they should be abolished.
void btree_modify_fsm_t::split_node(buf_t *buf, buf_t **rbuf, btree_key *median, block_size_t block_size) {
    *rbuf = transaction->allocate();
    if(node_handler::is_leaf(ptr_cast<node_t>(buf->get_data_read()))) {
        leaf_node_t *node = ptr_cast<leaf_node_t>(buf->get_data_write());
        leaf_node_t *rnode = ptr_cast<leaf_node_t>((*rbuf)->get_data_write());
        leaf_node_handler::split(block_size, node, rnode, median);
    } else {
        internal_node_t *node = ptr_cast<internal_node_t>(buf->get_data_write());
        internal_node_t *rnode = ptr_cast<internal_node_t>((*rbuf)->get_data_write());
        internal_node_handler::split(block_size, node, rnode, median);
    }
}

