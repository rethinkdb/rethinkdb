#include "buffer_cache/global_page_repl.hpp"

class thread_page_repl_t {
public:
    explicit thread_page_repl_t(global_page_repl_t *parent,
                                uint64_t thread_memory_limit)
        : parent_(parent),
          thread_memory_limit_(thread_memory_limit),
          thread_usage_(0) {
        (void)parent_;
        (void)thread_usage_;
    }

    void change_thread_memory_limit(uint64_t new_thread_memory_limit) {
        thread_memory_limit_ = new_thread_memory_limit;
        // RSI: Give this a real implementation.
    }

private:
    global_page_repl_t *const parent_;

    // How much memory this thread's caches are allowed to use.
    uint64_t thread_memory_limit_;
    // How much memory this thread's caches are actually using.
    uint64_t thread_usage_;
    DISABLE_COPYING(thread_page_repl_t);
};

global_page_repl_t::global_page_repl_t(uint64_t memory_limit)
    : memory_limit_(memory_limit), thread_page_repl_(this, memory_limit / get_num_db_threads()) { }

global_page_repl_t::~global_page_repl_t() { }

void global_page_repl_t::change_memory_limit(uint64_t new_memory_limit) {
    memory_limit_ = new_memory_limit;
    // RSI: Give this a real implementation.
}
