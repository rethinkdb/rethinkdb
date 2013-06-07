// Copyright 2010-2012 RethinkDB, all rights reserved.
#define __STDC_FORMAT_MACROS

#include "clustering/administration/proc_stats.hpp"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "arch/timing.hpp"
#include "utils.hpp"

proc_stats_collector_t::proc_stats_collector_t(perfmon_collection_t *stats) :
    instantaneous_stats_collector(),
    stats_membership(stats,
        &instantaneous_stats_collector, NULL,
        NULLPTR) {
}

proc_stats_collector_t::instantaneous_stats_collector_t::instantaneous_stats_collector_t() {
    struct timespec now = clock_monotonic();
    start_time = now.tv_sec;
}

void *proc_stats_collector_t::instantaneous_stats_collector_t::begin_stats() {
    return NULL;
}

void proc_stats_collector_t::instantaneous_stats_collector_t::visit_stats(void *) {
    /* Do nothing; the things we need to get can be gotten on any thread */
}

scoped_ptr_t<perfmon_result_t> proc_stats_collector_t::instantaneous_stats_collector_t::end_stats(void *) {
    scoped_ptr_t<perfmon_result_t> result = perfmon_result_t::alloc_map_result();

    // Basic process stats (version, pid, uptime)
    struct timespec now = clock_monotonic();

    result->insert("uptime", new perfmon_result_t(strprintf("%" PRIu64, now.tv_sec - start_time)));
    result->insert("timestamp", new perfmon_result_t(format_time(now)));

    result->insert("version", new perfmon_result_t(std::string(RETHINKDB_VERSION)));
    result->insert("pid", new perfmon_result_t(strprintf("%d", getpid())));

    return result;
}
