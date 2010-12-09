#include "btree/modify_fsm.hpp"
#include "utils.hpp"
#include "buffer_cache/large_buf.hpp"
#include "btree/leaf_node.hpp"
#include "btree/internal_node.hpp"
#include "btree/replicate.hpp"

// TODO: consider B#/B* trees to improve space efficiency

// TODO: perhaps allow memory reclamation due to oversplitting? We can
// be smart and only use a limited amount of ram for incomplete nodes
// (doing this efficiently very tricky for high insert
// workloads). Also, if the serializer is log-structured, we can write
// only a small part of each node.

// TODO: change rwi_write to rwi_intent followed by rwi_upgrade where
// relevant.

perfmon_counter_t pm_btree_depth("btree_depth");

btree_modify_fsm_t::transition_result_t btree_modify_fsm_t::do_start_transaction(event_t *event) {
    assert(state == start_transaction);

    /* Either start a new transaction or retrieve the one we started. */
    assert(transaction == NULL);
    if (event == NULL) {
        transaction = cache->begin_transaction(rwi_write, this);
    } else {
        assert(event->buf); // We shouldn't get a callback unless this is valid
        transaction = ptr_cast<transaction_t>(event->buf);
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
        sb_buf = reinterpret_cast<buf_t *>(event->buf);
    }

    if(sb_buf) {
        // Got the superblock buffer (either right away or through
        // cache notification). Grab the root id, and move on to
        // acquiring the root.
        node_id = reinterpret_cast<const btree_superblock_t*>(sb_buf->get_data_read())->root_block;
        assert(node_id != SUPERBLOCK_ID);
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
        leaf_node_handler::init(cache->get_block_size(), ptr_cast<leaf_node_t>(buf->get_data_write()), current_time());
        insert_root(node_id);
        pm_btree_depth++;
        return btree_fsm_t::transition_ok;
    }

    if(event == NULL) {
        // Acquire the actual root node
        buf = transaction->acquire(node_id, rwi_write, this);
    } else {
        // We already tried to acquire the root node, and here it is
        // via the cache notification.
        assert(event->buf);
        buf = reinterpret_cast<buf_t *>(event->buf);
    }

    if(buf == NULL) {
        return btree_fsm_t::transition_incomplete;
    } else {
        return btree_fsm_t::transition_ok;
    }
}

void btree_modify_fsm_t::insert_root(block_id_t root_id) {
    assert(sb_buf);
    reinterpret_cast<btree_superblock_t *>(sb_buf->get_data_write())->root_block = root_id;
    sb_buf->release();
    sb_buf = NULL;
}

btree_modify_fsm_t::transition_result_t btree_modify_fsm_t::do_acquire_node(event_t *event) {
    assert(state == acquire_node);
    if(event == NULL) {
        buf = transaction->acquire(node_id, rwi_write, this);
    } else {
        assert(event->buf);
        buf = reinterpret_cast<buf_t *>(event->buf);
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

    assert(old_value.is_large());

    if (!event) {
        old_large_buf = new large_buf_t(transaction);
        // TODO: Put the cast in node.hpp
        old_large_buf->acquire(old_value.lb_ref(), rwi_write, this);
        return btree_fsm_t::transition_incomplete;
    } else {
        assert(event->buf);
        assert(event->event_type == et_large_buf);
        assert(old_large_buf == reinterpret_cast<large_buf_t *>(event->buf));
        assert(old_large_buf->state == large_buf_t::loaded);
        return btree_fsm_t::transition_ok;
    }
}

btree_modify_fsm_t::transition_result_t btree_modify_fsm_t::do_acquire_sibling(event_t *event) {
    assert(state == acquire_sibling);

    if (!event) {
        assert(last_buf);
        internal_node_t *last_node = ptr_cast<internal_node_t>(last_buf->get_data_write());

        internal_node_handler::sibling(last_node, &key, &sib_node_id);
        sib_buf = transaction->acquire(sib_node_id, rwi_write, this);
    } else {
        assert(event->buf);
        sib_buf = reinterpret_cast<buf_t *>(event->buf);
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
        full = update_needed && leaf_node_handler::is_full(ptr_cast<leaf_node_t>(*node), &key, new_value);
    } else {
        full = internal_node_handler::is_full(ptr_cast<internal_node_t>(*node));
    }
    if (full) {
        did_split = true;
#ifdef BTREE_DEBUG
        printf("===SPLITTING===\n");
        printf("Parent:\n");
        if (last_buf != NULL)
            internal_node_handler::print(ptr_cast<internal_node_t>(last_buf->get_data_read()));
        else
            printf("NULL parent\n");

        printf("node:\n");
        if (node_handler::is_leaf(*node))
            leaf_node_handler::print(ptr_cast<leaf_node_t>(*node));
        else
            internal_node_handler::print(ptr_cast<internal_node_t>(*node));
#endif
        char memory[sizeof(btree_key) + MAX_KEY_SIZE];
        btree_key *median = reinterpret_cast<btree_key *>(memory);
        buf_t *rbuf;
        internal_node_t *last_node;
        block_id_t rnode_id;
        split_node(buf, &rbuf, &rnode_id, median);
        // Create a new root if we're splitting a root
        if(last_buf == NULL) {
            new_root = true;
            last_buf = transaction->allocate(&last_node_id);
            last_node = ptr_cast<internal_node_t>(last_buf->get_data_write());
            internal_node_handler::init(cache->get_block_size(), last_node);
        } else {
            last_node = ptr_cast<internal_node_t>(last_buf->get_data_write());
        }
        
        bool success = internal_node_handler::insert(cache->get_block_size(), last_node, median, node_id, rnode_id);
        check("could not insert internal btree node", !success);
     
#ifdef BTREE_DEBUG
        printf("\t|\n\t| Median = "); median->print(); printf("\n\t|\n\tV\n");
        printf("Parent:\n");
        internal_node_handler::print(last_node);

        printf("lnode:\n");
        if (node_handler::is_leaf(*node))
            leaf_node_handler::print(ptr_cast<leaf_node_t>(*node));
        else
            internal_node_handler::print(ptr_cast<internal_node_t>(*node));

        printf("rnode:\n");
        if (node_handler::is_leaf(ptr_cast<node_t>(rbuf->get_data_read())))
            leaf_node_handler::print(ptr_cast<leaf_node_t>(rbuf->get_data_read()));
        else
            internal_node_handler::print(ptr_cast<internal_node_t>(rbuf->get_data_read()));
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
            *node = ptr_cast<node_t>(buf->get_data_read());
            node_id = rnode_id;
        }
    }
    return new_root;
}

// have_finished_operating() is called by the subclass when it is done with operate() and has
// a new value for us to insert (or wants us to delete the key)
void btree_modify_fsm_t::have_finished_operating(btree_value *nv, large_buf_t *nlb) {
    assert(!operate_is_done);
    operate_is_done = true;
    update_needed = true;
    new_value = nv;
    new_large_buf = nlb;
    
    if (new_value && new_value->is_large()) {
        assert(new_large_buf);
        assert(new_value->lb_ref().block_id == new_large_buf->get_root_ref().block_id);
    } else {
        assert(!new_large_buf);
    }
    
    if (!in_operate_call) do_transition(NULL);
}

// have_failed_operating() is called by the subclass when it is done with operate() and does
// not want to make a change
void btree_modify_fsm_t::have_failed_operating() {
    assert(!operate_is_done);
    operate_is_done = true;
    update_needed = false;
    if (!in_operate_call) do_transition(NULL);
}

// have_copied_value is called by the replicant when it's done with the value
void btree_modify_fsm_t::have_copied_value() {
    replicants_awaited--;
    assert(replicants_awaited >= 0);
    if (replicants_awaited == 0 && !in_value_call) {
        state = update_complete;
        do_transition(NULL);
    }
}

void btree_modify_fsm_t::do_transition(event_t *event) {
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
            
            // Go to the core with the cache on it.
            case go_to_cache_core: {
                state = start_transaction;
                if (continue_on_cpu(slice->home_cpu, this)) res = btree_fsm_t::transition_ok;
                else res = btree_fsm_t::transition_incomplete;
                break;
            }
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

                const node_t *node = ptr_cast<node_t>(buf->get_data_read());

                // STEP 2: If we're at the destination leaf, potentially acquire a large value, then compute a new value.
                if (node_handler::is_leaf(node) && !have_computed_new_value) {
                    // TODO: Clean up this mess.
                    if (!dest_reached) {
                        key_found = leaf_node_handler::lookup(ptr_cast<leaf_node_t>(node), &key, &old_value);
                        dest_reached = true;
                    }

                    if (key_found && old_value.is_large() && !old_large_buf) {
                        state = acquire_large_value;
                        break;
                    }

                    expired = key_found && old_value.expired();
                    if (expired) {
                        // We tell operate() that the key wasn't found. If it calls
                        // have_finished_operating(), we'll replace/delete the value as usual;
                        // if it calls have_failed_operating(), we'll silently delete the key.
                        key_found = false;
                    }

                    // We've found the old value, or determined that it is not
                    // present or expired; now compute the new value.
                    if (!operated) {
                        // TODO: Maybe leaf_node_handler::lookup should put NULL into old_value so we can be more consistent here?
                        in_operate_call = true;
                        operate_is_done = false;
                        operate(key_found ? &old_value : NULL, old_large_buf);
                        in_operate_call = false;
                        operated = true;
                        if (!operate_is_done) {
                            res = btree_fsm_t::transition_incomplete;
                            break;
                        }
                    }

                    if (!update_needed && expired) { // Silently delete the key.
                        new_value = NULL;
                        new_large_buf = NULL;
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
                        pm_btree_depth++;
                    }
                }

                // STEP 3: Update if we're at a leaf node and operate() told us to.
                if (update_needed && !update_done) {
                    // TODO make sure we're updating leaf node timestamps.

                   assert(have_computed_new_value);
                   assert(node_handler::is_leaf(node));
                   if (new_value) { // We have a new value to insert
                       if (new_value->has_cas() && !cas_already_set) {
                           new_value->set_cas(slice->gen_cas());
                       }
                       new_value_timestamp = current_time();
                       bool success = leaf_node_handler::insert(cache->get_block_size(), ptr_cast<leaf_node_t>(buf->get_data_write()), &key, new_value, new_value_timestamp);
                       check("could not insert leaf btree node", !success);
                   } else {
                       // If we haven't already, do some deleting.
                       // key found, and value deleted
                       leaf_node_handler::remove(cache->get_block_size(), ptr_cast<leaf_node_t>(buf->get_data_write()), &key);
                   }
                   update_done = true;
                }

                // STEP 4: Check to see if it's underfull, and merge/level if it is.
                if (last_buf && node_handler::is_underfull(cache->get_block_size(), node)) { // the root node is never underfull
                    assert(!did_split); /* this failing means a split then
                                           merge or split then underfull bug
                                           chec epsilon usage in
                                           internal_node.cc and leaf_node.cc */
                    // merge or level.
                    if(!sib_buf) { // Acquire a sibling to merge or level with
                        //logDBG("generic acquire sibling\n");
                        state = acquire_sibling;
                        break;
                    } else {
                        // Sibling acquired, now decide whether to merge or level
                        node_t *sib_node = ptr_cast<node_t>(sib_buf->get_data_write());
#ifndef NDEBUG
                        node_handler::validate(cache->get_block_size(), sib_node);
#endif
                        node_t *parent_node = ptr_cast<node_t>(last_buf->get_data_write());
                        if (node_handler::is_mergable(cache->get_block_size(), node, sib_node, parent_node)) { // Merge
                            //logDBG("internal merge\n");

                            char key_to_remove_buf[sizeof(btree_key) + MAX_KEY_SIZE];
                            btree_key *key_to_remove = ptr_cast<btree_key>(key_to_remove_buf);

                            if (node_handler::nodecmp(node, sib_node) < 0) { // Nodes must be passed to merge in ascending order
                                node_handler::merge(cache->get_block_size(), ptr_cast<node_t>(buf->get_data_write()), sib_node, key_to_remove, parent_node);
                                buf->mark_deleted();
                                buf->release();
                                buf = sib_buf;
                                sib_buf = NULL;
                                node_id = sib_node_id;
                                node = sib_node;
                            } else {
                                node_handler::merge(cache->get_block_size(), sib_node, ptr_cast<node_t>(buf->get_data_write()), key_to_remove, parent_node);
                                sib_buf->mark_deleted();
                                sib_buf->release();
                                sib_buf = NULL;
                            }
                            sib_buf = NULL;

                            if (!internal_node_handler::is_singleton(ptr_cast<internal_node_t>(parent_node))) {
                                internal_node_handler::remove(cache->get_block_size(), ptr_cast<internal_node_t>(parent_node), key_to_remove);
                            } else {
                                //logDBG("generic collapse root\n");
                                // parent has only 1 key (which means it is also the root), replace it with the node
                                // when we get here node_id should be the id of the new root
                                last_buf->mark_deleted();
                                insert_root(node_id);
                                pm_btree_depth--;
                                assert(state == acquire_node);
                                break;
                            }
                        } else {
                            // Level
                            //logDBG("generic level\n");
                            char key_to_replace_buf[sizeof(btree_key) + MAX_KEY_SIZE];
                            btree_key *key_to_replace = ptr_cast<btree_key>(key_to_replace_buf);
                            char replacement_key_buf[sizeof(btree_key) + MAX_KEY_SIZE];
                            btree_key *replacement_key = ptr_cast<btree_key>(replacement_key_buf);
                            bool leveled = node_handler::level(cache->get_block_size(), ptr_cast<node_t>(buf->get_data_write()), sib_node, key_to_replace, replacement_key, parent_node);

                            if (leveled) {
                                internal_node_handler::update_key(ptr_cast<internal_node_t>(parent_node), key_to_replace, replacement_key);
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
                    state = call_replicants;
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
                node_id = internal_node_handler::lookup(ptr_cast<internal_node_t>(node), &key);
                assert(node_id != NULL_BLOCK_ID);
                assert(node_id != SUPERBLOCK_ID);

                assert(state == acquire_node && res == btree_fsm_t::transition_ok);
                buf = NULL;
                break;
            }

            // Notify anything that is waiting on a trigger
            case call_replicants: {

                // Release the final node
                if (last_node_id != NULL_BLOCK_ID) {
                    last_buf->release();
                    last_buf = NULL;
                    last_node_id = NULL_BLOCK_ID;
                }

                if (update_needed) {

                    replicants_awaited = slice->replicants.size();
                    in_value_call = true;

                    if (new_value) {

                        // Build a value to pass to the replicants
                        if (new_value->is_large()) {
                            for (int64_t i = 0; i < new_large_buf->get_num_segments(); i++) {
                                uint16_t size;
                                const void *data = new_large_buf->get_segment(i, &size);
                                replicant_bg.add_buffer(size, data);
                            }
                        } else {
                            replicant_bg.add_buffer(new_value->value_size(), new_value->value());
                        }

                        // Pass it to the replicants
                        for (int i = 0; i < (int)slice->replicants.size(); i++) {
                            slice->replicants[i]->callback->value(&key, &replicant_bg, this,
                                new_value->mcflags(), new_value->exptime(),
                                                                  new_value->has_cas() ? new_value->cas() : 0, new_value_timestamp);
                        }

                    } else {

                        // Pass NULL to the replicants
                        for (int i = 0; i < (int)slice->replicants.size(); i++) {
                            slice->replicants[i]->callback->value(&key, NULL, this, 0, 0, 0, current_time());
                        }

                    }

                    in_value_call = false;

                    if (replicants_awaited == 0) {
                        state = update_complete;
                        res = btree_fsm_t::transition_ok;
                    } else {
                        res = btree_fsm_t::transition_incomplete;
                    }

                } else {
                    state = update_complete;
                    res = btree_fsm_t::transition_ok;
                }
                break;
            }
            
            case update_complete: {
                // Free large bufs if necessary
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

                // End the transaction
                bool committed = transaction->commit(this);
                state = committing;
                if (committed) {
                    transaction = NULL;
                    state = call_callback_and_delete_self;
                    if (continue_on_cpu(home_cpu, this)) res = btree_fsm_t::transition_ok;
                    else res = btree_fsm_t::transition_incomplete;
                } else {
                    res = btree_fsm_t::transition_incomplete;
                }
                break;
            }

            // Finalize the transaction commit
            case committing: {
                if (event) {
                    assert(event->event_type == et_commit);
                    assert(event->buf == transaction);
                    transaction = NULL;
                    state = call_callback_and_delete_self;
                    if (continue_on_cpu(home_cpu, this)) res = btree_fsm_t::transition_ok;
                    else res = btree_fsm_t::transition_incomplete;
                }
                break;
            }
            
            // Call the callback and clean up
            case call_callback_and_delete_self: {
                assert_cpu();
                call_callback_and_delete();
                res = btree_fsm_t::transition_incomplete;   // So we break out of the loop
                break;
            }
        }
        // We're done with one step, but we may be able to go to the next one
        // without getting a new event.
        event = NULL;
    }
}

void btree_modify_fsm_t::split_node(buf_t *buf, buf_t **rbuf,
                                    block_id_t *rnode_id, btree_key *median) {
    buf_t *res = transaction->allocate(rnode_id);
    if(node_handler::is_leaf(ptr_cast<node_t>(buf->get_data_read()))) {
        leaf_node_t *node = ptr_cast<leaf_node_t>(buf->get_data_write());
        leaf_node_t *rnode = ptr_cast<leaf_node_t>(res->get_data_write());
        leaf_node_handler::split(cache->get_block_size(), node, rnode, median);
    } else {
        internal_node_t *node = ptr_cast<internal_node_t>(buf->get_data_write());
        internal_node_t *rnode = ptr_cast<internal_node_t>(res->get_data_write());
        internal_node_handler::split(cache->get_block_size(), node, rnode, median);
    }
    *rbuf = res;
}

