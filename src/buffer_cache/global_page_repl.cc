#include "buffer_cache/global_page_repl.hpp"

class thread_page_repl_t {
public:
    explicit thread_page_repl_t(global_page_repl_t *parent) : parent_(parent) {
        (void)parent_;
    }

private:
    global_page_repl_t *parent_;
    DISABLE_COPYING(thread_page_repl_t);
};

global_page_repl_t::global_page_repl_t(uint64_t memory_limit)
    : memory_limit_(memory_limit), thread_page_repl_(this) { }

global_page_repl_t::~global_page_repl_t() { }

void global_page_repl_t::change_memory_limit(uint64_t new_memory_limit) {
    memory_limit_ = new_memory_limit;
    // RSI: Give this a real implementation.
}
