#include "fsm.hpp"
#include "btree/key_value_store.hpp"

// TODO: allow multiple values per key
// TODO: add cursor/iterator mechanism

void btree_fsm_t::on_block_available(buf_t *buf) {
    // TODO: We should get rid of event_t and pass a buf_t instead?
    
    // Convert buf to an event and call do_transition
    event_t event;
    bzero((void*)&event, sizeof(event));
    event.event_type = et_cache;
    event.op = eo_read;
    event.buf = buf;
    event.result = 1;
    transition_result_t res = do_transition(&event);
    if(res == transition_complete) done();
}

void btree_fsm_t::on_large_buf_available(large_buf_t *large_buf) {
    event_t event;
    bzero((void*)&event, sizeof(event));
    event.event_type = et_large_buf;
    event.op = eo_read;
    event.buf = large_buf;
    event.result = 1;
    if (do_transition(&event) == transition_complete) done();
}

void btree_fsm_t::on_txn_begin(transaction_t *txn) {
    assert(transaction == NULL);
    event_t event;
    memset(&event, 0, sizeof(event));
    event.event_type = et_cache; // TODO(NNW): This is pretty hacky
    event.buf = txn;
    event.result = 1;
    if (do_transition(&event) == transition_complete) done();
}

void btree_fsm_t::on_txn_commit(transaction_t *txn) {
    event_t event;
    memset(&event, 0, sizeof(event));
    event.event_type = et_commit;
    event.buf = txn;
    if (do_transition(&event) == transition_complete) done();
}

void btree_fsm_t::step() { // XXX Rename and abstract.
    if (do_transition(NULL) == transition_complete) done();
}

void btree_fsm_t::done() {
    if (continue_on_cpu(home_cpu, this)) on_cpu_switch();
}

void btree_fsm_t::on_cpu_switch() {
    
    if (is_finished()) {
        // We have just been sent back to the core that created us
        if (callback) {
            callback->on_btree_fsm_complete(this);
        } else {
            // A btree not associated with any request.
            delete this;
        }
        
    } else {
        // We have just been sent to the core on which to work
        step();
    }
}
