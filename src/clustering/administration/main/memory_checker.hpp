// Copyright 2010-2016 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_MAIN_MEMORY_CHECKER_HPP_
#define CLUSTERING_ADMINISTRATION_MAIN_MEMORY_CHECKER_HPP_

#include <functional>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "arch/runtime/coroutines.hpp"
#include "arch/timing.hpp"
#include "clustering/administration/issues/memory.hpp"
#include "clustering/administration/main/cache_size.hpp"
#include "concurrency/auto_drainer.hpp"
#include "rdb_protocol/context.hpp"

class table_meta_client_t;

// memory_checker_t is created in serve.cc, and calls a repeating timer to
// Periodically check if we're using swap by looking at the proc file or system calls.
// If we're using swap, it creates an issue in a local issue tracker, and logs an error.
class memory_checker_t : private repeating_timer_callback_t {
public:
    memory_checker_t();

    memory_issue_tracker_t *get_memory_issue_tracker() {
        return &memory_issue_tracker;
    }
private:
    void do_check(auto_drainer_t::lock_t keepalive);
    void on_ring() final {
        coro_t::spawn_sometime(std::bind(&memory_checker_t::do_check,
                                         this,
                                         drainer.lock()));
    }
    memory_issue_tracker_t memory_issue_tracker;

    uint64_t refresh_timer;
    uint64_t swap_usage;

    bool print_log_message;

#if defined(__MACH__) || defined(_WIN32)
    bool first_check;
#endif

    // Timer must be destructed before drainer, because on_ring aquires a lock on drainer.
    auto_drainer_t drainer;
    repeating_timer_t timer;
};

#endif // CLUSTERING_ADMINISTRATION_MAIN_MEMORY_CHECKER_HPP_
