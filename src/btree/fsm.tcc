#ifndef __BTREE_FSM_TCC__
#define __BTREE_FSM_TCC__

// TODO: allow multiple values per key
// TODO: add cursor/iterator mechanism

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

template <class config_t>
void btree_fsm<config_t>::on_large_buf_available(large_buf_t *large_buf) {
    event_t event;
    bzero((void*)&event, sizeof(event));
    event.event_type = et_large_buf;
    event.op = eo_read;
    event.buf = large_buf;
    event.result = 1;
    transition_result_t res = do_transition(&event);
    assert(res != transition_complete); // We should only be acquiring a large buf in mid-operation.
}

template <class config_t>
void btree_fsm<config_t>::on_txn_begin(transaction_t *txn) {
    assert(transaction == NULL);
    event_t event;
    memset(&event, 0, sizeof(event));
    event.event_type = et_cache; // TODO(NNW): This is pretty hacky
    event.buf = txn;
    event.result = 1;
    if (do_transition(&event) == transition_complete && on_complete)
        on_complete(this);
}

template <class config_t>
void btree_fsm<config_t>::on_txn_commit(transaction_t *txn) {
    event_t event;
    memset(&event, 0, sizeof(event));
    event.event_type = et_commit;
    event.buf = txn;
    if (do_transition(&event) == transition_complete && on_complete)
        on_complete(this);
}

template <class config_t>
void btree_fsm<config_t>::step() { // XXX Rename and abstract.
    if (do_transition(NULL) == transition_complete && on_complete) {
        on_complete(this);
    }
}

#endif // __BTREE_FSM_TCC__
