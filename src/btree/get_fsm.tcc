
#ifndef __BTREE_GET_FSM_TCC__
#define __BTREE_GET_FSM_TCC__

#include "utils.hpp"
#include "cpu_context.hpp"

#include "btree/delete_fsm.hpp"

//TODO: remove
#include "btree/internal_node.hpp"
#include "btree/leaf_node.hpp"

template <class config_t>
typename btree_get_fsm<config_t>::transition_result_t btree_get_fsm<config_t>::do_acquire_superblock(event_t *event) {
    assert(state == acquire_superblock);

    if(event == NULL) {
        // First entry into the FSM. First, grab the transaction.
        transaction = cache->begin_transaction(rwi_read, NULL);
        assert(transaction); // Read-only transaction always begin immediately.

        // Now try to grab the superblock.
        block_id_t superblock_id = cache->get_superblock_id();
        last_buf = transaction->acquire(superblock_id, rwi_read, this);
    } else {
        // We already tried to grab the superblock, and we're getting
        // a cache notification about it.
        assert(event->buf);
        last_buf = (buf_t *)event->buf;
    }
    
    if(last_buf) {
        // Got the superblock buffer (either right away or through
        // cache notification). Grab the root id, and move on to
        // acquiring the root.
        node_id = btree_fsm_t::get_root_id(last_buf->ptr());
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
        last_buf->release();
        last_buf = NULL;
        op_result = btree_not_found;
        state = lookup_complete;
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
typename btree_get_fsm<config_t>::transition_result_t btree_get_fsm<config_t>::do_acquire_node(event_t *event) {
    assert(state == acquire_node);
    // Either we already have the node (then event should be NULL), or
    // we don't have the node (in which case we asked for it before,
    // and it should be getting to us via an event)
    assert((buf && !event) || (!buf && event));

    if(!buf) {
        // We asked for a node before and couldn't get it right
        // away. It must be in the event.
        assert(event && event->buf);
        buf = (buf_t*)event->buf;
    }
    assert(buf);

    node_handler::validate((node_t *)buf->ptr());

    // Release the previous buffer
    last_buf->release();
    last_buf = NULL;
    
    node_t *node = (node_t *)buf->ptr();
    if(node_handler::is_internal(node)) {
        block_id_t next_node_id = internal_node_handler::lookup((internal_node_t*)node, &key);
        assert(!cache_t::is_block_id_null(next_node_id));
        assert(next_node_id != cache->get_superblock_id());
        last_buf = buf;
        node_id = next_node_id;
        buf = transaction->acquire(node_id, rwi_read, this);
        if(buf) {
            return btree_fsm_t::transition_ok;
        } else {
            return btree_fsm_t::transition_incomplete;
        }
    } else {
        bool found = leaf_node_handler::lookup((leaf_node_t*)node, &key, &value);
        buf->release();
        state = lookup_complete;
        if (found && value.expired()) {
            delete_expired<config_t>(&key);
            found = false;
        }
        op_result = found ? btree_found : btree_not_found;
        return btree_fsm_t::transition_ok;
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
        check("btree_get_fsm::do_transition - invalid event", event->op != eo_read);
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

    // Finally, end our transaction.  This should always succeed immediately.
    if (res == btree_fsm_t::transition_ok && state == lookup_complete) {
        bool committed __attribute__((unused)) = transaction->commit(NULL);
        assert(committed); /* Read-only commits always finish immediately. */
        res = btree_fsm_t::transition_complete;
    }

    return res;
}

#endif // __BTREE_GET_FSM_TCC__

