
#ifndef __REQUEST_HANDLER_HPP__
#define __REQUEST_HANDLER_HPP__

#include "event.hpp"
#include "config/code.hpp"

struct event_t;

template<class config_t>
class request_handler_t {
public:
    typedef typename config_t::fsm_t fsm_t;
    
public:
    virtual ~request_handler_t() {}

    enum parse_result_t {
        op_malformed,
        op_partial_packet,
        op_req_shutdown,
        op_req_quit,
        op_req_complex
    };
    
    virtual parse_result_t parse_request(event_t *event) = 0;
    virtual void build_response(fsm_t *fsm) = 0;
};

#endif // __REQUEST_HANDLER_HPP__

