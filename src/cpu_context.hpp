
#ifndef __CPU_CONTEXT_HPP__
#define __CPU_CONTEXT_HPP__

#include "event_queue.hpp"

// A context structure that stores information relevant to each CPU
// core. Each thread can access the structure for its core through
// get_cpu_context() function.
struct cpu_context_t {
    event_queue_t *event_queue;
};

cpu_context_t* get_cpu_context();

#endif // __CPU_CONTEXT_HPP__

