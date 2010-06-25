
#ifndef __BTREE_DELETE_FSM_IMPL_HPP__
#define __BTREE_DELETE_FSM_IMPL_HPP__

#include "utils.hpp"
#include "cpu_context.hpp"

template <class config_t>
void btree_delete_fsm<config_t>::init_delete(int _key) {
    key = _key;
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
typename btree_delete_fsm<config_t>::transition_result_t btree_delete_fsm<config_t>::do_acquire_superblock(event_t *event) {
    assert(state == acquire_superblock);

    buf_t *buf = NULL;
    if(event == NULL) {
        // First entry into the FSM; try to grab the superblock.
        block_id_t superblock_id = cache->get_superblock_id();
        buf = transaction->acquire(superblock_id, rwi_read, this);
    } else {
        // We already tried to grab the superblock, and we're getting
        // a cache notification about it.
        assert(event->buf);
        buf = (buf_t *)event->buf;
    }
    
    if(buf) {
        // Got the superblock buffer (either right away or through
        // cache notification). Grab the root id, and move on to
        // acquiring the root.
        node_id = btree_fsm_t::get_root_id(buf->ptr());
        buf->release();
        state = acquire_root;
        return btree_fsm_t::transition_ok;
    } else {
        // Can't get the superblock buffer right away. Let's wait for
        // the cache notification.
        return btree_fsm_t::transition_incomplete;
    }
}

template <class config_t>
typename btree_delete_fsm<config_t>::transition_result_t btree_delete_fsm<config_t>::do_acquire_root(event_t *event) {
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
typename btree_delete_fsm<config_t>::transition_result_t btree_delete_fsm<config_t>::do_acquire_node(event_t *event) {
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
    node_t *node = buf->node();
    if(node->is_internal()) {
        block_id_t next_node_id = ((internal_node_t*)node)->lookup(key);
        /* XXX XXX Cannot release until the next acquire succeeds for locks! */
        buf->release();
        node_id = next_node_id;
        buf = transaction->acquire(node_id, rwi_read, this);
        if(node) {
            return btree_fsm_t::transition_ok;
        } else {
            return btree_fsm_t::transition_incomplete;
        }
    } else {
        int result = ((leaf_node_t*)node)->lookup(key, &value);
        ((leaf_node_t*)node)->remove(key);
        buf->release();
        state = delete_complete;
        op_result = result == 1 ? btree_found : btree_not_found;
        return btree_fsm_t::transition_ok;
    }
}

template <class config_t>
typename btree_delete_fsm<config_t>::transition_result_t btree_delete_fsm<config_t>::do_transition(event_t *event) {
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
        
    // Then, acquire the nodes, until we hit the leaf
    while(res == btree_fsm_t::transition_ok && state == acquire_node) {
        if(!buf) {
            state = acquire_node;
            res = do_acquire_node(event);
            event = NULL;
            if(res != btree_fsm_t::transition_ok || state != acquire_node) {
                break;
            }
        }

        res = do_acquire_node(event);
        event = NULL;
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

#endif // __BTREE_DELETE_FSM_IMPL_HPP__

