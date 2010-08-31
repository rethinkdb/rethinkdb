#ifndef __BTREE_MODIFY_FSM_TCC__
#define __BTREE_MODIFY_FSM_TCC__

#include "utils.hpp"
#include "cpu_context.hpp"

// TODO: consider B#/B* trees to improve space efficiency

// TODO: perhaps allow memory reclamation due to oversplitting? We can
// be smart and only use a limited amount of ram for incomplete nodes
// (doing this efficiently very tricky for high insert
// workloads). Also, if the serializer is log-structured, we can write
// only a small part of each node.

// TODO: change rwi_write to rwi_intent followed by rwi_upgrade where
// relevant.

template <class config_t>
typename btree_modify_fsm<config_t>::transition_result_t btree_modify_fsm<config_t>::do_start_transaction(event_t *event) {
    assert(state == start_transaction);

    /* Either start a new transaction or retrieve the one we started. */
    assert(transaction == NULL);
    if (event == NULL) {
        transaction = cache->begin_transaction(rwi_write, this);
    } else {
        assert(event->buf); // We shouldn't get a callback unless this is valid
        transaction = (typename config_t::transaction_t *)event->buf;
    }

    /* Determine our forward progress based on our new state. */
    if (transaction) {
        return btree_fsm_t::transition_ok;
    } else {
        return btree_fsm_t::transition_incomplete; // Flush lock is held.
    }
}

template <class config_t>
typename btree_modify_fsm<config_t>::transition_result_t btree_modify_fsm<config_t>::do_acquire_superblock(event_t *event)
{
    assert(state == acquire_superblock);

    if(event == NULL) {
        // First entry into the FSM; try to grab the superblock.
        sb_buf = transaction->acquire(SUPERBLOCK_ID, rwi_write, this);
    } else {
        // We already tried to grab the superblock, and we're getting
        // a cache notification about it.
        assert(event->buf);
        sb_buf = (buf_t *)event->buf;
    }

    if(sb_buf) {
        // Got the superblock buffer (either right away or through
        // cache notification). Grab the root id, and move on to
        // acquiring the root.
        node_id = ((btree_superblock_t*)sb_buf->ptr())->root_block;
        return btree_fsm_t::transition_ok;
    } else {
        // Can't get the superblock buffer right away. Let's wait for
        // the cache notification.
        return btree_fsm_t::transition_incomplete;
    }
}

template <class config_t>
typename btree_modify_fsm<config_t>::transition_result_t btree_modify_fsm<config_t>::do_acquire_root(event_t *event) {
    assert(state == acquire_root);

    // If there is no root, we make one.
    if(node_id == NULL_BLOCK_ID) {
        buf = transaction->allocate(&node_id);
        leaf_node_handler::init((leaf_node_t *)buf->ptr());
        buf->set_dirty();
        insert_root(node_id);
        return btree_fsm_t::transition_ok;
    }

    if(event == NULL) {
        // Acquire the actual root node
        buf = transaction->acquire(node_id, rwi_write, this);
    } else {
        // We already tried to acquire the root node, and here it is
        // via the cache notification.
        assert(event->buf);
        buf = (buf_t *)event->buf;
    }
    
    if(buf == NULL) {
        return btree_fsm_t::transition_incomplete;
    } else {
//        node_handler::validate((node_t *)buf->ptr());
        return btree_fsm_t::transition_ok;
    }
}

template <class config_t>
void btree_modify_fsm<config_t>::insert_root(block_id_t root_id) {
    if(!sb_buf)
        printf("failure\n");
    //assert(sb_buf);
    ((btree_superblock_t*)sb_buf->ptr())->root_block = root_id;
    sb_buf->set_dirty();
    sb_buf->release();
    sb_buf = NULL;
}

template <class config_t>
typename btree_modify_fsm<config_t>::transition_result_t btree_modify_fsm<config_t>::do_acquire_node(event_t *event) {
    assert(state == acquire_node);
    if(event == NULL) {
        buf = transaction->acquire(node_id, rwi_write, this);
    } else {
        assert(event->buf);
        buf = (buf_t*)event->buf;
    }

    if(buf) {
//        node_handler::validate((node_t *)buf->ptr());
        return btree_fsm_t::transition_ok;
    } else {
        return btree_fsm_t::transition_incomplete;
    }
}

template <class config_t>
typename btree_modify_fsm<config_t>::transition_result_t btree_modify_fsm<config_t>::do_acquire_sibling(event_t *event) {
    assert(state == acquire_sibling);

    if (!event) {
        assert(last_buf);
        internal_node_t *last_node = (internal_node_t *)last_buf->ptr();

        internal_node_handler::sibling(last_node, &key, &sib_node_id);
        sib_buf = transaction->acquire(sib_node_id, rwi_write, this);
    } else {
        assert(event->buf);
        sib_buf = (buf_t*) event->buf;
    }

    if (sib_buf) {
        return btree_fsm_t::transition_ok;
    } else {
        return btree_fsm_t::transition_incomplete;
    }
}

template <class config_t>
bool btree_modify_fsm<config_t>::do_check_for_split(node_t **node) {
    
    // Split the node if necessary
    bool new_root = false;
    bool full;
    if (node_handler::is_leaf(*node)) {
        if (update_needed && new_value == NULL) return false; // if we're deleting and it's a leaf node, there's no need to split
        full = update_needed && leaf_node_handler::is_full((leaf_node_t *)*node, &key, new_value);
    } else {
        full = internal_node_handler::is_full((internal_node_t *)*node);
    }
    if (full) {
#ifdef BTREE_DEBUG
        printf("===SPLITTING===\n");
        printf("Parent:\n");
        if (last_buf != NULL)
            internal_node_handler::print((internal_node_t *)last_buf->ptr());
        else
            printf("NULL parent\n");

        printf("node:\n");
        if (node_handler::is_leaf(*node))
            leaf_node_handler::print((leaf_node_t *) *node);
        else
            internal_node_handler::print((internal_node_t *) *node);
#endif
        char memory[sizeof(btree_key) + MAX_KEY_SIZE];
        btree_key *median = (btree_key *)memory;
        buf_t *rbuf;
        internal_node_t *last_node;
        block_id_t rnode_id;
        split_node(buf, &rbuf, &rnode_id, median);
        // Create a new root if we're splitting a root
        if(last_buf == NULL) {
            new_root = true;
            last_buf = transaction->allocate(&last_node_id);
            last_node = (internal_node_t *)last_buf->ptr();
            internal_node_handler::init(last_node);
            last_buf->set_dirty();
        } else {
            last_node = (internal_node_t *)last_buf->ptr();
        }
        
        bool success = internal_node_handler::insert(last_node, median, node_id, rnode_id);
        check("could not insert internal btree node", !success);
        last_buf->set_dirty();
     
#ifdef BTREE_DEBUG
        printf("\t|\n\t| Median = "); median->print(); printf("\n\t|\n\tV\n");
        printf("Parent:\n");
        internal_node_handler::print(last_node);

        printf("lnode:\n");
        if (node_handler::is_leaf(*node))
            leaf_node_handler::print((leaf_node_t *) *node);
        else
            internal_node_handler::print((internal_node_t *) *node);

        printf("rnode:\n");
        if (node_handler::is_leaf((node_t *) rbuf->ptr()))
            leaf_node_handler::print((leaf_node_t *) rbuf->ptr());
        else
            internal_node_handler::print((internal_node_t *) rbuf->ptr());
#endif
            
        // Figure out where the key goes
        if(sized_strcmp(key.contents, key.size, median->contents, median->size) <= 0) {
            // Left node and node are the same thing
            rbuf->release();
            rbuf = NULL;
        } else {
            buf->release();
            buf = rbuf;
            rbuf = NULL;
            *node = (node_t *)buf->ptr();
            node_id = rnode_id;
        }
    }
    return new_root;
}

template <class config_t>
typename btree_modify_fsm<config_t>::transition_result_t btree_modify_fsm<config_t>::do_transition(event_t *event)
{
    transition_result_t res = btree_fsm_t::transition_ok;

    // Make sure we've got either an empty or a cache event
    check("btree_fsm::do_transition - invalid event", event != NULL &&
          event->event_type != et_cache && event->event_type != et_commit);

    // Update the cache with the event
    if(event && event->event_type == et_cache) {
        check("btree_modify_fsm::do_transition - invalid event", event->op != eo_read);
        check("Could not complete AIO operation",
              event->result == 0 ||
              event->result == -1);
    }

    while (res == btree_fsm_t::transition_ok) {
        switch (state) {
            // First, begin a transaction.
            case start_transaction: {
                res = do_start_transaction(event);
                if (res == btree_fsm_t::transition_ok) state = acquire_superblock;
                break;
            }
            // Next, acquire the superblock (to get root node ID)
            case acquire_superblock: {
                assert(transaction); // We must have started our transaction by now.
                res = do_acquire_superblock(event);
                if (res == btree_fsm_t::transition_ok) state = acquire_root;
                break;
            }
            // Then, acquire the root block (or create it if it's missing)
            case acquire_root: {
                res = do_acquire_root(event);
                if (res == btree_fsm_t::transition_ok) state = acquire_node;
                break;
            }
            // If the previously acquired node was underfull, we acquire a sibling to level or merge
            case acquire_sibling: {
                res = do_acquire_sibling(event);
                if (res == btree_fsm_t::transition_ok) state = acquire_node;
                break;
            }
            // If we were acquiring a node, do that
            // Go up the tree...
            case acquire_node: {
                if(!buf) {
                    res = do_acquire_node(event);
                    if (res != btree_fsm_t::transition_ok) {
                        break;
                    }
                }

                node_t *node = (node_t *)buf->ptr();

                // STEP 1: Compute a new value.
                if (node_handler::is_leaf(node) && !have_computed_new_value) {
                    union {
                        char value_memory[MAX_TOTAL_NODE_CONTENTS_SIZE+sizeof(btree_value)];
                        btree_value old_value;
                    } u;

                    bool key_found = leaf_node_handler::lookup((btree_leaf_node*)node, &key, &u.old_value);

                    expired = key_found && u.old_value.expired();
                    if (expired) {
                        // We tell operate() that the key wasn't found. If it returns
                        // true, we'll replace/delete the value as usual; if it returns
                        // false, we'll silently delete the key.
                        key_found = false;
                    }

                    // We've found the old value, or determined that it is not
                    // present or expired; now compute the new value.
                    if (key_found) {
                        update_needed = operate(&u.old_value, &new_value);
                    } else {
                        update_needed = operate(NULL, &new_value);
                    }

                    if (!update_needed && expired) { // Silently delete the key.
                        new_value = NULL;
                        update_needed = true;
                    }

                    // by convention, new_value will be NULL if we're doing a delete.
                    have_computed_new_value = true;
                }

                // STEP 2: Check if it's overfull. If so we would need to do a split.
                if (update_needed || node_handler::is_internal(node)) { // Split internal nodes proactively.
                    bool new_root = do_check_for_split(&node);
                    if(new_root) {
                        insert_root(last_node_id);
                    }
                }

                // STEP 3: Update if we're at a leaf node and operate() told us to.
                if (update_needed) {
                   assert(have_computed_new_value);
                   assert(node_handler::is_leaf(node));
                   if (new_value) { // We have a new value to insert
                       if (new_value->has_cas()) {
                           new_value->set_cas(get_cpu_context()->worker->gen_cas());
                       }
                       bool success = leaf_node_handler::insert((leaf_node_t*)node, &key, new_value);
                       check("could not insert leaf btree node", !success);
                   } else {
                        // If we haven't already, do some deleting 
                       //key found, and value deleted
                       leaf_node_handler::remove((leaf_node_t*)node, &key);
                   }
                   update_needed = false; // TODO: update_needed should probably stay true; this can be a different state instead.
                   buf->set_dirty();
                }

                // STEP 4: Check to see if it's underfull, and merge/level if it is.
                if(!sb_buf && last_buf && node_handler::is_underfull(node))
                    printf("hala\n");
                if (last_buf && node_handler::is_underfull(node)) { // the root node is never underfull
                    // merge or level.
                    if(!sib_buf) { // Acquire a sibling to merge or level with
                        //logf(DBG, "generic acquire sibling\n");
                        state = acquire_sibling;
                        break;
                    } else {
                        // Sibling acquired, now decide whether to merge or level
                        node_t *sib_node = (node_t *)sib_buf->ptr();
                        node_handler::validate(sib_node);
                        node_t *parent_node = (node_t *)last_buf->ptr();
                        if (node_handler::is_mergable(node, sib_node, parent_node)) { // Merge
                            //logf(DBG, "internal merge\n");
                            btree_key *key_to_remove = (btree_key *)alloca(sizeof(btree_key) + MAX_KEY_SIZE); //TODO get alloca outta here
                            if (node_handler::nodecmp(node, sib_node) < 0) { // Nodes must be passed to merge in ascending order
                                node_handler::merge(node, sib_node, key_to_remove, parent_node);
                                buf->mark_deleted();
                                buf->release();
                                buf = sib_buf;
                                node_id = sib_node_id;
                                node = sib_node;
                                buf->set_dirty();
                            } else {
                                node_handler::merge(sib_node, node, key_to_remove, parent_node);
                                sib_buf->mark_deleted();
                                sib_buf->release();
                                buf->set_dirty();
                            }
                            sib_buf = NULL;

                            if (!internal_node_handler::is_singleton((internal_node_t*)parent_node)) {
                                internal_node_handler::remove((internal_node_t*)parent_node, key_to_remove);
                            } else {
                                //logf(DBG, "generic collapse root\n");
                                // parent has only 1 key (which means it is also the root), replace it with the node
                                // when we get here node_id should be the id of the new root
                                last_buf->mark_deleted();
                                insert_root(node_id);
                                assert(state == acquire_node);
                                break;
                            }
                        } else {
                            // Level
                            //logf(DBG, "generic level\n");
                            btree_key *key_to_replace = (btree_key *)alloca(sizeof(btree_key) + MAX_KEY_SIZE);
                            btree_key *replacement_key = (btree_key *)alloca(sizeof(btree_key) + MAX_KEY_SIZE);
                            bool leveled = node_handler::level(node, sib_node, key_to_replace, replacement_key, parent_node);

                            if (leveled) {
                                //set everyone dirty
                                sib_buf->set_dirty();

                                internal_node_handler::update_key((btree_internal_node *)parent_node, key_to_replace, replacement_key);
                                last_buf->set_dirty();
                                buf->set_dirty();
                            }
                            sib_buf->release();
                            sib_buf = NULL;
                        }
                    }
                }

                // Release the superblock, if we haven't already.
                // For internal nodes, we only release the superblock if we're past the root node.
                if(sb_buf && (last_buf || node_handler::is_leaf(node))) {
                        sb_buf->release();
                        sb_buf = NULL;
                }                    

                if(node_handler::is_leaf(node)) {
                    buf->release();
                    buf = NULL;
                    state = update_complete;
                    res = btree_fsm_t::transition_ok;
                    break;
                }

                // STEP 5: Semi-finalize and move up the tree.
                // Release and update the last node
                if(last_buf) {
                    assert(last_node_id != NULL_BLOCK_ID);
                    last_buf->release();
                }
                last_buf = buf;
                last_node_id = node_id;

                // Look up the next node
                node_id = internal_node_handler::lookup((internal_node_t*)node, &key);
                assert(node_id != NULL_BLOCK_ID);
                assert(node_id != SUPERBLOCK_ID);

                assert(state == acquire_node && res == btree_fsm_t::transition_ok);
                buf = NULL;
                break;
            }

            // Finalize the operation
            case update_complete: {
                // Release the final node
                if(last_node_id != NULL_BLOCK_ID) {
                    last_buf->release();
                    last_buf = NULL;
                    last_node_id = NULL_BLOCK_ID;
                }

                // End the transaction
                bool committed = transaction->commit(this);
                state = committing;
                if (committed) {
                    transaction = NULL;
                    res = btree_fsm_t::transition_complete;
                } else {
                    res = btree_fsm_t::transition_incomplete;
                }
                break;
            }

            // Finalize the transaction commit
            case committing:
                if (event) {
                    assert(event->event_type == et_commit);
                    assert(event->buf == transaction);
                    transaction = NULL;
                    res = btree_fsm_t::transition_complete;
                }
                break;
        }
        // We're done with one step, but we may be able to go to the next one
        // without getting a new event.
        event = NULL;
    }

    assert(res != btree_fsm_t::transition_complete || is_finished());
    return res;
}

template <class config_t>
void btree_modify_fsm<config_t>::split_node(buf_t *buf, buf_t **rbuf,
                                         block_id_t *rnode_id, btree_key *median) {
    buf_t *res = transaction->allocate(rnode_id);
    if(node_handler::is_leaf((node_t *)buf->ptr())) {
    	leaf_node_t *node = (leaf_node_t *)buf->ptr();
        leaf_node_t *rnode = (leaf_node_t *)res->ptr();
        leaf_node_handler::split(node, rnode, median);
        buf->set_dirty();
        res->set_dirty();
    } else {
    	internal_node_t *node = (internal_node_t *)buf->ptr();
        internal_node_t *rnode = (internal_node_t *)res->ptr();
        internal_node_handler::split(node, rnode, median);
        buf->set_dirty();
        res->set_dirty();
    }
    *rbuf = res;
}


#endif // __BTREE_MODIFY_FSM_TCC__
