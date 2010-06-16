
#ifndef __BTREE_SET_FSM_IMPL_HPP__
#define __BTREE_SET_FSM_IMPL_HPP__

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

template <class config_t>
void btree_set_fsm<config_t>::init_update(int _key, int _value) {
    key = _key;
    value = _value;
    state = acquire_superblock;
}

template <class config_t>
typename btree_set_fsm<config_t>::transition_result_t btree_set_fsm<config_t>::do_acquire_superblock(event_t *event)
{
    assert(state == acquire_superblock);

    buf_t *buf = NULL;
    if(event == NULL) {
        // First entry into the FSM. First, grab the transaction.
        transaction = cache->begin_transaction();

        // Now try to grab the superblock.
        block_id_t superblock_id = btree_fsm_t::get_cache()->get_superblock_id();
        buf = btree_fsm_t::get_cache()->acquire(btree_fsm_t::transaction, superblock_id, this);
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
        node = (node_t*)btree_fsm_t::get_cache()->acquire(btree_fsm_t::transaction, node_id, this);
    } else {
        // We already tried to acquire the root node, and here it is
        // via the cache notification.
        assert(event->buf);
        node = (node_t*)event->buf;
    }
    
    if(node == NULL) {
        return btree_fsm_t::transition_incomplete;
    } else {
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
        void *ptr = btree_fsm_t::get_cache()->allocate(btree_fsm_t::transaction, &node_id);
        node = new (ptr) leaf_node_t();
    }
    if(set_root_id(node_id, event)) {
        state = acquire_node;
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
        return btree_fsm_t::transition_ok;
    } else {
        return btree_fsm_t::transition_incomplete;
    }
}

template <class config_t>
typename btree_set_fsm<config_t>::transition_result_t btree_set_fsm<config_t>::do_acquire_node(event_t *event)
{
    if(event == NULL) {
        node = (node_t*)btree_fsm_t::get_cache()->acquire(btree_fsm_t::transaction, node_id, this);
    } else {
        assert(event->buf);
        node = (node_t*)event->buf;
    }

    if(node)
        return btree_fsm_t::transition_ok;
    else
        return btree_fsm_t::transition_incomplete;
}

template <class config_t>
typename btree_set_fsm<config_t>::transition_result_t btree_set_fsm<config_t>::do_transition(event_t *event)
{
    transition_result_t res = btree_fsm_t::transition_ok;

    // Make sure we've got either an empty or a cache event
    check("btree_fsm::do_transition - invalid event",
          event != NULL && event->event_type != et_cache);

    // Update the cache with the event
    if(event) {
        check("Could not complete AIO operation",
              event->result == 0 ||
              event->result == -1);

        if(event->op == eo_write) {
            nwrites--;
            // We always expect to get all AIO write cache
            // notifications back before we return
            // "transition_complete" status. This means that caches
            // with delayed writeback policies must emulate write
            // completion events before they actually happen on disk.
            if(res == btree_fsm_t::transition_ok && state == update_complete) {
                if(nwrites == 0) {
                    // End the transaction
                    btree_fsm_t::get_cache()->end_transaction(btree_fsm_t::transaction);
                    return btree_fsm_t::transition_complete;
                } else {
                    return btree_fsm_t::transition_incomplete;
                }
            } else {
                return btree_fsm_t::transition_incomplete;
            }
        } else if(event->op != eo_read) {
            check("btree_set_fsm::do_transition - invalid event type", 1);
        }
    }

    // First, acquire the superblock (to get root node ID)
    if(res == btree_fsm_t::transition_ok && state == acquire_superblock) {
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
    while(res == btree_fsm_t::transition_ok && state == acquire_node)
    {
        if(!node) {
            state = acquire_node;
            res = do_acquire_node(event);
            event = NULL;
            if(res != btree_fsm_t::transition_ok || state != acquire_node) {
                break;
            }
        }

        // Proactively split the node
        if(node->is_full()) {
            int median;
            node_t *rnode;
            block_id_t rnode_id;
            bool new_root = false;
            split_node(node, &rnode, &rnode_id, &median);
            // Create a new root if we're splitting a root
            if(last_node == NULL) {
                new_root = true;
                void *ptr = btree_fsm_t::get_cache()->allocate(btree_fsm_t::transaction, &last_node_id);
                last_node = new (ptr) internal_node_t();
            };
            last_node->insert(median, node_id, rnode_id);
            last_node_dirty = true;
                
            // Figure out where the key goes
            if(key < median) {
                // Left node and node are the same thing
                btree_fsm_t::get_cache()->release(btree_fsm_t::transaction, rnode_id, (void*)rnode, true, this);
                nwrites++;
            } else if(key >= median) {
                btree_fsm_t::get_cache()->release(btree_fsm_t::transaction, node_id, (void*)node, true, this);
                nwrites++;
                node = rnode;
                node_id = rnode_id;
            }
            node_dirty = true;

            if(new_root) {
                state = insert_root_on_split;
                res = do_insert_root_on_split(NULL);
                if(res != btree_fsm_t::transition_ok || state != acquire_node) {
                    break;
                }
            }
        }
        // Insert the value, or move up the tree
        if(node->is_leaf()) {
            ((leaf_node_t*)node)->insert(key, value);
            btree_fsm_t::get_cache()->release(btree_fsm_t::transaction, node_id, (void*)node, true, this);
            nwrites++;
            state = update_complete;
            res = btree_fsm_t::transition_ok;
            break;
        } else {
            // Release and update the last node
            if(!cache_t::is_block_id_null(last_node_id)) {
                btree_fsm_t::get_cache()->release(btree_fsm_t::transaction, last_node_id, (void*)last_node, last_node_dirty,
                                            this);
                last_node_dirty ? (nwrites++) : 0;
            }
            last_node = (internal_node_t*)node;
            last_node_id = node_id;
            last_node_dirty = node_dirty;
            node_dirty = false;
                
            // Look up the next node
            node_id = ((internal_node_t*)node)->lookup(key);
            node = NULL;
        }
    }

    // Release the final node
    if(res == btree_fsm_t::transition_ok && state == update_complete) {
        if(!cache_t::is_block_id_null(last_node_id)) {
            btree_fsm_t::get_cache()->release(btree_fsm_t::transaction, last_node_id, (void*)last_node, last_node_dirty,
                                        this);
            last_node_dirty ? (nwrites++) : 0;
            last_node_id = cache_t::null_block_id;
        }
    }

    return res;
}

template <class config_t>
int btree_set_fsm<config_t>::set_root_id(block_id_t root_id, event_t *event) {
    void *buf;
    if(event == NULL) {
        block_id_t superblock_id = btree_fsm_t::get_cache()->get_superblock_id();
        buf = btree_fsm_t::get_cache()->acquire(btree_fsm_t::transaction, superblock_id, this);
    } else {
        assert(event->buf);
        buf = event->buf;
    }
    
    if(buf) {
        memcpy(buf, (void*)&root_id, sizeof(root_id));
        btree_fsm_t::get_cache()->release(btree_fsm_t::transaction, btree_fsm_t::get_cache()->get_superblock_id(), buf, true, this);
        nwrites++;
        return 1;
    } else {
        return 0;
    }
}

template <class config_t>
void btree_set_fsm<config_t>::split_node(node_t *node, node_t **rnode,
                                         block_id_t *rnode_id, int *median) {
    void *ptr = btree_fsm_t::get_cache()->allocate(btree_fsm_t::transaction, rnode_id);
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

