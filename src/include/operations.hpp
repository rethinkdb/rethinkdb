
#ifndef __OPERATIONS_HPP__
#define __OPERATIONS_HPP__

#include "event.hpp"

struct event_t;

class operations_t {
public:
    virtual ~operations_t() {}

    enum result_t {
        malformed_command,
        incomplete_command,
        shutdown_server,
        quit_connection,
        command_success_no_response,
        command_success_response_ready
    };

    virtual int process_command(event_t *event) = 0;
};

#endif // __OPERATIONS_HPP__

