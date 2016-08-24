// Copyright 2010-2016 RethinkDB, all rights reserved.
#include "clustering/administration/main/memory_checker.hpp"

#include <math.h>
#include <sys/resource.h>

#include "clustering/administration/metadata.hpp"
#include "clustering/table_manager/table_meta_client.hpp"
#include "logger.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/pseudo_time.hpp"

static const int64_t delay_time = 60*1000;
static const int64_t reset_checks = 10;

static const int64_t practice_runs = 2;

memory_checker_t::memory_checker_t() :
    checks_until_reset(0),
    swap_usage(0),
    print_log_message(true),
    practice_runs_remaining(practice_runs),
    timer(delay_time, this)
{
    coro_t::spawn_sometime(std::bind(&memory_checker_t::do_check,
                                     this,
                                     drainer.lock()));
}

void memory_checker_t::do_check(UNUSED auto_drainer_t::lock_t keepalive) {
#if defined(__MACH__) || defined(_WIN32)
    size_t new_swap_usage = 0;
#else
    struct rusage current_usage;

    int res = getrusage(RUSAGE_SELF, &current_usage);

    if (res == -1) {
        logWRN("Error in swap usage detection: %d\n", get_errno());
        return;
    }
    size_t new_swap_usage = current_usage.ru_majflt;
#endif

    if (practice_runs_remaining == practice_runs) {
        swap_usage = new_swap_usage;
    }

    const std::string error_message =
        "RethinkDB has been accessing a lot of swap memory in the past ten"
        " minutes. This may impact performace.";

    if (new_swap_usage > swap_usage + 200 && practice_runs_remaining == 0) {
        // We've started using more swap
        if (print_log_message) {
            logWRN("RethinkDB has been accessing a lot of swap memory in the past ten"
                   " minutes. This may impact performace.");

            print_log_message = false;
        }
        checks_until_reset = reset_checks;
        memory_issue_tracker.report_error(error_message);
    } else if (checks_until_reset == 0) {
        // We haven't had more than 200 major page faults per minute for the last 10m.
        memory_issue_tracker.report_success();
        print_log_message = true;
    }

    swap_usage = new_swap_usage;

    if (checks_until_reset > 0) {
        --checks_until_reset;
    }
    if (practice_runs_remaining > 0) {
        --practice_runs_remaining;
    }
}

