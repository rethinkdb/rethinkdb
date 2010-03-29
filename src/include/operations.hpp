
#ifndef __OPERATIONS_HPP__
#define __OPERATIONS_HPP__

#include "event.hpp"
#include "config/code.hpp"

struct event_t;

class operations_t {
public:
    typedef code_config_t::fsm_t fsm_t;
    typedef code_config_t::btree_fsm_t btree_fsm_t;
    
public:
    virtual ~operations_t() {}

    enum initial_result_t {
        op_malformed,
        op_partial_packet,
        op_completed,
        op_initiated
    };

    enum complete_result_t {
        op_shutdown,
        op_quit,
        op_response_ready
    };

    virtual initial_result_t initiate_op(event_t *event) = 0;
    virtual complete_result_t complete_op(event_t *event) = 0;
};

#endif // __OPERATIONS_HPP__

