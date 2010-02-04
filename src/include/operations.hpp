
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

    enum result_t {
        malformed_command,
        incomplete_command,
        shutdown_server,
        quit_connection,
        command_success_no_response,
        command_success_response_ready,
        command_aio_wait
    };

    virtual result_t process_command(event_t *event) = 0;
    virtual void complete_op(btree_fsm_t *btree_fsm, event_t *event) = 0;
};

#endif // __OPERATIONS_HPP__

