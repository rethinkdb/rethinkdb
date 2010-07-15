#ifndef __BTREE_SET_FSM_TCC__
#define __BTREE_SET_FSM_TCC__

#include "cpu_context.hpp"

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

// TODO: change rwi_write to rwi_intent followed by rwi_upgrade where
// relevant.
template <class config_t>
void btree_set_fsm<config_t>::init_update(btree_key *_key, byte *data, unsigned int length, btree_set_kind _set_kind) {
    set_kind = _set_kind;
    keycpy(key, _key);
    value->size = length;
    memcpy(&value->contents, data, length);
    state = start_transaction;
}

template <class config_t>
typename btree_set_fsm<config_t>::transition_result_t btree_set_fsm<config_t>::do_start_transaction(event_t *event) {
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
typename btree_set_fsm<config_t>::transition_result_t btree_set_fsm<config_t>::do_acquire_superblock(event_t *event)
{
    assert(state == acquire_superblock);

    if(event == NULL) {
        // First entry into the FSM; try to grab the superblock.
        block_id_t superblock_id = btree_fsm_t::get_cache()->get_superblock_id();
        sb_buf = transaction->acquire(superblock_id, rwi_write, this);
    } else {
        // We already tried to grab the superblock, and we're getting
        // a cache notification about it.
        assert(event->buf);
        sb_buf = (buf_t *)event->buf;
    }

    if(sb_buf) {
        // Got the superblock buffer (either right away or through
        // cache notification). Grab the root id, and move on to
        // acquiring the root.
        node_id = btree_fsm_t::get_root_id(sb_buf->ptr());
        if(cache_t::is_block_id_null(node_id))
            state = insert_root;
        else
            state = acquire_root;
        return btree_fsm_t::transition_ok;
    } else {
        // Can't get the superblock buffer right away. Let's wait for
        // the cache notification.
        return btree_fsm_t::transition_incomplete;
    }
}

template <class config_t>
typename btree_set_fsm<config_t>::transition_result_t btree_set_fsm<config_t>::do_acquire_root(event_t *event)
{
    // If root didn't exist, we should have ended up in
    // do_insert_root()
    assert(!cache_t::is_block_id_null(node_id));

    if(event == NULL) {
        // Acquire the actual root node
        buf = transaction->acquire(node_id, rwi_write, this);
    } else {
        // We already tried to acquire the root node, and here it is
        // via the cache notification.
        assert(event->buf);
        buf = (buf_t *)event->buf;
    }
    
    if(buf == NULL) {
        return btree_fsm_t::transition_incomplete;
    } else {
        node_handler::validate((node_t *)buf->ptr());
        state = acquire_node;
        return btree_fsm_t::transition_ok;
    }
}

template <class config_t>
typename btree_set_fsm<config_t>::transition_result_t btree_set_fsm<config_t>::do_insert_root(event_t *event)
{
    // If this is the first time we're entering this function,
    // allocate the root (otherwise, the root has already been
    // allocated here, and we just need to set its id in the metadata)
    if(cache_t::is_block_id_null(node_id)) {
        buf = transaction->allocate(&node_id);
        leaf_node_handler::init((leaf_node_t *)buf->ptr());
        buf->set_dirty();
    }
    if(set_root_id(node_id, event)) {
        state = acquire_node;
        sb_buf->release();
        sb_buf = NULL;
        return btree_fsm_t::transition_ok;
    } else {
        return btree_fsm_t::transition_incomplete;
    }
}

template <class config_t>
typename btree_set_fsm<config_t>::transition_result_t btree_set_fsm<config_t>::do_insert_root_on_split(event_t *event)
{
    if(set_root_id(last_node_id, event)) {
        state = acquire_node;
        sb_buf->release();
        sb_buf = NULL;
        return btree_fsm_t::transition_ok;
    } else {
        return btree_fsm_t::transition_incomplete;
    }
}

template <class config_t>
typename btree_set_fsm<config_t>::transition_result_t btree_set_fsm<config_t>::do_acquire_node(event_t *event)
{
    if(event == NULL) {
        buf = transaction->acquire(node_id, rwi_write, this);
    } else {
        assert(event->buf);
        buf = (buf_t*)event->buf;
    }

    if(buf) {
        node_handler::validate((node_t *)buf->ptr());
        return btree_fsm_t::transition_ok;
    }
    else
        return btree_fsm_t::transition_incomplete;
}

template <class config_t>
typename btree_set_fsm<config_t>::transition_result_t btree_set_fsm<config_t>::do_transition(event_t *event)
{
    transition_result_t res = btree_fsm_t::transition_ok;

    // Make sure we've got either an empty or a cache event
    check("btree_fsm::do_transition - invalid event", event != NULL &&
          event->event_type != et_cache && event->event_type != et_commit);

    // Update the cache with the event
    if(event && event->event_type == et_cache) {
        check("Could not complete AIO operation",
              event->result == 0 ||
              event->result == -1);

        check("btree_set_fsm::do_transition - invalid event type", event->op != eo_read);
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

    // Then, acquire the root block (or create it if it's missing)
    if(res == btree_fsm_t::transition_ok && (state == acquire_root || state == insert_root)) {
        if(state == acquire_root)
            res = do_acquire_root(event);
        else if(state == insert_root)
            res = do_insert_root(event);
        event = NULL;
    }

    // If we were inserting root on split, do that
    if(res == btree_fsm_t::transition_ok && state == insert_root_on_split) {
        res = do_insert_root_on_split(event);
        event = NULL;
    }

    // If we were acquiring a node, do that
    // Go up the tree...
    while(res == btree_fsm_t::transition_ok && state == acquire_node) {
        if(!buf) {
            state = acquire_node;
            res = do_acquire_node(event);
            event = NULL;
            if(res != btree_fsm_t::transition_ok || state != acquire_node) {
                break;
            }
        }

        // Proactively split the node
        node_t *node = (node_t *)buf->ptr();
        bool full;
                
        if (node_handler::is_leaf(node)) {
            full = leaf_node_handler::is_full((leaf_node_t *)node, key, value);
        } else {
            full = internal_node_handler::is_full((internal_node_t *)node);
        }
        if(full) {
            char memory[sizeof(btree_key) + MAX_KEY_SIZE];
            btree_key *median = (btree_key *)memory;
            buf_t *rbuf;
            internal_node_t *last_node;
            block_id_t rnode_id;
            bool new_root = false;
            split_node(buf, &rbuf, &rnode_id, median);
            // Create a new root if we're splitting a root
            if(last_buf == NULL) {
                new_root = true;
                last_buf = transaction->allocate(&last_node_id);
                last_node = (internal_node_t *)last_buf->ptr();
                internal_node_handler::init(last_node);
                last_buf->set_dirty();
            } else {
                last_node = (internal_node_t *)last_buf->ptr();
            }
            bool success = internal_node_handler::insert(last_node, median, node_id, rnode_id);
            check("could not insert internal btree node", !success);
            last_buf->set_dirty();
                
            // Figure out where the key goes
            if(sized_strcmp(key->contents, key->size, median->contents, median->size) <= 0) {
                // Left node and node are the same thing
                rbuf->release();
                rbuf = NULL;
            } else {
                buf->release();
                buf = rbuf;
                rbuf = NULL;
                node = (node_t *)buf->ptr();
                node_id = rnode_id;
            }

            if(new_root) {
                state = insert_root_on_split;
                res = do_insert_root_on_split(NULL);
                if(res != btree_fsm_t::transition_ok || state != acquire_node) {
                    break;
                }
            }
        }

        // Release the superblock, if we haven't already
        if(sb_buf) {
            sb_buf->release();
            sb_buf = NULL;
        }
        
        // Insert the value, or move up the tree
        if(node_handler::is_leaf(node)) {

            // TODO: write unit tests for checking add, replace, incr, decr
            char value_memory[MAX_IN_NODE_VALUE_SIZE+sizeof(btree_value)];
            btree_value *current_value = (btree_value*)value_memory;
            long long new_val = 0;
            unsigned long long cur_val = 0;
            // If it's an add operation, check that the key doesn't exist.
            // If it's a replace operation, check that the key does exist.
            bool key_found = false;
            if(set_kind == btree_set_kind_add || set_kind == btree_set_kind_replace) {
                key_found = leaf_node_handler::lookup((leaf_node_t*)node, key, current_value);
            }else if(set_kind == btree_set_kind_incr || set_kind == btree_set_kind_decr) {
                key_found = leaf_node_handler::lookup((leaf_node_t*)node, key, current_value);
                new_val = atoll(value->contents);
                cur_val = strtoull(current_value->contents, NULL, 10);
            }
           /*  NOTE: memcached actually does a few things differently:
            *   - If you say `decr 1 -50`, memcached will set 1 to 0 no matter
            *      what it's value is. We on the other hand will add 50 to 1.
            *
            *   - Also, if you say 'incr 1 -50' in memcached and the value
            *     goes below 0, memcached will wrap around. We just set the value to 0.
            */
            if (key_found && set_kind == btree_set_kind_decr) {
                // we underflowed and wrapped around while subtracting, set to zero.
                if ((signed long long)cur_val - new_val < 0) 
                    cur_val = 0;
                else
                    cur_val -= new_val;
            } else if(key_found && set_kind == btree_set_kind_incr) {
                if ((signed long long)cur_val + new_val < 0)
                    cur_val = 0;
                else
                    cur_val += new_val;
            }
            
            if (set_kind == btree_set_kind_set ||
                (set_kind == btree_set_kind_add && !key_found) ||
                (set_kind == btree_set_kind_replace && key_found) ||
                (set_kind == btree_set_kind_decr && key_found) ||
                (set_kind == btree_set_kind_incr && key_found))
            {
                if (set_kind == btree_set_kind_decr || set_kind == btree_set_kind_incr) {
                    int chars_written = sprintf(value->contents, "%llu", cur_val);
                    value->size = chars_written;
                }
                bool success = leaf_node_handler::insert((leaf_node_t*)node, key, value);
                check("could not insert leaf btree node", !success);
                set_was_successful = true;
                buf->set_dirty();
            }
            buf->release();
            buf = NULL;
            state = update_complete;
            res = btree_fsm_t::transition_ok;
            break;
        } else {
            // Release and update the last node
            if(!cache_t::is_block_id_null(last_node_id)) {
                last_buf->release();
                last_buf = NULL;
            }
            last_buf = buf;
            buf = NULL;
            last_node_id = node_id;
            node_id = cache_t::null_block_id;
                
            // Look up the next node
            node_id = internal_node_handler::lookup((internal_node_t*)node, key);
            assert(!cache_t::is_block_id_null(node_id));
            assert(node_id != cache->get_superblock_id());
        }
    }

    // Finalize the operation
    if(res == btree_fsm_t::transition_ok && state == update_complete) {
        // Release the final node
        if(!cache_t::is_block_id_null(last_node_id)) {
            last_buf->release();
            last_buf = NULL;
            last_node_id = cache_t::null_block_id;
        }

        // End the transaction
        bool committed = transaction->commit(this);
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

template <class config_t>
int btree_set_fsm<config_t>::set_root_id(block_id_t root_id, event_t *event) {
    memcpy(sb_buf->ptr(), (void*)&root_id, sizeof(root_id));
    sb_buf->set_dirty();
    return 1;
}

template <class config_t>
void btree_set_fsm<config_t>::split_node(buf_t *buf, buf_t **rbuf,
                                         block_id_t *rnode_id, btree_key *median) {
    buf_t *res = transaction->allocate(rnode_id);
    if(node_handler::is_leaf((node_t *)buf->ptr())) {
    	leaf_node_t *node = (leaf_node_t *)buf->ptr();
        leaf_node_t *rnode = (leaf_node_t *)res->ptr();
        leaf_node_handler::split(node, rnode, median);
        buf->set_dirty();
        res->set_dirty();
    } else {
    	internal_node_t *node = (internal_node_t *)buf->ptr();
        internal_node_t *rnode = (internal_node_t *)res->ptr();
        internal_node_handler::split(node, rnode, median);
        buf->set_dirty();
        res->set_dirty();
    }
    *rbuf = res;
}

template<class config_t>
btree_key* btree_set_fsm<config_t>::get_key() {
    return key;
}

template<class config_t>
btree_value* btree_set_fsm<config_t>::get_value() {
    return value;
}
template<class config_t>
btree_set_kind btree_set_fsm<config_t>::get_set_kind() {
    return set_kind;
}

#ifndef NDEBUG
template <class config_t>
void btree_set_fsm<config_t>::deadlock_debug(void) {
    printf("set-fsm %p {\n", this);
    printf("    buf = %p\n", buf);
    printf("    last_buf = %p\n", last_buf);
    printf("    sb_buf = %p\n", sb_buf);
    printf("    node_id = %d\n", (int)node_id);
    printf("    last_node_id = %d\n", (int)last_node_id);
    const char *st_name;
    switch(state) {
        case uninitialized: st_name = "uninitialized"; break;
        case start_transaction: st_name = "start_transaction"; break;
        case acquire_superblock: st_name = "acquire_superblock"; break;
        case acquire_root: st_name = "acquire_root"; break;
        case insert_root: st_name = "insert_root"; break;
        case insert_root_on_split: st_name = "insert_root_on_split"; break;
        case acquire_node: st_name = "acquire_node"; break;
        case update_complete: st_name = "update_complete"; break;
        case committing: st_name = "committing"; break;
        default: st_name = "WTF"; break;
    }
    printf("    state = %s\n", st_name);
    printf("}\n");
}
#endif

#endif // __BTREE_SET_FSM_TCC__

