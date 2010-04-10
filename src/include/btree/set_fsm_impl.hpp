
#ifndef __BTREE_SET_FSM_IMPL_HPP__
#define __BTREE_SET_FSM_IMPL_HPP__

template <class config_t>
void btree_set_fsm<config_t>::init_update(int _key, int _value) {
    key = _key;
    value = _value;
    state = update_acquiring_superblock;
}

template <class config_t>
transition_result_t btree_set_fsm<config_t>::do_update_acquiring_superblock()
{
    assert(state == update_acquiring_superblock);
    
    if(get_root_id(&node_id) == 0) {
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
transition_result_t btree_set_fsm<config_t>::do_update_acquiring_root()
{
    // If root didn't exist, we should have ended up in
    // do_update_inserting_root()
    assert(!serializer_t::is_block_id_null(node_id));

    // Acquire the actual root node
    node = (node_t*)btree_fsm_t::cache->acquire(node_id, btree_fsm_t::netfsm);
    if(node == NULL) {
        return btree_fsm_t::transition_incomplete;
    } else {
        state = lookup_acquiring_node;
        return btree_fsm_t::transition_ok;
    }
}

template <class config_t>
transition_result_t btree_set_fsm<config_t>::do_update_inserting_root()
{
    // If this is the first time we're entering this function,
    // allocate the root (otherwise, the root has already been
    // allocated here, and we just need to set its id in the metadata)
    if(serializer_t::is_block_id_null(node_id)) {
        void *ptr = cache->allocate(&node_id);
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
transition_result_t btree_set_fsm<config_t>::do_update_inserting_root_on_split()
{
}

template <class config_t>
transition_result_t btree_set_fsm<config_t>::do_update_acquiring_node()
{
}

template <class config_t>
transition_result_t btree_set_fsm<config_t>::do_transition(event_t *event)
{
    // Do shared transition operations
    transition_result_t res = btree_fsm_t::do_transition(event);
    
    // Acquire the superblock
    // Grab the root (or create it if it's missing)

    // Go up the tree...
    do {
        // Proactively split the node (and create a new root if we're splitting a root)
        // Insert the value, or move up the tree
    } while(1);

    // Release the final node
        
    assert(res == btree_fsm_t::transition_incomplete ||
           res == btree_fsm_t::transition_ok);
    return res;
}

template <class config_t>
int btree_fsm<config_t>::set_root_id(block_id_t root_id) {
    block_id_t superblock_id = btree_fsm_t::cache->get_superblock_id();
    void *buf = btree_fsm_t::cache->acquire(superblock_id, btree_fsm_t::netfsm);
    if(buf == NULL) {
        return 0;
    }
    memcpy(buf, (void*)root_id, sizeof(*root_id));
    btree_fsm_t::cache->release(superblock_id, buf, true, btree_fsm_t::netfsm);
    return 1;
}

#endif // __BTREE_SET_FSM_IMPL_HPP__

