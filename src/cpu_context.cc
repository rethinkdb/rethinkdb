
#include "config/args.hpp"
#include "config/code.hpp"
#include "cpu_context.hpp"
#include "event_queue.hpp"

static __thread cpu_context_t cpu_context;

cpu_context_t* get_cpu_context() {
    return &cpu_context;
}

int get_cpu_id() {
    return get_cpu_context()->event_queue->message_hub.current_cpu;
}