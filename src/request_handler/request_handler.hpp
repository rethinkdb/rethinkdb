#ifndef __REQUEST_HANDLER_HPP__
#define __REQUEST_HANDLER_HPP__

#include "event.hpp"
#include "config/alloc.hpp"
#include "arch/arch.hpp"

struct event_t;

/*
initiate_conn_fsm_transition() is in main.cc. There should be a better way for the request handler
to notify the connection FSM that there is a response in the send buffer waiting to be sent, but
that's something for another day.
TODO: Come up with a better way for request handler to notify conn_fsm as part of redoing the
callback system.
*/

class conn_fsm_t;
class data_transferred_callback;
class fill_large_value_msg_t;

class request_handler_t {

public:
    explicit request_handler_t()
        : fill_lv_msg(NULL) {}
    virtual ~request_handler_t() {}

    enum parse_result_t {
        op_malformed,
        op_partial_packet,
        op_req_quit,
        op_req_complex,
        op_req_parallelizable,
        op_req_send_now
    };

    virtual parse_result_t parse_request(event_t *event) = 0;

    void fill_value(byte *buf, unsigned int size, data_transferred_callback *cb);
    void write_value(byte *buf, unsigned int size, data_transferred_callback *cb);

    void request_complete();
    
    conn_fsm_t *conn_fsm;

    fill_large_value_msg_t *fill_lv_msg;
};

#endif // __REQUEST_HANDLER_HPP__
