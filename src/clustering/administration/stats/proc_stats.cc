// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/stats/proc_stats.hpp"

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
        &instantaneous_stats_collector, static_cast<const char *>(NULL)) {
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

ql::datum_t proc_stats_collector_t::instantaneous_stats_collector_t::end_stats(void *) {
    ql::datum_object_builder_t builder;

    // Basic process stats (version, pid, uptime)
    struct timespec now = clock_monotonic();

    builder.overwrite("uptime", ql::datum_t(static_cast<double>(now.tv_sec - start_time)));
    builder.overwrite("timestamp", ql::datum_t(format_time(now).c_str()));

    builder.overwrite("version", ql::datum_t(RETHINKDB_VERSION));
    builder.overwrite("pid", ql::datum_t(static_cast<double>(getpid())));

    return std::move(builder).to_datum();
}
