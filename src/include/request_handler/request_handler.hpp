
#ifndef __REQUEST_HANDLER_HPP__
#define __REQUEST_HANDLER_HPP__

#include "event.hpp"
#include "config/code.hpp"

struct event_t;
struct event_queue_t;

// The handler is instatiated once per queue.

template<class config_t>
class request_handler_t {
public:
    typedef typename config_t::conn_fsm_t conn_fsm_t;
    
public:
    request_handler_t(event_queue_t *eq) : event_queue(eq) {}
    virtual ~request_handler_t() {}

    enum parse_result_t {
        op_malformed,
        op_partial_packet,
        op_req_shutdown,
        op_req_quit,
        op_req_complex
    };
    
    virtual parse_result_t parse_request(event_t *event) = 0;
    virtual void build_response(conn_fsm_t *fsm) = 0;

protected:
    event_queue_t *event_queue;
};

#endif // __REQUEST_HANDLER_HPP__

