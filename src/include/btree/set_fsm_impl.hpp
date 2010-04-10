
#ifndef __BTREE_SET_FSM_IMPL_HPP__
#define __BTREE_SET_FSM_IMPL_HPP__

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
    printf("do_update_acquiring_superblock\n");
    
    if(get_root_id(&node_id) == 0) {
        // We're temporarily assigning superblock id to node_id
        // variable, so that when we get the next disk completion
        // event, we can notify the cache.
        node_id = btree_fsm_t::cache->get_superblock_id();
        return btree_fsm_t::transition_incomplete;
    } else {
        if(serializer_t::is_block_id_null(node_id))
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
    assert(!serializer_t::is_block_id_null(node_id));
    printf("do_update_acquiring_root\n");

    // Acquire the actual root node
    node = (node_t*)btree_fsm_t::cache->acquire(node_id, btree_fsm_t::netfsm);
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
    printf("do_update_inserting_root\n");
    // If this is the first time we're entering this function,
    // allocate the root (otherwise, the root has already been
    // allocated here, and we just need to set its id in the metadata)
    if(serializer_t::is_block_id_null(new_root_id)) {
        void *ptr = btree_fsm_t::cache->allocate(&new_root_id);
        node = new (ptr) leaf_node_t();
    }
    if(set_root_id(new_root_id)) {
        node_id = new_root_id;
        new_root_id = cache_t::null_block_id;
        state = update_acquiring_node;
        return btree_fsm_t::transition_ok;
    } else {
        node_id = btree_fsm_t::cache->get_superblock_id();
        return btree_fsm_t::transition_incomplete;
    }
}

template <class config_t>
typename btree_set_fsm<config_t>::transition_result_t btree_set_fsm<config_t>::do_update_inserting_root_on_split()
{
    assert(!serializer_t::is_block_id_null(new_split_root_id));
    printf("do_update_inserting_root_on_split\n");
    
    if(set_root_id(new_split_root_id)) {
        new_split_root_id = cache_t::null_block_id;
        state = update_acquiring_node;
        return btree_fsm_t::transition_ok;
    } else {
        return btree_fsm_t::transition_incomplete;
    }
}

template <class config_t>
typename btree_set_fsm<config_t>::transition_result_t btree_set_fsm<config_t>::do_update_acquiring_node()
{
    printf("do_update_acquiring_node\n");
    node = (node_t*)btree_fsm_t::cache->acquire(node_id, btree_fsm_t::netfsm);
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
        block_id_t loaded_block_id = node_id;
        if(!serializer_t::is_block_id_null(new_split_root_id))
            loaded_block_id = new_split_root_id;
        btree_fsm_t::cache->aio_complete(loaded_block_id, event->buf);
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

    // Go up the tree...
    while(res == btree_fsm_t::transition_ok && state == update_acquiring_node)
    {
        // Proactively split the node
        if(node->is_full()) {
            printf("Splitting node\n");
            int median;
            node_t *rnode;
            block_id_t rnode_id;
            split_node(node, &rnode, &rnode_id, &median);
            // Create a new root if we're splitting a root
            if(last_node == NULL) {
                printf("Allocating root on split\n");
                void *ptr = btree_fsm_t::cache->allocate(&new_split_root_id);
                last_node = new (ptr) internal_node_t();
                last_node_id = new_split_root_id;
            };
            last_node->insert(median, node_id, rnode_id);
            last_node_dirty = true;
                
            // Figure out where the key goes
            if(key < median) {
                // Left node and node are the same thing
                btree_fsm_t::cache->release(rnode_id, (void*)rnode, true, btree_fsm_t::netfsm);
            } else if(key >= median) {
                btree_fsm_t::cache->release(node_id, (void*)node, true, btree_fsm_t::netfsm);
                node = rnode;
                node_id = rnode_id;
            }

            if(!serializer_t::is_block_id_null(new_split_root_id)) {
                state == update_inserting_root_on_split;
                res = do_update_inserting_root_on_split();
                if(res != btree_fsm_t::transition_ok || state != update_acquiring_node)
                    break;
            }
        }
        // Insert the value, or move up the tree
        if(node->is_leaf()) {
            printf("Inserting into leaf\n");
            ((leaf_node_t*)node)->insert(key, value);
            btree_fsm_t::cache->release(node_id, (void*)node, true, btree_fsm_t::netfsm);
            state = update_complete;
            break;
        } else {
            // Release and update the last node
            if(!cache_t::is_block_id_null(last_node_id)) {
                btree_fsm_t::cache->release(last_node_id, (void*)last_node, last_node_dirty,
                                            btree_fsm_t::netfsm);
            }
            last_node = (internal_node_t*)node;
            last_node_id = node_id;
            last_node_dirty = false;
                
            // Look up the next node; update the node
            node_id = ((internal_node_t*)node)->lookup(key);
            res = do_update_acquiring_node();
        }
    }

    // Release the final node
    if(res == btree_fsm_t::transition_ok && state == update_complete) {
        if(!cache_t::is_block_id_null(last_node_id))
            btree_fsm_t::cache->release(last_node_id, (void*)last_node, last_node_dirty,
                                        btree_fsm_t::netfsm);
    }
        
    assert(res == btree_fsm_t::transition_incomplete ||
           res == btree_fsm_t::transition_ok);
    return res;
}

template <class config_t>
int btree_set_fsm<config_t>::set_root_id(block_id_t root_id) {
    block_id_t superblock_id = btree_fsm_t::cache->get_superblock_id();
    void *buf = btree_fsm_t::cache->acquire(superblock_id, btree_fsm_t::netfsm);
    if(buf == NULL) {
        return 0;
    }
    memcpy(buf, (void*)&root_id, sizeof(root_id));
    btree_fsm_t::cache->release(superblock_id, buf, true, btree_fsm_t::netfsm);
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

