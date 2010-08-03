
#ifndef __CPU_CONTEXT_HPP__
#define __CPU_CONTEXT_HPP__

#include "worker_pool.hpp"

// A context structure that stores information relevant to each CPU
// core. Each thread can access the structure for its core through
// get_cpu_context() function.
struct cpu_context_t {
    worker_t *worker;
};

cpu_context_t* get_cpu_context();

#endif // __CPU_CONTEXT_HPP__

