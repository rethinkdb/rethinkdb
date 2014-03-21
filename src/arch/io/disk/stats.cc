#include "arch/io/disk/stats.hpp"

stats_diskmgr_t::stats_diskmgr_t(perfmon_collection_t *stats, const std::string &name) :
    read_sampler(secs_to_ticks(1)),
    write_sampler(secs_to_ticks(1)),
    stats_membership(stats,
                     &read_sampler, (name + "_read").c_str(),
                     &write_sampler, (name + "_write").c_str()) { }


void stats_diskmgr_t::submit(action_t *a) {
    if (a->get_is_read()) {
        read_sampler.begin(&a->start_time);
    } else {
        write_sampler.begin(&a->start_time);
    }
    submit_fun(a);
}

void stats_diskmgr_t::done(conflict_resolving_diskmgr_action_t *p) {
    action_t *a = static_cast<action_t *>(p);
    if (a->get_is_read()) {
        read_sampler.end(&a->start_time);
    } else {
        write_sampler.end(&a->start_time);
    }
    done_fun(a);
}
