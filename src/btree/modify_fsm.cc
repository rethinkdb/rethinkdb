#include "btree/modify_fsm.hpp"
#include "utils.hpp"
#include "buffer_cache/large_buf.hpp"
#include "btree/leaf_node.hpp"
#include "btree/internal_node.hpp"

// TODO: consider B#/B* trees to improve space efficiency

// TODO: perhaps allow memory reclamation due to oversplitting? We can
// be smart and only use a limited amount of ram for incomplete nodes
// (doing this efficiently very tricky for high insert
// workloads). Also, if the serializer is log-structured, we can write
// only a small part of each node.

// TODO: change rwi_write to rwi_intent followed by rwi_upgrade where
// relevant.

btree_modify_fsm_t::transition_result_t btree_modify_fsm_t::do_start_transaction(event_t *event) {
    assert(state == start_transaction);

    /* Either start a new transaction or retrieve the one we started. */
    assert(transaction == NULL);
    if (event == NULL) {
        transaction = cache->begin_transaction(rwi_write, this);
    } else {
        assert(event->buf); // We shouldn't get a callback unless this is valid
        transaction = (transaction_t *)event->buf;
    }

    /* Determine our forward progress based on our new state. */
    if (transaction) {
        return btree_fsm_t::transition_ok;
    } else {
        return btree_fsm_t::transition_incomplete; // Flush lock is held.
    }
}

btree_modify_fsm_t::transition_result_t btree_modify_fsm_t::do_acquire_superblock(event_t *event) {
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
        node_id = ((const btree_superblock_t*)sb_buf->get_data_read())->root_block;
        return btree_fsm_t::transition_ok;
    } else {
        // Can't get the superblock buffer right away. Let's wait for
        // the cache notification.
        return btree_fsm_t::transition_incomplete;
    }
}

btree_modify_fsm_t::transition_result_t btree_modify_fsm_t::do_acquire_root(event_t *event) {
    assert(state == acquire_root);

    // If there is no root, we make one.
    if(node_id == NULL_BLOCK_ID) {
        buf = transaction->allocate(&node_id);
        leaf_node_handler::init(leaf_node_handler::leaf_node(buf->get_data_write()));
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
        return btree_fsm_t::transition_ok;
    }
}

void btree_modify_fsm_t::insert_root(block_id_t root_id) {
    assert(sb_buf);
    ((btree_superblock_t*)sb_buf->get_data_write())->root_block = root_id;
    sb_buf->release();
    sb_buf = NULL;
}

btree_modify_fsm_t::transition_result_t btree_modify_fsm_t::do_acquire_node(event_t *event) {
    assert(state == acquire_node);
    if(event == NULL) {
        buf = transaction->acquire(node_id, rwi_write, this);
    } else {
        assert(event->buf);
        buf = (buf_t*)event->buf;
    }

    if(buf) {
        return btree_fsm_t::transition_ok;
    } else {
        return btree_fsm_t::transition_incomplete;
    }
}

// TODO: Fix this when large_buf's API supports it.
btree_modify_fsm_t::transition_result_t btree_modify_fsm_t::do_acquire_large_value(event_t *event) {
    assert(state == acquire_large_value);

    assert(old_value.large_value());

    if (!event) {
        old_large_buf = new large_buf_t(transaction);
        // TODO: Put the cast in node.hpp
        old_large_buf->acquire(* (block_id_t *) old_value.value(), old_value.value_size(), rwi_write, this);
        return btree_fsm_t::transition_incomplete;
    } else {
        assert(event->buf);
        assert(event->event_type == et_large_buf);
        assert(old_large_buf == (large_buf_t *) event->buf);
        assert(old_large_buf->state == large_buf_t::loaded);
        return btree_fsm_t::transition_ok;
    }
}

btree_modify_fsm_t::transition_result_t btree_modify_fsm_t::do_acquire_sibling(event_t *event) {
    assert(state == acquire_sibling);

    if (!event) {
        assert(last_buf);
        internal_node_t *last_node = internal_node_handler::internal_node(last_buf->get_data_write());

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

bool btree_modify_fsm_t::do_check_for_split(const node_t **node) {
    // Split the node if necessary
    bool new_root = false;
    bool full;
    if (node_handler::is_leaf(*node)) {
        if (update_needed && new_value == NULL) return false; // if we're deleting and it's a leaf node, there's no need to split
        full = update_needed && leaf_node_handler::is_full(leaf_node_handler::leaf_node(*node), &key, new_value);
    } else {
        full = internal_node_handler::is_full(internal_node_handler::internal_node(*node));
    }
    if (full) {
        did_split = true;
#ifdef BTREE_DEBUG
        printf("===SPLITTING===\n");
        printf("Parent:\n");
        if (last_buf != NULL)
            internal_node_handler::print(internal_node_handler::internal_node(last_buf->get_data_read()));
        else
            printf("NULL parent\n");

        printf("node:\n");
        if (node_handler::is_leaf(*node))
            leaf_node_handler::print(leaf_node_handler::leaf_node(*node));
        else
            internal_node_handler::print(internal_node_handler::internal_node(*node));
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
            last_node = internal_node_handler::internal_node(last_buf->get_data_write());
            internal_node_handler::init(last_node);
        } else {
            last_node = internal_node_handler::internal_node(last_buf->get_data_write());
        }
        
        bool success = internal_node_handler::insert(last_node, median, node_id, rnode_id);
        check("could not insert internal btree node", !success);
     
#ifdef BTREE_DEBUG
        printf("\t|\n\t| Median = "); median->print(); printf("\n\t|\n\tV\n");
        printf("Parent:\n");
        internal_node_handler::print(last_node);

        printf("lnode:\n");
        if (node_handler::is_leaf(*node))
            leaf_node_handler::print(leaf_node_handler::leaf_node(*node));
        else
            internal_node_handler::print(internal_node_handler::internal_node(*node));

        printf("rnode:\n");
        if (node_handler::is_leaf(node_handler::node(rbuf->get_data_read())))
            leaf_node_handler::print(leaf_node_handler::leaf_node(rbuf->get_data_read()));
        else
            internal_node_handler::print(internal_node_handler::internal_node(rbuf->get_data_read()));
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
            *node = node_handler::node(buf->get_data_read());
            node_id = rnode_id;
        }
    }
    return new_root;
}

btree_modify_fsm_t::transition_result_t btree_modify_fsm_t::do_transition(event_t *event) {
    transition_result_t res = btree_fsm_t::transition_ok;

    // Make sure we've got either an empty or a cache event
    check("btree_fsm::do_transition - invalid event", event != NULL &&
          event->event_type != et_cache && event->event_type != et_large_buf && event->event_type != et_commit);

    // Update the cache with the event
    if(event && (event->event_type == et_cache || event->event_type == et_large_buf)) {
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
            // If we found our value and it was large, acquire the value's large_buf.
            case acquire_large_value: {
                res = do_acquire_large_value(event);
                if (res == btree_fsm_t::transition_ok) state = acquire_node;
                break;
            }
            case operating: {
                on_operate_completed();
                state = acquire_node;
                res = btree_fsm_t::transition_ok;
                break;
            }
            // If we were acquiring a node, do that
            // Go up the tree...
            case acquire_node: {
                // STEP 1: Acquire the node.
                if(!buf) {
                    res = do_acquire_node(event);
                    if (res != btree_fsm_t::transition_ok) {
                        break;
                    }
                }

                const node_t *node = node_handler::node(buf->get_data_read());

                // STEP 2: If we're at the destination leaf, potentially acquire a large value, then compute a new value.
                if (node_handler::is_leaf(node) && !have_computed_new_value) {
                    // TODO: Clean up this mess.
                    if (!dest_reached) {
                        key_found = leaf_node_handler::lookup((btree_leaf_node*)node, &key, &old_value);
                        dest_reached = true;
                    }

                    if (key_found && old_value.large_value() && !old_large_buf) {
                        state = acquire_large_value;
                        break;
                    }

                    expired = key_found && old_value.expired();
                    if (expired) {
                        // We tell operate() that the key wasn't found. If it returns
                        // true, we'll replace/delete the value as usual; if it returns
                        // false, we'll silently delete the key.
                        key_found = false;
                    }

                    // We've found the old value, or determined that it is not
                    // present or expired; now compute the new value.
                    if (!operated) {
                        // TODO: Maybe leaf_node_handler::lookup should put NULL into old_value so we can be more consistent here?
                        res = operate(key_found ? &old_value : NULL, old_large_buf, &new_value);
                        operated = true;
                        if (res == btree_fsm_t::transition_ok) {
                            on_operate_completed();
                        } else {
                            state = operating;
                            break;
                        }
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
                    // Update stats
                    if (new_value && !key_found) {
                        // TODO PERFMON get_cpu_context()->worker->total_items++;
                        // TODO PERFMON get_cpu_context()->worker->curr_items++;
                    } else if (key_found && !new_value) {
                        // TODO PERFMON get_cpu_context()->worker->curr_items--;
                    }
                    
                   assert(have_computed_new_value);
                   assert(node_handler::is_leaf(node));
                   if (new_value) { // We have a new value to insert
                       if (new_value->has_cas()) {
                           new_value->set_cas(slice->gen_cas());
                       }
                       bool success = leaf_node_handler::insert(leaf_node_handler::leaf_node(buf->get_data_write()), &key, new_value);
                       check("could not insert leaf btree node", !success);
                   } else {
                        // If we haven't already, do some deleting 
                       //key found, and value deleted
                       leaf_node_handler::remove(leaf_node_handler::leaf_node(buf->get_data_write()), &key);
                   }
                   update_needed = false; // TODO: update_needed should probably stay true; this can be a different state instead.
                }

                // STEP 4: Check to see if it's underfull, and merge/level if it is.
                if (last_buf && node_handler::is_underfull(node)) { // the root node is never underfull
                    assert(!did_split); /* this failing means a split then
                                           merge or split then underfull bug
                                           chec epsilon usage in
                                           internal_node.cc and leaf_node.cc */
                    // merge or level.
                    if(!sib_buf) { // Acquire a sibling to merge or level with
                        //logf(DBG, "generic acquire sibling\n");
                        state = acquire_sibling;
                        break;
                    } else {
                        // Sibling acquired, now decide whether to merge or level
                        node_t *sib_node = node_handler::node(sib_buf->get_data_write());
                        node_handler::validate(sib_node);
                        node_t *parent_node = node_handler::node(last_buf->get_data_write());
                        if (node_handler::is_mergable(node, sib_node, parent_node)) { // Merge
                            //logf(DBG, "internal merge\n");
                            btree_key *key_to_remove = (btree_key *)alloca(sizeof(btree_key) + MAX_KEY_SIZE); //TODO get alloca outta here
                            if (node_handler::nodecmp(node, sib_node) < 0) { // Nodes must be passed to merge in ascending order
                                node_handler::merge(node_handler::node(buf->get_data_write()), sib_node, key_to_remove, parent_node);
                                buf->mark_deleted();
                                buf->release();
                                buf = sib_buf;
                                sib_buf = NULL;
                                node_id = sib_node_id;
                                node = sib_node;
                            } else {
                                node_handler::merge(sib_node, node_handler::node(buf->get_data_write()), key_to_remove, parent_node);
                                sib_buf->mark_deleted();
                                sib_buf->release();
                                sib_buf = NULL;
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
                            bool leveled = node_handler::level(node_handler::node(buf->get_data_write()), sib_node, key_to_replace, replacement_key, parent_node);

                            if (leveled) {
                                internal_node_handler::update_key((btree_internal_node *)parent_node, key_to_replace, replacement_key);
                            }
                            sib_buf->release();
                            sib_buf = NULL;
                        }
                    }
                } else {
                    did_split = false;
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
                    last_buf = NULL;
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
                if (last_node_id != NULL_BLOCK_ID) {
                    last_buf->release();
                    last_buf = NULL;
                    last_node_id = NULL_BLOCK_ID;
                }
                if (old_large_buf) {
                    old_large_buf->release(); // TODO: Delete if necessary.
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

void btree_modify_fsm_t::split_node(buf_t *buf, buf_t **rbuf,
                                    block_id_t *rnode_id, btree_key *median) {
    buf_t *res = transaction->allocate(rnode_id);
    if(node_handler::is_leaf(node_handler::node(buf->get_data_read()))) {
        leaf_node_t *node = leaf_node_handler::leaf_node(buf->get_data_write());
        leaf_node_t *rnode = leaf_node_handler::leaf_node(res->get_data_write());
        leaf_node_handler::split(node, rnode, median);
    } else {
        internal_node_t *node = internal_node_handler::internal_node(buf->get_data_write());
        internal_node_t *rnode = internal_node_handler::internal_node(res->get_data_write());
        internal_node_handler::split(node, rnode, median);
    }
    *rbuf = res;
}

