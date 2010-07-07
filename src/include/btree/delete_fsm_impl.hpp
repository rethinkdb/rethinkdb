
#ifndef __BTREE_DELETE_FSM_IMPL_HPP__
#define __BTREE_DELETE_FSM_IMPL_HPP__

#include "utils.hpp"
#include "cpu_context.hpp"

template <class config_t>
void btree_delete_fsm<config_t>::init_delete(btree_key *_key) {
    keycpy(key, _key);
    state = start_transaction;
}

template <class config_t>
typename btree_delete_fsm<config_t>::transition_result_t btree_delete_fsm<config_t>::do_start_transaction(event_t *event) {
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
typename btree_delete_fsm<config_t>::transition_result_t btree_delete_fsm<config_t>::do_acquire_superblock(event_t *event)
{
    printf("Acquire superblock\n");
    assert(state == acquire_superblock);

    if(event == NULL) {
        // First entry into the FSM; try to grab the superblock.
        block_id_t superblock_id = btree_fsm_t::get_cache()->get_superblock_id();
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
        if(cache_t::is_block_id_null(node_id)) {
            op_result = btree_not_found;
            state = delete_complete;
            sb_buf->release();
            sb_buf = NULL;
        } else {
            state = acquire_root;
        }
        return btree_fsm_t::transition_ok;
    } else {
        // Can't get the superblock buffer right away. Let's wait for
        // the cache notification.
        return btree_fsm_t::transition_incomplete;
    }
}

template <class config_t>
typename btree_delete_fsm<config_t>::transition_result_t btree_delete_fsm<config_t>::do_acquire_root(event_t *event) {
    printf("Acquire root\n");
    assert(state == acquire_root);
    
    // Make sure root exists
    if(cache_t::is_block_id_null(node_id)) {
        op_result = btree_not_found;
        state = delete_complete;
        return btree_fsm_t::transition_ok;
    }

    if(event == NULL) {
        // Acquire the actual root node
        buf = transaction->acquire(node_id, rwi_read, this);
    } else {
        // We already tried to grab the root, and we're getting a
        // cache notification about it.
        assert(event->buf);
        buf = (buf_t*)event->buf;
    }
    
    if(buf == NULL) {
        // Can't grab the root right away. Wait for a cache event.
        return btree_fsm_t::transition_incomplete;
    } else {
        // Got the root, move on to grabbing the node
        state = acquire_node;
        return btree_fsm_t::transition_ok;
    }
}

template <class config_t>
typename btree_delete_fsm<config_t>::transition_result_t btree_delete_fsm<config_t>::do_insert_root_on_collapse(event_t *event)
{
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
typename btree_delete_fsm<config_t>::transition_result_t btree_delete_fsm<config_t>::do_acquire_node(event_t *event) {
    printf("Acquire node\n");
    assert(state == acquire_node);
    // Either we already have the node (then event should be NULL), or
    // we don't have the node (in which case we asked for it before,
    // and it should be getting to us via an event)
    //assert((buf && !event) || (!buf && event));

    if (!event) {
        buf = transaction->acquire(node_id, rwi_read, this);
    } else {
        assert(event->buf);
        buf = (buf_t*) event->buf;
    }

    if(buf)
        return btree_fsm_t::transition_ok;
    else
        return btree_fsm_t::transition_incomplete;
}

template <class config_t>
typename btree_delete_fsm<config_t>::transition_result_t btree_delete_fsm<config_t>::do_acquire_sibling(event_t *event) {
    printf("Acquire sibling\n");
    assert(state == acquire_sibling);

    if (!event) {
        assert(last_buf);
        node_t *last_node = (node_t *)last_buf->ptr();

        internal_node_handler::sibling(((internal_node_t*)last_node), key, &sib_node_id);
        sib_buf = transaction->acquire(sib_node_id, rwi_read, this);
    } else {
        assert(event->buf);
        sib_buf = (buf_t*) event->buf;
    }

    if(sib_buf) {
        printf("Done with acquire sibling\n");
        state = acquire_node;
        return btree_fsm_t::transition_ok;
    } else {
        printf("Acquire sibling incomplete\n");
        return btree_fsm_t::transition_incomplete;
    }
}

template <class config_t>
typename btree_delete_fsm<config_t>::transition_result_t btree_delete_fsm<config_t>::do_transition(event_t *event) {
    printf("Do_transition\n");
    transition_result_t res = btree_fsm_t::transition_ok;

    // Make sure we've got either an empty or a cache event
    check("btree_fsm::do_transition - invalid event",
          !(!event || event->event_type == et_cache));

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
        
    // Then, acquire the root block
    if(res == btree_fsm_t::transition_ok && state == acquire_root) {
        res = do_acquire_root(event);
        event = NULL;
    }

    // If the previously acquired node was underfull, we acquire a sibling to level or merge
    if(res == btree_fsm_t::transition_ok && state == acquire_sibling) {
        res = do_acquire_sibling(event);
        event = NULL;
    }
    
    // If we need to change the root due to merging it's last two children, do that
    if(res == btree_fsm_t::transition_ok && state == insert_root_on_collapse) {
        res = do_insert_root_on_collapse(event);
        event = NULL;
    }

    // Acquire nodes
    while(res == btree_fsm_t::transition_ok && state == acquire_node) {
        printf("Start acquire node loop\n");
        if(!buf) {
            printf("No buf\n");
            state = acquire_node;
            res = do_acquire_node(event);
            event = NULL;
            if(res != btree_fsm_t::transition_ok || state != acquire_node) {
                break;
            }
        }

        node_t* node = (node_t*)buf->ptr();

        //Deal with underfull nodes if we find them
        if (((node_handler::is_leaf(node) && leaf_node_handler::is_underfull((leaf_node_t*)node)) ||
                   (!node_handler::is_leaf(node) && internal_node_handler::is_underfull((internal_node_t*) node))) && last_buf) {
            printf("Underfull node\n");
            if(!sib_buf) {
                state = acquire_sibling;
                res = do_acquire_sibling(NULL);
                event = NULL;
                printf("Sibling acquired: sib_buf = %li\n", (long int) sib_buf); //sibling NOT necessarily acquired
                continue;
            } else {
                // we have our sibling so we're ready to go
                printf("combining time\n");
                node_t *sib_node = (node_t*)sib_buf->ptr();
                if((node_handler::is_leaf(sib_node) && leaf_node_handler::is_underfull_or_min((leaf_node_t*) sib_node)) ||
                           (!node_handler::is_leaf(sib_node) && internal_node_handler::is_underfull_or_min((internal_node_t*) sib_node))) {
                    printf("Merge time\n");
                    btree_key *key_to_remove = (btree_key *)alloca(sizeof(btree_key) + MAX_KEY_SIZE); //TODO get alloca outta here
                    if (node_handler::is_leaf(node)) {
                        if (leaf_node_handler::nodecmp((leaf_node_t*)node, (leaf_node_t*)sib_node)) {
                            leaf_node_handler::merge((leaf_node_t*)node, (leaf_node_t*)sib_node, key_to_remove);
                            buf->release(); //TODO delete when api is ready
                            buf = sib_buf;
                            node_id = sib_node_id;
                        } else {
                            leaf_node_handler::merge((leaf_node_t*)sib_node, (leaf_node_t*)node, key_to_remove);
                            sib_buf->release(); //TODO delete when api is ready
                        }
                    } else {
                        if (internal_node_handler::nodecmp((internal_node_t*)node, (internal_node_t*)sib_node)) {
                            internal_node_handler::merge((internal_node_t*)node, (internal_node_t*)sib_node, key_to_remove, (internal_node_t *) last_buf->ptr());
                            buf->release(); //TODO delete when api is ready
                            buf = sib_buf;
                            node_id = sib_node_id;
                        } else {
                            internal_node_handler::merge((internal_node_t*)sib_node, (internal_node_t*)node, key_to_remove, (internal_node_t *) last_buf->ptr());
                            sib_buf->release(); //TODO delete when api is ready
                        }
                    }
                    sib_buf = NULL;

                    if (!internal_node_handler::is_singleton((internal_node_t*) last_buf->ptr())) {
                        //normal merge
                        internal_node_handler::remove((internal_node_t*) last_buf->ptr(), key_to_remove);
                    } else {
                        //parent has only 1 key (which means it is also the root), replace it with the node
                        // when we get here node_id should be the id of the new root
                        state = insert_root_on_collapse;
                        res = do_insert_root_on_collapse(NULL);
                        event = NULL;
                        continue;
                    }
                } else {
                    printf("Level time\n");
                    btree_key *key_to_replace = (btree_key *)alloca(sizeof(btree_key) + MAX_KEY_SIZE);
                    btree_key *replacement_key = (btree_key *)alloca(sizeof(btree_key) + MAX_KEY_SIZE);
                    if (node_handler::is_leaf(node))
                        leaf_node_handler::level(((leaf_node_t*)node), (leaf_node_t*) sib_node, key_to_replace, replacement_key);
                    else
                        internal_node_handler::level((internal_node_t*)node, (internal_node_t*) sib_node, key_to_replace, replacement_key, (internal_node_t*) last_buf->ptr());

                    //set everyone dirty
                    sib_buf->set_dirty();
                    sib_buf->release();
                    sib_buf = NULL;

                    internal_node_handler::update_key((internal_node_t*) last_buf->ptr(), key_to_replace, replacement_key);
                    last_buf->set_dirty();
                    
                    buf->set_dirty();
                }
            }
        }
        // Release the superblock, if we haven't already
        if(sb_buf && (last_buf || node_handler::is_leaf(node))) {
            sb_buf->release();
            sb_buf = NULL;
        }

        //actually do some deleting 
        if (node_handler::is_leaf(node)) {
            if(leaf_node_handler::remove((leaf_node_t*)node, key)) {
                //key found, and value deleted
                buf->set_dirty();
                buf->release();
                op_result = btree_found;
            } else {
                //key not found, nothing deleted
                buf->release();
                op_result = btree_not_found;
            }
            if (last_buf)
                last_buf->release();
            state = delete_complete;
            res = btree_fsm_t::transition_ok;
            break;
        } else {
            if(!cache_t::is_block_id_null(last_node_id) && last_buf) {
                last_buf->release();
            }
            last_buf = buf;
            last_node_id = node_id;

            node_id = internal_node_handler::lookup((internal_node_t*)node, key);
            buf = NULL;

            res = do_acquire_node(event);
            event = NULL;
        } 
    }

    // Finally, end our transaction.  This should always succeed immediately.
    if (res == btree_fsm_t::transition_ok && state == delete_complete) {
        bool committed __attribute__((unused)) = transaction->commit(this);
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
int btree_delete_fsm<config_t>::set_root_id(block_id_t root_id, event_t *event) {
    sb_buf->set_dirty();
    memcpy(sb_buf->ptr(), (void*)&root_id, sizeof(root_id));
    return 1;
}

#endif // __BTREE_DELETE_FSM_IMPL_HPP__

