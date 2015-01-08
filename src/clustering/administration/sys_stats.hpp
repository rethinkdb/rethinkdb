// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_SYS_STATS_HPP_
#define CLUSTERING_ADMINISTRATION_SYS_STATS_HPP_

#include <string>

#include "perfmon/perfmon.hpp"
#include "utils.hpp"

/* Class to get system statistics, such as disk space usage.
Similar to proc_stats_collector_t, but not based on /proc. */

class sys_stats_collector_t : public home_thread_mixin_debug_only_t {
public:
    sys_stats_collector_t(const base_path_t &path, perfmon_collection_t *stats);

private:
    // similar to proc_stats_collector_t::instantaneous_stats_collector_t
    class instantaneous_stats_collector_t : public perfmon_t {
    public:
        explicit instantaneous_stats_collector_t(const base_path_t &path);
        void *begin_stats();
        void visit_stats(void *);
        ql::datum_t end_stats(void *);
    private:
        const base_path_t base_path;

        DISABLE_COPYING(instantaneous_stats_collector_t);
    };

    instantaneous_stats_collector_t instantaneous_stats_collector;
    perfmon_membership_t stats_membership;

    DISABLE_COPYING(sys_stats_collector_t);
};

#endif /* CLUSTERING_ADMINISTRATION_SYS_STATS_HPP_ */
