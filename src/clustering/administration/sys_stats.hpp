#ifndef CLUSTERING_ADMINISTRATION_SYS_STATS_HPP_
#define CLUSTERING_ADMINISTRATION_SYS_STATS_HPP_

#include "perfmon/perfmon.hpp"

#include <string>

/* Class to get system statistics, such as disk space usage.
Similar to proc_stats_collector_t, but not based on /proc. */

class sys_stats_collector_t : public home_thread_mixin_debug_only_t {
public:
    explicit sys_stats_collector_t(const std::string &path, perfmon_collection_t *stats);

private:
    // similar to proc_stats_collector_t::instantaneous_stats_collector_t
    class instantaneous_stats_collector_t : public perfmon_t {
    public:
        instantaneous_stats_collector_t(const std::string &path);
        void *begin_stats();
        void visit_stats(void *);
        perfmon_result_t *end_stats(void *);
    private:
        std::string filepath;
    };

    instantaneous_stats_collector_t instantaneous_stats_collector;
    perfmon_membership_t stats_membership;
};

#endif /* CLUSTERING_ADMINISTRATION_SYS_STATS_HPP_ */
