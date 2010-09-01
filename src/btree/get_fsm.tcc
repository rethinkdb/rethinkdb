#ifndef __BTREE_GET_FSM_TCC__
#define __BTREE_GET_FSM_TCC__

#include "utils.hpp"
#include "cpu_context.hpp"


//TODO: remove
#include "btree/internal_node.hpp"
#include "btree/leaf_node.hpp"

#include "btree/delete_expired_fsm.hpp"

template <class config_t>
typename btree_get_fsm<config_t>::transition_result_t btree_get_fsm<config_t>::do_acquire_superblock(event_t *event) {
    assert(state == acquire_superblock);

    if(event == NULL) {
        // First entry into the FSM. First, grab the transaction.
        transaction = cache->begin_transaction(rwi_read, NULL);
        assert(transaction); // Read-only transaction always begin immediately.

        // Now try to grab the superblock.
        last_buf = transaction->acquire(SUPERBLOCK_ID, rwi_read, this);
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
        node_id = ((btree_superblock_t*)last_buf->ptr())->root_block;
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
    if(node_id == NULL_BLOCK_ID) {
        last_buf->release();
        last_buf = NULL;
        this->status_code = btree_fsm<config_t>::S_NOT_FOUND;
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
        assert(next_node_id != NULL_BLOCK_ID);
        assert(next_node_id != SUPERBLOCK_ID);
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
        if (found && value.expired()) {
            delete_expired<config_t>(&key);
            found = false;
        }
        if (found && value.large_value()) {
            state = acquire_large_value;
        } else {
            state = lookup_complete;
        }
        this->status_code = found ? btree_fsm<config_t>::S_SUCCESS : btree_fsm<config_t>::S_NOT_FOUND;
        return btree_fsm_t::transition_ok;
    }
}

// TODO: Fix this when large_value's API supports it.
template <class config_t>
typename btree_get_fsm<config_t>::transition_result_t btree_get_fsm<config_t>::do_acquire_large_value(event_t *event) {
    assert(state == acquire_large_value);

    assert(value.large_value());

    if (!event) {
        large_value = new large_buf_t(transaction);
        // TODO: Put the cast in node.hpp
        large_value->acquire(* (block_id_t *) value.value(), value.value_size(), rwi_read, this);
        return btree_fsm_t::transition_incomplete;
    } else {
        assert(event->buf);
        assert(event->event_type == et_large_buf);
        assert(large_value == (large_buf_t *) event->buf);
        assert(large_value->state == large_buf_t::loaded);

        state = large_value_acquired;
        return btree_fsm_t::transition_ok;
    }
}

template <class config_t>
typename btree_get_fsm<config_t>::transition_result_t btree_get_fsm<config_t>::do_large_value_acquired(event_t *event) {
    assert(state == large_value_acquired);

    assert(large_value);
    assert(!event);
    assert(large_value->get_index_block_id() == value.lv_index_block_id());

    write_lv_msg = new write_large_value_msg<config_t>(large_value, req, this, this);
    write_lv_msg->return_cpu = get_cpu_context()->worker->workerid;
    write_lv_msg->send(this->return_cpu);
    // And now, we wait... For the socket to be ready for our value.
    return btree_fsm_t::transition_incomplete;
}

template <class config_t>
typename btree_get_fsm<config_t>::transition_result_t btree_get_fsm<config_t>::do_transition(event_t *event) {
    transition_result_t res = btree_fsm_t::transition_ok;

    // Make sure we've got either an empty or a cache event
    check("btree_fsm::do_transition - invalid event",
          !(!event || event->event_type == et_cache || event->event_type == et_large_buf));

    if (event && event->event_type == et_large_buf) assert(state == acquire_large_value);

    // Update the cache with the event
    if(event) {
        check("btree_get_fsm::do_transition - invalid event", event->op != eo_read);
        check("Could not complete AIO operation",
              event->result == 0 ||
              event->result == -1);
    }

    while (res == btree_fsm_t::transition_ok) {
        switch (state) {
            // First, acquire the superblock (to get root node ID)
            case acquire_superblock:
                res = do_acquire_superblock(event);
                break;

            // Then, acquire the root block
            case acquire_root:
                res = do_acquire_root(event);
                break;

            // Then, acquire the nodes, until we hit the leaf
            case acquire_node:
                res = do_acquire_node(event);
                break;

            // If the key is associated with a large value, acquire it.
            case acquire_large_value:
                res = do_acquire_large_value(event);
                break;

            // The large value is acquired; now we need to output it to the socket.
            case large_value_acquired:
                res = do_large_value_acquired(event);
                break;

            // XXX
            case large_value_writing:
                assert(!event);
                large_value->release();
                state = lookup_complete;
                break;

            // Finally, end our transaction. This should always succeed immediately.
            case lookup_complete: {
                bool committed __attribute__((unused)) = transaction->commit(NULL);
                assert(committed); /* Read-only commits always finish immediately. */
                res = btree_fsm_t::transition_complete;
                break;
           }
        }
        event = NULL;
    }

    return res;
}

template <class config_t>
void btree_get_fsm<config_t>::on_large_value_read() {
    this->step();
}

template <class config_t>
void btree_get_fsm<config_t>::on_large_value_completed(bool _success) {
    assert(_success);
    write_lv_msg = NULL;
    this->step();
}

#endif // __BTREE_GET_FSM_TCC__
