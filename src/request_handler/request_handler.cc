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

    // We delay in case we are being called from within parse_request. The conn_fsm
    // will get confused if we call do_transition() from within parse_request().

    request_count++;
    
    // If request count was nonzero before then we are already scheduled to be called
    if (request_count == 1) {
        call_later_on_this_cpu(this);
    }
}

void request_handler_t::on_cpu_switch() {
    int count = request_count;   // In case do_transition_and_handle_result() deletes us
    request_count = 0;
    for (; count > 0; count--) {
        // Notify the conn_fsm
        event_t e;
        bzero(&e, sizeof(e));
        e.state = conn_fsm;
        e.event_type = et_request_complete;
        conn_fsm->do_transition_and_handle_result(&e);
    }
}