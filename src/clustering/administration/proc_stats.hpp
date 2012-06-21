#ifndef CLUSTERING_ADMINISTRATION_PROC_STATS_HPP_
#define CLUSTERING_ADMINISTRATION_PROC_STATS_HPP_

#include "concurrency/auto_drainer.hpp"
#include "perfmon/perfmon.hpp"

class proc_stats_collector_t : public home_thread_mixin_t {
public:
    explicit proc_stats_collector_t(perfmon_collection_t *stats);

private:
    class instantaneous_stats_collector_t : public perfmon_t {
    public:
        explicit instantaneous_stats_collector_t(perfmon_collection_t *stats);
        void *begin_stats();
        void visit_stats(void *);
        void end_stats(void *, perfmon_result_t *);
    private:
        ticks_t start_time;
    };

    void collect_periodically(auto_drainer_t::lock_t);

    instantaneous_stats_collector_t instantaneous_stats_collector;

    perfmon_sampler_t
        cpu_thread_user,
        cpu_thread_system,
        cpu_thread_combined,
        cpu_global_combined,
        net_global_received,
        net_global_sent,
        memory_faults;

    auto_drainer_t drainer;
};

#endif /* CLUSTERING_ADMINISTRATION_PROC_STATS_HPP_ */
