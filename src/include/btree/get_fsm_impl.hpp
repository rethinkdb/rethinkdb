
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
        // We're temporarily assigning superblock id to node_id
        // variable, so that when we get the next disk completion
        // event, we can notify the cache.
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
    if(cache_t::is_block_id_null(node_id)) {
        op_result = btree_not_found;
        return btree_fsm_t::transition_complete;
    }

    // Acquire the actual root node
    node = (node_t*)btree_fsm_t::cache->acquire(node_id, this);
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

    if(!node) {
        node = (node_t*)btree_fsm_t::cache->acquire(node_id, this);
    }
    if(node->is_internal()) {
        block_id_t next_node_id = ((internal_node_t*)node)->lookup(key);
        btree_fsm_t::cache->release(node_id, (void*)node, false, this);
        node_id = next_node_id;
        node = (node_t*)btree_fsm_t::cache->acquire(node_id, this);
        if(node) {
            return btree_fsm_t::transition_ok;
        } else {
            return btree_fsm_t::transition_incomplete;
        }
    } else {
        int result = ((leaf_node_t*)node)->lookup(key, &value);
        btree_fsm_t::cache->release(node_id, (void*)node, false, this);
        op_result = result == 1 ? btree_found : btree_not_found;
        return btree_fsm_t::transition_complete;
    }
}

template <class config_t>
typename btree_get_fsm<config_t>::transition_result_t btree_get_fsm<config_t>::do_transition(event_t *event) {
    transition_result_t res = btree_fsm_t::transition_ok;

    // Make sure we've got either an empty or a disk event
    check("btree_fsm::do_transition - invalid event",
          event != NULL && event->event_type != et_disk);

    // Update the cache with the event
    if(event) {
        check("btree_set_fsm::do_transition - invalid event", event->op != eo_read);
        check("Could not complete AIO operation",
              event->result == 0 ||
              event->result == -1);

        // TODO: right now we assume that block_id_t is the same thing
        // as event->offset. In the future (once we fix the event
        // system), the block_id_t state should really be stored
        // within the event state. Currently, we cast the offset to
        // block_id_t, but it's a big hack, we need to get rid of this
        // later.
        btree_fsm_t::cache->aio_complete((block_id_t)event->offset, event->buf, false);
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

    return res;
}

#endif // __BTREE_GET_FSM_IMPL_HPP__

