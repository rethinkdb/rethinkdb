#include "get_fsm.hpp"
#include "utils.hpp"

#include "btree/internal_node.hpp"
#include "btree/leaf_node.hpp"

#include "btree/delete_expired_fsm.hpp"

/* The get_fsm has two paths it takes: one for large values and one for small ones. For large
values, it holds onto the large value buffer while it goes back to the request handler's core
and delivers the large value. Then it returns again to the cache's core and frees the value,
and finally goes to the request handler's core again to free itself. For small values, it
duplicates the value into an internal buffer and then goes to the request handler's core,
delivers the value, and frees itself all in one trip. If the value is not found, we take
a third path that is basically the same as the one for small values. */

btree_get_fsm_t::transition_result_t btree_get_fsm_t::do_acquire_superblock(event_t *event) {
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
        node_id = ((const btree_superblock_t*)last_buf->get_data_read())->root_block;
        assert(node_id != SUPERBLOCK_ID);
        state = acquire_root;
        return btree_fsm_t::transition_ok;
    } else {
        // Can't get the superblock buffer right away. Let's wait for
        // the cache notification.
        return btree_fsm_t::transition_incomplete;
    }
}

btree_get_fsm_t::transition_result_t btree_get_fsm_t::do_acquire_root(event_t *event) {
    assert(state == acquire_root);

    // Make sure root exists
    if(node_id == NULL_BLOCK_ID) {
        last_buf->release();
        last_buf = NULL;
        
        // Commit transaction now because we won't be returning to this core
        bool committed __attribute__((unused)) = transaction->commit(NULL);
        assert(committed);   // Read-only transactions complete immediately
        
        state = deliver_not_found_notification;
        if (continue_on_cpu(home_cpu, this)) return btree_fsm_t::transition_ok;
        else return btree_fsm_t::transition_incomplete;
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

btree_get_fsm_t::transition_result_t btree_get_fsm_t::do_acquire_node(event_t *event) {

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

    node_handler::validate(cache->get_block_size(), node_handler::node(buf->get_data_read()));

    // Release the previous buffer
    last_buf->release();
    last_buf = NULL;

    const node_t *node = node_handler::node(buf->get_data_read());
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
        buf = NULL;
        
        if (found && value.expired()) {
            delete_expired(&key, store);
            found = false;
        }
        
        if (!found) {
            // Commit transaction now because we won't be returning to this core
            bool committed __attribute__((unused)) = transaction->commit(NULL);
            assert(committed);   // Read-only transactions complete immediately
            
            state = deliver_not_found_notification;
            if (continue_on_cpu(home_cpu, this)) return btree_fsm_t::transition_ok;
            else return btree_fsm_t::transition_incomplete;
            
        } else if (value.is_large()) {
            // Don't commit transaction yet because we need to keep holding onto
            // the large buf until it's been read.
        
            state = acquire_large_value;
            return btree_fsm_t::transition_ok;
            
        } else {
            
            // Commit transaction now because we won't be returning to this core
            bool committed __attribute__((unused)) = transaction->commit(NULL);
            assert(committed);   // Read-only transactions complete immediately
            
            state = deliver_small_value;
            if (continue_on_cpu(home_cpu, this)) return btree_fsm_t::transition_ok;
            else return btree_fsm_t::transition_incomplete;
        }
    }
}

// TODO: Fix this when large_value's API supports it.
btree_get_fsm_t::transition_result_t btree_get_fsm_t::do_acquire_large_value(event_t *event) {
    assert(state == acquire_large_value);

    assert(value.is_large());

    if (!event) {
        large_value = new large_buf_t(transaction);

        large_value->acquire(value.lb_ref(), rwi_read, this);
        return btree_fsm_t::transition_incomplete;
    } else {
        assert(event->buf);
        assert(event->event_type == et_large_buf);
        assert(large_value == (large_buf_t *) event->buf);
        assert(large_value->state == large_buf_t::loaded);

        state = deliver_large_value;
        return btree_fsm_t::transition_ok;
    }
}

btree_get_fsm_t::transition_result_t btree_get_fsm_t::do_deliver_large_value(event_t *event) {
    assert(state == deliver_large_value);

    assert(large_value);
    assert(!event);
    assert(large_value->get_root_ref().block_id == value.lb_ref().block_id);

    for (int64_t i = 0; i < large_value->get_num_segments(); i++) {
        uint16_t size;
        const void *data = large_value->get_segment(i, &size);
        value_buffers.add_buffer(size, data);
    }
    
    state = write_large_value;
    if (continue_on_cpu(home_cpu, this)) return btree_fsm_t::transition_ok;
    else return btree_fsm_t::transition_incomplete;
}

btree_get_fsm_t::transition_result_t btree_get_fsm_t::do_write_large_value(event_t *event) {
    assert(state == write_large_value);
    
    in_callback_value_call = true;
    value_was_copied = false;
    callback->value(&value_buffers, this, value.mcflags(), 0);
    in_callback_value_call = false;
    
    state = return_after_deliver_large_value;
    if (value_was_copied) return btree_fsm_t::transition_ok;
    else return btree_fsm_t::transition_incomplete;
}

void btree_get_fsm_t::have_copied_value() {
    value_was_copied = true;
    if (!in_callback_value_call) do_transition(NULL);
}

btree_get_fsm_t::transition_result_t btree_get_fsm_t::do_return_after_deliver_large_value(event_t *event) {
    assert(state == return_after_deliver_large_value);
    
    state = free_large_value;
    if (continue_on_cpu(slice->home_cpu, this)) return btree_fsm_t::transition_ok;
    else return btree_fsm_t::transition_incomplete;
}

btree_get_fsm_t::transition_result_t btree_get_fsm_t::do_free_large_value(event_t *event) {
    assert(state == free_large_value);
    
    large_value->release();
    delete large_value;
    
    bool committed __attribute__((unused)) = transaction->commit(NULL);
    assert(committed);   // Read-only transactions complete immediately
    
    state = delete_self;
    if (continue_on_cpu(home_cpu, this)) return btree_fsm_t::transition_ok;
    else return btree_fsm_t::transition_incomplete;
}

btree_get_fsm_t::transition_result_t btree_get_fsm_t::do_deliver_small_value(event_t *event) {
    assert(state == deliver_small_value);
    
    in_callback_value_call = true;
    value_was_copied = false;
    value_buffers.add_buffer(value.value_size(), value.value());
    callback->value(&value_buffers, this, value.mcflags(), 0);
    in_callback_value_call = false;
    
    state = delete_self;
    if (value_was_copied) return btree_fsm_t::transition_ok;
    else return btree_fsm_t::transition_incomplete;
}

btree_get_fsm_t::transition_result_t btree_get_fsm_t::do_deliver_not_found_notification(event_t *event) {
    assert(state == deliver_not_found_notification);
    
    callback->not_found();
    
    state = delete_self;
    return btree_fsm_t::transition_ok;
}

void btree_get_fsm_t::do_transition(event_t *event) {
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
            
            // Go to the core where the cache is
            case go_to_cache_core:
                state = acquire_superblock;
                if (continue_on_cpu(slice->home_cpu, this)) res = btree_fsm_t::transition_ok;
                else res = btree_fsm_t::transition_incomplete;
                break;
            
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

            // The large value is acquired; now we need to output it to the socket. Go to
            // the core of the thing that requested the value.
            case deliver_large_value:
                res = do_deliver_large_value(event);
                break;

            // Call the callback to deliver the large value.
            case write_large_value:
                res = do_write_large_value(event);
                break;
            
            // Return to free the large value.
            case return_after_deliver_large_value:
                res = do_return_after_deliver_large_value(event);
                break;
            
            // Free the large value, then go back to free ourself
            case free_large_value:
                res = do_free_large_value(event);
                break;
            
            // If we had a small value, we go straight to here after acquire_node. Go to
            // the core of the thing that requested the value, with the value in our
            // 'value' attribute.
            case deliver_small_value:
                res = do_deliver_small_value(event);
                break;
            
            case deliver_not_found_notification:
                res = do_deliver_not_found_notification(event);
                break;
            
            // Delete ourself to reclaim the memory
            case delete_self:
                delete this;
                return;
        }
        event = NULL;
    }
}
