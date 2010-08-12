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
        state = acquire_superblock;
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
        block_id_t superblock_id = cache->get_superblock_id();
        sb_buf = transaction->acquire(superblock_id, rwi_write, this);
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
        node_id = btree_fsm_t::get_root_id(sb_buf->ptr());
        if(cache_t::is_block_id_null(node_id))
            state = insert_root;
        else
            state = acquire_root;
        return btree_fsm_t::transition_ok;
    } else {
        // Can't get the superblock buffer right away. Let's wait for
        // the cache notification.
        return btree_fsm_t::transition_incomplete;
    }
}

template <class config_t>
typename btree_modify_fsm<config_t>::transition_result_t btree_modify_fsm<config_t>::do_acquire_root(event_t *event)
{
    // If root didn't exist, we should have ended up in
    // do_insert_root()
    assert(!cache_t::is_block_id_null(node_id));

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
        state = acquire_node;
        return btree_fsm_t::transition_ok;
    }
}

template <class config_t>
typename btree_modify_fsm<config_t>::transition_result_t btree_modify_fsm<config_t>::do_insert_root(event_t *event)
{
    // If this is the first time we're entering this function,
    // allocate the root (otherwise, the root has already been
    // allocated here, and we just need to set its id in the metadata)
    
    if(cache_t::is_block_id_null(node_id)) {
        buf = transaction->allocate(&node_id);
        leaf_node_handler::init((leaf_node_t *)buf->ptr());
        buf->set_dirty();
    }
    if(set_root_id(node_id, event)) {
        state = acquire_node;
        sb_buf->release();
        sb_buf = NULL;
        return btree_fsm_t::transition_ok;
    } else {
        return btree_fsm_t::transition_incomplete;
    }
}

template <class config_t>
typename btree_modify_fsm<config_t>::transition_result_t btree_modify_fsm<config_t>::do_insert_root_on_split(event_t *event)
{
    if(set_root_id(last_node_id, event)) {
        state = acquire_node;
        sb_buf->release();
        sb_buf = NULL;
        return btree_fsm_t::transition_ok;
    } else {
        return btree_fsm_t::transition_incomplete;
    }
}

template <class config_t>
typename btree_modify_fsm<config_t>::transition_result_t btree_modify_fsm<config_t>::do_acquire_node(event_t *event)
{
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
    }
    else
        return btree_fsm_t::transition_incomplete;
}

template <class config_t>
typename btree_modify_fsm<config_t>::transition_result_t btree_modify_fsm<config_t>::do_acquire_sibling(event_t *event) {
    assert(state == acquire_sibling);

    if (!event) {
        assert(last_buf);
        node_t *last_node = (node_t *)last_buf->ptr();

        internal_node_handler::sibling(((internal_node_t*)last_node), &key, &sib_node_id);
        sib_buf = transaction->acquire(sib_node_id, rwi_write, this);
    } else {
        assert(event->buf);
        sib_buf = (buf_t*) event->buf;
    }

    if(sib_buf) {
        state = acquire_node;
        return btree_fsm_t::transition_ok;
    } else {
        return btree_fsm_t::transition_incomplete;
    }
}

template <class config_t>
typename btree_modify_fsm<config_t>::transition_result_t btree_modify_fsm<config_t>::do_insert_root_on_collapse(event_t *event)
{
    /* similar to do_insert_root(() in modify_fsm. */
    if(set_root_id(node_id, event)) {
        state = acquire_node;
        sb_buf->release();
        sb_buf = NULL;
        return btree_fsm_t::transition_ok;
    } else {
        return btree_fsm_t::transition_incomplete;
    }
}

template <class config_t>
bool btree_modify_fsm<config_t>::do_check_for_split(node_t *node) {
    
    // Split the node if necessary
    bool new_root = false;
    bool full;
    if (node_handler::is_leaf(node)) {
        if (new_value == NULL) return btree_fsm_t::transition_ok; // if it's a delete and a leaf node, there's no need to split
        full = set_was_successful && leaf_node_handler::is_full((leaf_node_t *)node, &key, new_value);
    } else {
        full = internal_node_handler::is_full((internal_node_t *)node);
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
        if (node_handler::is_leaf(node))
            leaf_node_handler::print((leaf_node_t *) node);
        else
            internal_node_handler::print((internal_node_t *) node);
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
        printf("Parent:\n");
        internal_node_handler::print(last_node);

        printf("lnode:\n");
        if (node_handler::is_leaf(node))
            leaf_node_handler::print((leaf_node_t *) node);
        else
            internal_node_handler::print((internal_node_t *) node);

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
            node = (node_t *)buf->ptr();
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
        check("btree_delete _fsm::do_transition - invalid event", event->op != eo_read);
        check("Could not complete AIO operation",
              event->result == 0 ||
              event->result == -1);
    }

    // First, begin a transaction.
    if(res == btree_fsm_t::transition_ok && state == start_transaction) {
        res = do_start_transaction(event);
        event = NULL;
    }

    // Next, acquire the superblock (to get root node ID)
    if(res == btree_fsm_t::transition_ok && state == acquire_superblock) {
        assert(transaction); // We must have started our transaction by now.
        res = do_acquire_superblock(event);
        event = NULL;
    }

    // Then, acquire the root block (or create it if it's missing)
    if(res == btree_fsm_t::transition_ok && (state == acquire_root || state == insert_root)) {
        if(state == acquire_root)
            res = do_acquire_root(event);
        else if(state == insert_root)
            res = do_insert_root(event);
        event = NULL;
    }

    // If the previously acquired node was underfull, we acquire a sibling to level or merge
    if(res == btree_fsm_t::transition_ok && state == acquire_sibling) {
        res = do_acquire_sibling(event);
        event = NULL;
    }

    // If we were inserting root on split, do that
    if(res == btree_fsm_t::transition_ok && state == insert_root_on_split) {
        res = do_insert_root_on_split(event);
        event = NULL;
    }

    // If we need to change the root due to merging it's last two children, do that
    if(res == btree_fsm_t::transition_ok && state == insert_root_on_collapse) {
        res = do_insert_root_on_collapse(event);
        event = NULL;
    }

    // If we were acquiring a node, do that
    // Go up the tree...

    while(res == btree_fsm_t::transition_ok && state == acquire_node) {
    
        if(!buf) {
            state = acquire_node;
            res = do_acquire_node(event);
            event = NULL;
            if(res != btree_fsm_t::transition_ok || state != acquire_node) {
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
                // We can't call delete_expired() here because it would delete
                // the value after we finish modifying it. However, if
                // operate() gives us a new value, we're fine; if it doesn't,
                // we can go delete the value at that point.

                // XXX Is there any point in doing this check inside operate()?
                key_found = false;
            }

            // We've found the old value, or determined that it is not present
            // or expired; now compute the new value.
            
            if (key_found) {
                op_result = btree_found;
                set_was_successful = operate(&u.old_value, &new_value);
            } else {
                op_result = btree_not_found;
                set_was_successful = operate(NULL, &new_value);
            }

            if (!set_was_successful && expired) { // Silently delete the key.
                set_was_successful = true;
                new_value = NULL;
            }

            // by convention, new_value will be NULL if we're doing a delete.
            
            have_computed_new_value = true;
        }
        
        // STEP 2: Check if it's overfull. If so we would need to do a split.
        
        if (set_was_successful) {
            bool new_root = do_check_for_split(node);
            if(new_root) {
                state = insert_root_on_split;
                res = do_insert_root_on_split(NULL);
            }
            if(res != btree_fsm_t::transition_ok || state != acquire_node) {
                break;
            }
        }
        //  STEP 3: Check to see if it's underfull
        /*  TODO: Add some code to calculate if it will become underfull
            during a replace, incr, decr etc.
        */

        /*  But before we check if it's underfull, we need to do
         *  some deleting if we're a leaf node and we are a delete_fsm.
         */
         if (set_was_successful) {
            assert(have_computed_new_value);
            assert(node_handler::is_leaf(node));
            if (new_value) {
                // We have a new value to insert
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
            buf->set_dirty();
         }
        
        if (((node_handler::is_leaf(node) && leaf_node_handler::is_underfull((leaf_node_t *)node)) ||
            (node_handler::is_internal(node) && internal_node_handler::is_underfull((internal_node_t *)node)))
            && last_buf) { // the root node is never underfull

            // as long as we aren't an empty root.
            if (!(node_handler::is_leaf(node) && leaf_node_handler::is_empty((leaf_node_t *)node))) {

                // merge or level.
                if(!sib_buf) { // Acquire a sibling to merge or level with
                    //logf(DBG, "generic acquire sibling\n");
                    state = acquire_sibling;
                    res = do_acquire_sibling(NULL);
                    event = NULL;
                    continue;
                } else {
                    // Sibling acquired, now decide whether to merge or level
                    internal_node_t *sib_node = (internal_node_t*)sib_buf->ptr();
                    node_handler::validate(sib_node);
                    internal_node_t *parent_node = (internal_node_t*)last_buf->ptr();
                    internal_node_t *internal_node = (internal_node_t*)node;
                    if ( internal_node_handler::is_mergable(internal_node, sib_node, parent_node)) { // Merge
                        //logf(DBG, "internal merge\n");
                        btree_key *key_to_remove = (btree_key *)alloca(sizeof(btree_key) + MAX_KEY_SIZE); //TODO get alloca outta here
                        if (internal_node_handler::nodecmp(internal_node, sib_node) < 0) { // Nodes must be passed to merge in ascending order
                            internal_node_handler::merge(internal_node, sib_node, key_to_remove, parent_node);
                            buf->release(); //TODO delete when api is ready
                            buf = sib_buf;
                            buf->set_dirty();
                            node_id = sib_node_id;
                        } else {
                            internal_node_handler::merge(sib_node, internal_node, key_to_remove, parent_node);
                            sib_buf->release(); //TODO delete when api is ready
                            sib_buf->set_dirty();
                        }
                        sib_buf = NULL;

                        if (!internal_node_handler::is_singleton(parent_node)) {
                            internal_node_handler::remove(parent_node, key_to_remove);
                        } else {
                            //logf(DBG, "generic collapse root\n");
                            // parent has only 1 key (which means it is also the root), replace it with the node
                            // when we get here node_id should be the id of the new root
                            state = insert_root_on_collapse;
                            res = do_insert_root_on_collapse(NULL);
                            event = NULL;
                            continue;
                        }
                    } else {
                        // Level
                        //logf(DBG, "generic level\n");
                        btree_key *key_to_replace = (btree_key *)alloca(sizeof(btree_key) + MAX_KEY_SIZE);
                        btree_key *replacement_key = (btree_key *)alloca(sizeof(btree_key) + MAX_KEY_SIZE);
                        bool leveled = internal_node_handler::level(internal_node,  sib_node, key_to_replace, replacement_key, parent_node);

                        if (leveled) {
                            //set everyone dirty
                            sib_buf->set_dirty();

                            internal_node_handler::update_key(parent_node, key_to_replace, replacement_key);
                            last_buf->set_dirty();
                            buf->set_dirty();
                        }
                        sib_buf->release();
                        sib_buf = NULL;
                    }
                }                   
            }        
        }
        
        // Release the superblock, if we haven't already
        if(sb_buf) {
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

        // STEP 5.: Semi-finalize and move up the tree.
        // Release and update the last node
        if (last_buf) {
            assert(!cache_t::is_block_id_null(last_node_id));
            last_buf->release();
        }
        last_buf = buf;
        last_node_id = node_id;

        // Look up the next node
        node_id = internal_node_handler::lookup((internal_node_t*)node, &key);
        assert(!cache_t::is_block_id_null(node_id));
        assert(node_id != cache->get_superblock_id());

        assert(state == acquire_node);
        last_buf = buf;
    }

    // Finalize the transaction commit
    if(res == btree_fsm_t::transition_ok && state == committing) {
        if (event != NULL) {
            assert(event->event_type == et_commit);
            assert(event->buf == transaction);
            transaction = NULL;
            res = btree_fsm_t::transition_complete;
        }
    }

    // Finalize the operation
    if(res == btree_fsm_t::transition_ok && state == update_complete) {
        // Release the final node
        if(!cache_t::is_block_id_null(last_node_id)) {
            last_buf->release();
            last_buf = NULL;
            last_node_id = cache_t::null_block_id;
        }

        // End the transaction
        bool committed = transaction->commit(this);
        state = committing;
        if (committed) {
            transaction = NULL;
            res = btree_fsm_t::transition_complete;
        }
        event = NULL;
    }

    // Finalize the transaction commit
    if(res == btree_fsm_t::transition_ok && state == committing) {
        if (event != NULL) {
            assert(event->event_type == et_commit);
            assert(event->buf == transaction);
            transaction = NULL;
            res = btree_fsm_t::transition_complete;
        }
    }

    assert(res != btree_fsm_t::transition_complete || is_finished());
    return res;
}

template <class config_t>
int btree_modify_fsm<config_t>::set_root_id(block_id_t root_id, event_t *event) {
    memcpy(sb_buf->ptr(), (void*)&root_id, sizeof(root_id));
    sb_buf->set_dirty();
    return 1;
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
