
#ifndef __BTREE_GET_FSM_IMPL_HPP__
#define __BTREE_GET_FSM_IMPL_HPP__

#include "utils.hpp"
#include "cpu_context.hpp"

template <class config_t>
void btree_get_fsm<config_t>::init_lookup(int _key) {
    key = _key;
    state = acquire_superblock;
}

template <class config_t>
typename btree_get_fsm<config_t>::transition_result_t btree_get_fsm<config_t>::do_acquire_superblock(event_t *event) {
    assert(state == acquire_superblock);

    buf_t *buf = NULL;
    if(event == NULL) {
        // First entry into the FSM. First, grab the transaction.
        btree_fsm_t::transaction = btree_fsm_t::get_cache()->begin_transaction();

        // Now try to grab the superblock.
        block_id_t superblock_id = btree_fsm_t::get_cache()->get_superblock_id();
        buf = transaction->acquire(superblock_id, this);
    } else {
        // We already tried to grab the superblock, and we're getting
        // a cache notification about it.
        assert(event->buf);
        buf = event->buf;
    }
    
    if(buf) {
        // Got the superblock buffer (either right away or through
        // cache notification). Grab the root id, and move on to
        // acquiring the root.
        node_id = btree_fsm_t::get_root_id(buf);
        btree_fsm_t::get_cache()->release(btree_fsm_t::transaction, btree_fsm_t::get_cache()->get_superblock_id(), buf, false, this);
        state = acquire_root;
        return btree_fsm_t::transition_ok;
    } else {
        // Can't get the superblock buffer right away. Let's wait for
        // the cache notification.
        return btree_fsm_t::transition_incomplete;
    }
}

template <class config_t>
typename btree_get_fsm<config_t>::transition_result_t btree_get_fsm<config_t>::do_acquire_root(event_t *event) {
    assert(state == acquire_root);
    
    // Make sure root exists
    if(cache_t::is_block_id_null(node_id)) {
        op_result = btree_not_found;
        state = lookup_complete;
        // End the transaction
        btree_fsm_t::get_cache()->end_transaction(btree_fsm_t::transaction);
        return btree_fsm_t::transition_complete;
    }

    if(event == NULL) {
        // Acquire the actual root node
        node = (node_t*)btree_fsm_t::get_cache()->acquire(btree_fsm_t::transaction, node_id, this);
    } else {
        // We already tried to grab the root, and we're getting a
        // cache notification about it.
        assert(event->buf);
        node = (node_t*)event->buf;
    }
    
    if(node == NULL) {
        // Can't grab the root right away. Wait for a cache event.
        return btree_fsm_t::transition_incomplete;
    } else {
        // Got the root, move on to grabbing the node
        state = acquire_node;
        return btree_fsm_t::transition_ok;
    }
}

template <class config_t>
typename btree_get_fsm<config_t>::transition_result_t btree_get_fsm<config_t>::do_acquire_node(event_t *event) {
    assert(state == acquire_node);
    // Either we already have the node (then event should be NULL), or
    // we don't have the node (in which case we asked for it before,
    // and it should be getting to us via an event)
    assert((node && !event) || (!node && event));

    if(!node) {
        // We asked for a node before and couldn't get it right
        // away. It must be in the event.
        assert(event && event->buf);
        node = (node_t*)event->buf;
    }
    assert(node);
    if(node->is_internal()) {
        block_id_t next_node_id = ((internal_node_t*)node)->lookup(key);
        btree_fsm_t::get_cache()->release(btree_fsm_t::transaction, node_id, (void*)node, false, this);
        node_id = next_node_id;
        node = (node_t*)btree_fsm_t::get_cache()->acquire(btree_fsm_t::transaction, node_id, this);
        if(node) {
            return btree_fsm_t::transition_ok;
        } else {
            return btree_fsm_t::transition_incomplete;
        }
    } else {
        int result = ((leaf_node_t*)node)->lookup(key, &value);
        btree_fsm_t::get_cache()->release(btree_fsm_t::transaction, node_id, (void*)node, false, this);
        state = lookup_complete;
        op_result = result == 1 ? btree_found : btree_not_found;
        // End the transaction
        btree_fsm_t::get_cache()->end_transaction(btree_fsm_t::transaction);
        return btree_fsm_t::transition_complete;
    }
}

template <class config_t>
typename btree_get_fsm<config_t>::transition_result_t btree_get_fsm<config_t>::do_transition(event_t *event) {
    transition_result_t res = btree_fsm_t::transition_ok;

    // Make sure we've got either an empty or a cache event
    check("btree_fsm::do_transition - invalid event",
          !(!event || event->event_type == et_cache));

    // Update the cache with the event
    if(event) {
        check("btree_set_fsm::do_transition - invalid event", event->op != eo_read);
        check("Could not complete AIO operation",
              event->result == 0 ||
              event->result == -1);
    }
    
    // First, acquire the superblock (to get root node ID)
    if(res == btree_fsm_t::transition_ok && state == acquire_superblock) {
        res = do_acquire_superblock(event);
        event = NULL;
    }
        
    // Then, acquire the root block
    if(res == btree_fsm_t::transition_ok && state == acquire_root) {
        res = do_acquire_root(event);
        event = NULL;
    }
        
    // Then, acquire the nodes, until we hit the leaf
    while(res == btree_fsm_t::transition_ok && state == acquire_node) {
        res = do_acquire_node(event);
        event = NULL;
    }

    return res;
}

#endif // __BTREE_GET_FSM_IMPL_HPP__

