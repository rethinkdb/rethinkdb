// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_PROC_STATS_HPP_
#define CLUSTERING_ADMINISTRATION_PROC_STATS_HPP_

#include "concurrency/auto_drainer.hpp"
#include "perfmon/perfmon.hpp"

/* Class to represent and parse the contents of /proc/[pid]/stat */
struct proc_pid_stat_t {
    int pid;
    char name[500];
    char state;
    int ppid, pgrp, session, tty_nr, tpgid;
    unsigned int flags;
    uint64_t minflt, cminflt, majflt, cmajflt, utime, stime;
    int64_t cutime, cstime, priority, nice, num_threads, itrealvalue;
    uint64_t starttime;
    uint64_t vsize;
    int64_t rss;
    uint64_t rsslim, startcode, endcode, startstack, kstkesp, kstkeip, signal, blocked,
        sigignore, sigcatch, wchan, nswap, cnswap;
    int exit_signal, processor;
    unsigned int rt_priority, policy;
    uint64_t delayacct_blkio_ticks;
    uint64_t guest_time;
    int64_t cguest_time;

    static proc_pid_stat_t for_pid(pid_t pid);
    static proc_pid_stat_t for_pid_and_tid(pid_t pid, pid_t tid);
private:
    void read_from_file(const char * path);
};

class proc_stats_collector_t : public home_thread_mixin_debug_only_t {
public:
    explicit proc_stats_collector_t(perfmon_collection_t *stats);

private:
    class instantaneous_stats_collector_t : public perfmon_t {
    public:
        instantaneous_stats_collector_t();
        void *begin_stats();
        void visit_stats(void *);
        perfmon_result_t *end_stats(void *);
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

    perfmon_multi_membership_t stats_membership;
};

#endif /* CLUSTERING_ADMINISTRATION_PROC_STATS_HPP_ */
