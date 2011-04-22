#ifndef __ARCH_LINUX_DISK_STATS_HPP__
#define __ARCH_LINUX_DISK_STATS_HPP__

#include "perfmon.hpp"
#include <boost/function.hpp>

extern perfmon_duration_sampler_t pm_io_reads, pm_io_writes;

template<class payload_t>
struct stats_diskmgr_t {

    struct action_t : public payload_t {
        ticks_t start_time;
    };

    void submit(action_t *a) {
        if (a->get_is_read()) pm_io_reads.begin(&a->start_time);
        else pm_io_writes.begin(&a->start_time);
        submit_fun(a);
    }

    boost::function<void (action_t *)> done_fun;

    boost::function<void (payload_t *)> submit_fun;

    void done(payload_t *p) {
        action_t *a = static_cast<action_t *>(p);
        if (a->get_is_read()) pm_io_reads.end(&a->start_time);
        else pm_io_writes.end(&a->start_time);
        done_fun(a);
    }
};

#endif /* __ARCH_LINUX_DISK_STATS_HPP__ */
