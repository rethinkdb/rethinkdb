#include "fsm.hpp"
#include "btree/key_value_store.hpp"

// TODO: allow multiple values per key
// TODO: add cursor/iterator mechanism

void btree_fsm_t::on_cpu_switch() {
    do_transition(NULL);
}

void btree_fsm_t::on_block_available(buf_t *buf) {
    // TODO: We should get rid of event_t and pass a buf_t instead?
    
    // Convert buf to an event and call do_transition
    event_t event;
    bzero((void*)&event, sizeof(event));
    event.event_type = et_cache;
    event.op = eo_read;
    event.buf = buf;
    event.result = 1;
    do_transition(&event);
}

void btree_fsm_t::on_large_buf_available(large_buf_t *large_buf) {
    event_t event;
    bzero((void*)&event, sizeof(event));
    event.event_type = et_large_buf;
    event.op = eo_read;
    event.buf = large_buf;
    event.result = 1;
    do_transition(&event);
}

void btree_fsm_t::on_txn_begin(transaction_t *txn) {
    assert(transaction == NULL);
    event_t event;
    memset(&event, 0, sizeof(event));
    event.event_type = et_cache; // TODO(NNW): This is pretty hacky
    event.buf = txn;
    event.result = 1;
    do_transition(&event);
}

void btree_fsm_t::on_txn_commit(transaction_t *txn) {
    event_t event;
    memset(&event, 0, sizeof(event));
    event.event_type = et_commit;
    event.buf = txn;
    do_transition(&event);
}

void btree_fsm_t::step() { // XXX Rename and abstract.
    do_transition(NULL);
}
