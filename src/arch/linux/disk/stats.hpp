#ifndef __ARCH_LINUX_DISK_STATS_HPP__
#define __ARCH_LINUX_DISK_STATS_HPP__

#include "perfmon.hpp"
#include <boost/function.hpp>

template<class payload_t>
struct stats_diskmgr_t {

    stats_diskmgr_t(perfmon_duration_sampler_t *rs, perfmon_duration_sampler_t *ws) :
        read_sampler(rs), write_sampler(ws) { }

    struct action_t : public payload_t {
        ticks_t start_time;
    };

    void submit(action_t *a) {
        if (a->get_is_read()) read_sampler->begin(&a->start_time);
        else write_sampler->begin(&a->start_time);
        submit_fun(a);
    }

    boost::function<void (action_t *)> done_fun;

    boost::function<void (payload_t *)> submit_fun;

    void done(payload_t *p) {
        action_t *a = static_cast<action_t *>(p);
        if (a->get_is_read()) read_sampler->end(&a->start_time);
        else write_sampler->end(&a->start_time);
        done_fun(a);
    }

private:
    perfmon_duration_sampler_t *read_sampler, *write_sampler;
};

#endif /* __ARCH_LINUX_DISK_STATS_HPP__ */
