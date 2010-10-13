#include "request_handler/request_handler.hpp"
#include "conn_fsm.hpp"

void request_handler_t::fill_value(byte *buf, unsigned int size, data_transferred_callback *cb) {
    // TODO: Put this where it belongs.
    conn_fsm->fill_external_buf(buf, size, cb);
}

void request_handler_t::write_value(const byte *buf, unsigned int size, data_transferred_callback *cb) {
    conn_fsm->send_external_buf(buf, size, cb);
}

void request_handler_t::request_complete() {
    // Notify the conn_fsm
    event_t e;
    bzero(&e, sizeof(e));
    e.state = conn_fsm;
    e.event_type = et_request_complete;
    conn_fsm->do_transition_and_handle_result(&e);
}
