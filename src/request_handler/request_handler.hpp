
#ifndef __REQUEST_HANDLER_HPP__
#define __REQUEST_HANDLER_HPP__

#include "event.hpp"
#include "config/code.hpp"

struct event_t;
struct event_queue_t;

/*
initiate_conn_fsm_transition() is in main.cc. There should be a better way for the request handler
to notify the connection FSM that there is a response in the send buffer waiting to be sent, but
that's something for another day.
TODO: Come up with a better way for request handler to notify conn_fsm as part of redoing the
callback system.
*/
void initiate_conn_fsm_transition(event_queue_t *event_queue, event_t *event);

template<class config_t>
class request_handler_t {
public:
    typedef typename config_t::cache_t cache_t;
    typedef typename config_t::conn_fsm_t conn_fsm_t;
    typedef typename config_t::btree_fsm_t btree_fsm_t;
    
public:
    explicit request_handler_t(event_queue_t *eq, conn_fsm_t *conn_fsm)
        : event_queue(eq), conn_fsm(conn_fsm) {}
    virtual ~request_handler_t() {}

    enum parse_result_t {
        op_malformed,
        op_partial_packet,
        op_req_shutdown,
        op_req_quit,
        op_req_complex,
        op_req_parallelizable,
        op_req_send_now
    };
    
    virtual parse_result_t parse_request(event_t *event) = 0;
    
    void request_complete() {
        // Notify the conn_fsm
        event_t e;
        bzero(&e, sizeof(e));
        e.state = conn_fsm;
        e.event_type = et_request_complete;
        initiate_conn_fsm_transition(event_queue, &e);
    }
    
    event_queue_t *event_queue;
    conn_fsm_t *conn_fsm;
};

#endif // __REQUEST_HANDLER_HPP__
