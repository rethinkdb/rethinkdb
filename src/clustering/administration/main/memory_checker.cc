// Copyright 2010-2016 RethinkDB, all rights reserved.
#include "clustering/administration/main/memory_checker.hpp"

#include <math.h>

#include "clustering/administration/metadata.hpp"
#include "clustering/table_manager/table_meta_client.hpp"
#include "logger.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/pseudo_time.hpp"

static const int64_t delay_time = 60*1000;
static const int64_t reset_checks = 60;

memory_checker_t::memory_checker_t(rdb_context_t *_rdb_ctx) :
    rdb_ctx(_rdb_ctx),
    refresh_timer(0),
    swap_usage(0),
    print_log_message(true)
#if defined(__MACH__) || defined(_WIN32)
    ,first_check(true)
#endif
    ,timer(delay_time, this)
{
    rassert(rdb_ctx != nullptr);
    coro_t::spawn_sometime(std::bind(&memory_checker_t::do_check,
                                     this,
                                     drainer.lock()));
}

void memory_checker_t::do_check(auto_drainer_t::lock_t keepalive) {
    ql::env_t env(rdb_ctx,
                  ql::return_empty_normal_batches_t::NO,
                  keepalive.get_drain_signal(),
                  ql::global_optargs_t(),
                  nullptr);

    uint64_t new_swap_usage = get_used_swap();

#if defined(__MACH__)
    // This is because mach won't give us the swap used by our process.
    if (first_check) {
        swap_usage = new_swap_usage;
        first_check = false;
    }
#endif

#if defined(__MACH__)
    const std::string error_message =
        "Data from a process on this server"
        " has been placed into swap memory in the past hour."
        " If the data is from RethinkDB, this may impact performance.";
#else
    const std::string error_message =
        "Some RethinkDB data on this server"
        " has been placed into swap memory in the past hour."
        " This may impact performance.";
#endif

    if (new_swap_usage > swap_usage) {
        // We've started using more swap
        if (print_log_message) {
#if defined(__MACH__)
            logWRN("Data from a process on this server"
                   " has been placed into swap memory."
                   " If the data is from RethinkDB, this may impact performance.");
#else
            logWRN("Some RethinkDB data on this server"
                   " has been placed into swap memory."
                   " This may impact performance.");
#endif
            print_log_message = false;
        }
        refresh_timer = reset_checks;
        memory_issue_tracker.report_error(error_message);
    } else if (refresh_timer == 0) {
        // We haven't put anything in swap for 1 hour.
        memory_issue_tracker.report_success();
        print_log_message = true;
    }

    swap_usage = new_swap_usage;

    if (refresh_timer > 0) {
        --refresh_timer;
    }
}

