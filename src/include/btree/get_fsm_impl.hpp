
#ifndef __BTREE_GET_FSM_IMPL_HPP__
#define __BTREE_GET_FSM_IMPL_HPP__

#include "utils.hpp"

template <class config_t>
void btree_get_fsm<config_t>::init_lookup(int _key) {
    key = _key;
    state = lookup_acquiring_superblock;
}

template <class config_t>
typename btree_get_fsm<config_t>::transition_result_t btree_get_fsm<config_t>::do_lookup_acquiring_superblock() {
    assert(state == lookup_acquiring_superblock);
    
    if(get_root_id(&node_id) == 0) {
        return btree_fsm_t::transition_incomplete;
    } else {
        state = lookup_acquiring_root;
        return btree_fsm_t::transition_ok;
    }
}

template <class config_t>
typename btree_get_fsm<config_t>::transition_result_t btree_get_fsm<config_t>::do_lookup_acquiring_root() {
    assert(state == lookup_acquiring_root);
    
    // Make sure root exists
    if(cache->is_block_id_null(node_id)) {
        op_result = btree_not_found;
        return btree_fsm_t::transition_complete;
    }

    // Acquire the actual root node
    node = (node_t*)cache->acquire(node_id, netfsm);
    if(node == NULL) {
        return btree_fsm_t::transition_incomplete;
    } else {
        state = lookup_acquiring_node;
        return btree_fsm_t::transition_ok;
    }
}

template <class config_t>
typename btree_get_fsm<config_t>::transition_result_t btree_get_fsm<config_t>::do_lookup_acquiring_node() {
    assert(state == lookup_acquiring_node);
    assert(node);
    
    if(node->is_internal()) {
        block_id_t next_node_id = ((internal_node_t*)node)->lookup(key);
        cache->release(node_id, (void*)node, false, netfsm);
        node_id = next_node_id;
        node = (node_t*)cache->acquire(node_id, netfsm);
        if(node) {
            return btree_fsm_t::transition_ok;
        } else {
            return btree_fsm_t::transition_incomplete;
        }
    } else {
        int result = ((leaf_node_t*)node)->lookup(key, &value);
        cache->release(node_id, (void*)node, false, netfsm);
        op_result = result == 1 ? btree_found : btree_not_found;
        return btree_fsm_t::transition_complete;
    }
}

template <class config_t>
typename btree_get_fsm<config_t>::transition_result_t btree_get_fsm<config_t>::do_transition(event_t *event) {
    transition_result_t res = btree_fsm_t::transition_ok;
    // TODO: If event is null, we're initiating the first
    // transition. Is this the API we want?
    if(event == NULL || event->event_type == et_disk) {
        // Update the cache with the event
        if(event) {
            check("Could not complete AIO operation",
                  event->result == 0 ||
                  event->result == -1);
            cache->aio_complete(node_id, event->buf);
        }

        // First, acquire the superblock (to get root node ID)
        if(res == btree_fsm_t::transition_ok && state == lookup_acquiring_superblock)
            res = do_lookup_acquiring_superblock();
        
        // Then, acquire the root block
        if(res == btree_fsm_t::transition_ok && state == lookup_acquiring_root)
            res = do_lookup_acquiring_root();
        
        // Then, acquire the nodes, until we hit the leaf
        while(res == btree_fsm_t::transition_ok && state == lookup_acquiring_node) {
            res = do_lookup_acquiring_node();
        }

        // TODO: we consider state transitions reentrant, which means
        // we are likely to call acquire twice, but release only
        // once. We need to figure out how to get around that cleanly.
    } else {
        check("btree_get_fsm::do_transition - invalid event", 1);
    }
    assert(res == btree_fsm_t::transition_incomplete ||
           res == btree_get_fsm_complete);
    return res;
}

template <class config_t>
int btree_get_fsm<config_t>::get_root_id(block_id_t *root_id) {
    block_id_t superblock_id = cache->get_superblock_id();
    void *buf = cache->acquire(superblock_id, netfsm);
    if(buf == NULL) {
        return 0;
    }
    memcpy((void*)root_id, buf, sizeof(*root_id));
    cache->release(superblock_id, buf, false, netfsm);
    return 1;
}

#endif // __BTREE_GET_FSM_IMPL_HPP__

