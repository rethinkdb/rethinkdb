
#ifndef __CPU_CONTEXT_HPP__
#define __CPU_CONTEXT_HPP__

#include "arch/arch.hpp"
struct worker_t;

// A context structure that stores information relevant to each CPU
// core. Each thread can access the structure for its core through
// get_cpu_context() function.
struct cpu_context_t {
    worker_t *worker;
    event_queue_t *event_queue;   // Same as worker->event_queue
};

cpu_context_t* get_cpu_context();
int get_cpu_id();   // Mostly for debugging

#endif // __CPU_CONTEXT_HPP__

