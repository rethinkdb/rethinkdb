
#ifndef __BTREE_SET_FSM_IMPL_HPP__
#define __BTREE_SET_FSM_IMPL_HPP__

// TODO: holy shit this state machine is a fucking mess. We should
// make it NOT write only (i.e. human beings should be able to easily
// understand what is happening here). Needs to be redesigned (with
// UML and all).

// TODO: we should combine multiple writes into a single call to OS
// whenever possible. Perhaps the cache should do it itself (let's
// wait 'till we add transactions, perhaps it will be easier then).

// TODO: consider B#/B* trees to improve space efficiency

// TODO: perhaps allow memory reclamation due to oversplitting? We can
// be smart and only use a limited amount of ram for incomplete nodes
// (doing this efficiently very tricky for high insert
// workloads). Also, if the serializer is log-structured, we can write
// only a small part of each node.

template <class config_t>
void btree_set_fsm<config_t>::init_update(int _key, int _value) {
    key = _key;
    value = _value;
    state = update_acquiring_superblock;
}

template <class config_t>
typename btree_set_fsm<config_t>::transition_result_t btree_set_fsm<config_t>::do_update_acquiring_superblock()
{
    assert(state == update_acquiring_superblock);
    
    if(get_root_id(&node_id) == 0) {
        // We're temporarily assigning superblock id to node_id
        // variable, so that when we get the next disk completion
        // event, we can notify the cache.
        return btree_fsm_t::transition_incomplete;
    } else {
        if(cache_t::is_block_id_null(node_id))
            state = update_inserting_root;
        else
            state = update_acquiring_root;
        return btree_fsm_t::transition_ok;
    }
}

template <class config_t>
typename btree_set_fsm<config_t>::transition_result_t btree_set_fsm<config_t>::do_update_acquiring_root()
{
    // If root didn't exist, we should have ended up in
    // do_update_inserting_root()
    assert(!cache_t::is_block_id_null(node_id));

    // Acquire the actual root node
    node = (node_t*)btree_fsm_t::cache->acquire(node_id, this);
    if(node == NULL) {
        return btree_fsm_t::transition_incomplete;
    } else {
        state = update_acquiring_node;
        return btree_fsm_t::transition_ok;
    }
}

template <class config_t>
typename btree_set_fsm<config_t>::transition_result_t btree_set_fsm<config_t>::do_update_inserting_root()
{
    // If this is the first time we're entering this function,
    // allocate the root (otherwise, the root has already been
    // allocated here, and we just need to set its id in the metadata)
    if(cache_t::is_block_id_null(node_id)) {
        void *ptr = btree_fsm_t::cache->allocate(&node_id);
        node = new (ptr) leaf_node_t();
    }
    if(set_root_id(node_id)) {
        state = update_acquiring_node;
        return btree_fsm_t::transition_ok;
    } else {
        return btree_fsm_t::transition_incomplete;
    }
}

template <class config_t>
typename btree_set_fsm<config_t>::transition_result_t btree_set_fsm<config_t>::do_update_inserting_root_on_split()
{
    if(set_root_id(last_node_id)) {
        state = update_acquiring_node;
        return btree_fsm_t::transition_ok;
    } else {
        return btree_fsm_t::transition_incomplete;
    }
}

template <class config_t>
typename btree_set_fsm<config_t>::transition_result_t btree_set_fsm<config_t>::do_update_acquiring_node()
{
    node = (node_t*)btree_fsm_t::cache->acquire(node_id, this);
    if(node)
        return btree_fsm_t::transition_ok;
    else
        return btree_fsm_t::transition_incomplete;
}

template <class config_t>
typename btree_set_fsm<config_t>::transition_result_t btree_set_fsm<config_t>::do_transition(event_t *event)
{
    transition_result_t res = btree_fsm_t::transition_ok;

    // Make sure we've got either an empty or a disk event
    check("btree_fsm::do_transition - invalid event",
          event != NULL && event->event_type != et_disk);

    // Update the cache with the event
    if(event) {
        check("Could not complete AIO operation",
              event->result == 0 ||
              event->result == -1);
        // TODO: right now we assume that block_id_t is the same thing
        // as event->offset. In the future (once we fix the event
        // system), the block_id_t state should really be stored
        // within the event state. Currently, we cast the offset to
        // block_id_t, but it's a big hack, we need to get rid of this
        // later.
        if(event->op == eo_read) {
            btree_fsm_t::cache->aio_complete((block_id_t)event->offset, event->buf, false);
        } else if(event->op == eo_write) {
            btree_fsm_t::cache->aio_complete((block_id_t)event->offset, event->buf, true);
            nwrites--;
            // TODO: right now we expect to get all AIO write
            // notifications back before we return
            // "transition_complete" status. This will be a big
            // problem when we build caches with different (possibly
            // delayed) writeback strategies. We need to figure out
            // how to change this behavior.
            if(res == btree_fsm_t::transition_ok && state == update_complete) {
                if(nwrites == 0) {
                    return btree_fsm_t::transition_complete;
                } else {
                    return btree_fsm_t::transition_incomplete;
                }
            } else {
                return btree_fsm_t::transition_incomplete;
            }
        
        } else {
            check("btree_set_fsm::do_transition - invalid event", 1);
        }
    }

    // First, acquire the superblock (to get root node ID)
    if(res == btree_fsm_t::transition_ok && state == update_acquiring_superblock)
        res = do_update_acquiring_superblock();

    // Then, acquire the root block (or create it if it's missing)
    if(res == btree_fsm_t::transition_ok) {
        if(state == update_acquiring_root)
            res = do_update_acquiring_root();
        else if(state == update_inserting_root)
            res = do_update_inserting_root();
    }

    // If we were inserting root on split, do that
    if(res == btree_fsm_t::transition_ok && state == update_inserting_root_on_split)
        res = do_update_inserting_root_on_split();

    // If we were acquiring a node, do that
    // Go up the tree...
    while(res == btree_fsm_t::transition_ok && state == update_acquiring_node)
    {
        if(!node) {
            state = update_acquiring_node;
            res = do_update_acquiring_node();
            if(res != btree_fsm_t::transition_ok || state != update_acquiring_node) {
                break;
            }
        }

        // Proactively split the node
        if(node->is_full()) {
            int median;
            node_t *rnode;
            block_id_t rnode_id;
            bool new_root = false;
            split_node(node, &rnode, &rnode_id, &median);
            // Create a new root if we're splitting a root
            if(last_node == NULL) {
                new_root = true;
                void *ptr = btree_fsm_t::cache->allocate(&last_node_id);
                last_node = new (ptr) internal_node_t();
            };
            last_node->insert(median, node_id, rnode_id);
            last_node_dirty = true;
                
            // Figure out where the key goes
            if(key < median) {
                // Left node and node are the same thing
                btree_fsm_t::cache->release(rnode_id, (void*)rnode, true, this);
                nwrites++;
            } else if(key >= median) {
                btree_fsm_t::cache->release(node_id, (void*)node, true, this);
                nwrites++;
                node = rnode;
                node_id = rnode_id;
            }
            node_dirty = true;

            if(new_root) {
                state = update_inserting_root_on_split;
                res = do_update_inserting_root_on_split();
                if(res != btree_fsm_t::transition_ok || state != update_acquiring_node) {
                    break;
                }
            }
        }
        // Insert the value, or move up the tree
        if(node->is_leaf()) {
            ((leaf_node_t*)node)->insert(key, value);
            btree_fsm_t::cache->release(node_id, (void*)node, true, this);
            nwrites++;
            state = update_complete;
            res = btree_fsm_t::transition_ok;
            break;
        } else {
            // Release and update the last node
            if(!cache_t::is_block_id_null(last_node_id)) {
                btree_fsm_t::cache->release(last_node_id, (void*)last_node, last_node_dirty,
                                            this);
                last_node_dirty ? (nwrites++) : 0;
            }
            last_node = (internal_node_t*)node;
            last_node_id = node_id;
            last_node_dirty = node_dirty;
            node_dirty = false;
                
            // Look up the next node
            node_id = ((internal_node_t*)node)->lookup(key);
            node = NULL;
        }
    }

    // Release the final node
    if(res == btree_fsm_t::transition_ok && state == update_complete) {
        if(!cache_t::is_block_id_null(last_node_id)) {
            btree_fsm_t::cache->release(last_node_id, (void*)last_node, last_node_dirty,
                                        this);
            last_node_dirty ? (nwrites++) : 0;
            last_node_id = cache_t::null_block_id;
        }

        /*
        // TODO: this is a hack to return 'transition_complete' status
        // despite the fact that not all writes have returned (see the
        // nwrites variable, and the relevant todo close to the
        // beginning of this function. We should really redesign this
        // to account for the fact that writes may not remove
        // immediately (do we return 'transaction_complete' when all
        // writes to disk are done, or when all writes to memory are
        // done? This should probably be configurable).
        return btree_fsm_t::transition_complete;
        */
    }

    return res;
}

template <class config_t>
int btree_set_fsm<config_t>::set_root_id(block_id_t root_id) {
    block_id_t superblock_id = btree_fsm_t::cache->get_superblock_id();
    void *buf = btree_fsm_t::cache->acquire(superblock_id, this);
    if(buf == NULL) {
        return 0;
    }
    memcpy(buf, (void*)&root_id, sizeof(root_id));
    btree_fsm_t::cache->release(superblock_id, buf, true, this);
    nwrites++;
    return 1;
}

template <class config_t>
void btree_set_fsm<config_t>::split_node(node_t *node, node_t **rnode,
                                         block_id_t *rnode_id, int *median) {
    void *ptr = btree_fsm_t::cache->allocate(rnode_id);
    if(node->is_leaf()) {
        *rnode = new (ptr) leaf_node_t();
        ((leaf_node_t*)node)->split((leaf_node_t*)*rnode,
                                    median);
    } else {
        *rnode = new (ptr) internal_node_t();
        ((internal_node_t*)node)->split((internal_node_t*)*rnode,
                                        median);
    }
}


#endif // __BTREE_SET_FSM_IMPL_HPP__

