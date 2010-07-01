
#include "config/args.hpp"
#include "config/code.hpp"
#include "cpu_context.hpp"

static __thread cpu_context_t cpu_context;

cpu_context_t* get_cpu_context() {
    return &cpu_context;
}

