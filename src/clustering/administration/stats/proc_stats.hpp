// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_STATS_PROC_STATS_HPP_
#define CLUSTERING_ADMINISTRATION_STATS_PROC_STATS_HPP_

#include "concurrency/auto_drainer.hpp"
#include "perfmon/perfmon.hpp"

class proc_stats_collector_t : public home_thread_mixin_debug_only_t {
public:
    explicit proc_stats_collector_t(perfmon_collection_t *stats);

private:
    class instantaneous_stats_collector_t : public perfmon_t {
    public:
        instantaneous_stats_collector_t();
        void *begin_stats();
        void visit_stats(void *);
        ql::datum_t end_stats(void *);
    private:
        ticks_t start_time;
    };

    instantaneous_stats_collector_t instantaneous_stats_collector;

    perfmon_multi_membership_t stats_membership;
};

#endif /* CLUSTERING_ADMINISTRATION_STATS_PROC_STATS_HPP_ */
