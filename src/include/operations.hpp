
#ifndef __OPERATIONS_HPP__
#define __OPERATIONS_HPP__

#include "event.hpp"

struct event_t;

class operations_t {
public:
    virtual ~operations_t() {}

    virtual int process_command(event_t *event) = 0;
};

#endif // __OPERATIONS_HPP__

