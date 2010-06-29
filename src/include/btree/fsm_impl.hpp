
#ifndef __BTREE_FSM_IMPL_HPP__
#define __BTREE_FSM_IMPL_HPP__

// TODO: mapping only int->int, allow arbitrary key and value types
// TODO: ignoring duplicates, allow duplicate elements
// TODO: not thread safe, implement concurrency control methods
// TODO: multiple values require cursor/iterator mechanism
// TODO: consider redoing nodes with a visitor pattern to avoid ugly casts

template <class config_t>
typename btree_fsm<config_t>::block_id_t btree_fsm<config_t>::get_root_id(void *superblock_buf) {
    return *((block_id_t*)superblock_buf);
}

template <class config_t>
void btree_fsm<config_t>::on_block_available(buf_t *buf) {
    // TODO: Since we're changing the event system to untangle the
    // central processing at main.cc, we should have do_transition
    // accept a buf_t instead of event_t.
    
    // Convert buf to an event and call do_transition
    event_t event;
    bzero((void*)&event, sizeof(event));
    event.event_type = et_cache;
    event.op = eo_read;
    event.buf = buf;
    event.result = 1;
    transition_result_t res = do_transition(&event);
    if(res == transition_complete) {
        // Booooyahh, the operation completed. Notify whoever is
        // interested.
        if(on_complete) {
            on_complete(this);
        }
    }
}

#endif // __BTREE_FSM_IMPL_HPP__

