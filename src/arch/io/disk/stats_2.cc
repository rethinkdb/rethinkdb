#include "arch/io/disk/stats_2.hpp"

#include <inttypes.h>

#include "containers/printf_buffer.hpp"

void debug_print(printf_buffer_t *buf,
                 const stats_diskmgr_2_action_t &action) {
    buf->appendf("stats_diskmgr_2_action{start_time=%" PRIu64 "}<",
                 action.start_time);
    const pool_diskmgr_t::action_t &parent_action = action;
    debug_print(buf, parent_action);
}

/* Give the `stats_diskmgr_2_t` constructor a
   `passive_producer_t<action_t *>`; it will get its operations from there. It
   will call `done_fun` on each one when it's done. */
stats_diskmgr_2_t::stats_diskmgr_2_t(
        perfmon_collection_t *stats, const std::string &name,
        passive_producer_t<action_t *> *_source) :
    passive_producer_t<pool_diskmgr_t::action_t *>(_source->available),
    producer(this),
    source(_source),
    read_sampler(secs_to_ticks(1)),
    write_sampler(secs_to_ticks(1)),
    stats_membership(stats,
                     &read_sampler, (name + "_read").c_str(),
                     &write_sampler, (name + "_write").c_str()) { }


void stats_diskmgr_2_t::done(pool_diskmgr_t::action_t *p) {
    action_t *a = static_cast<action_t *>(p);
    if (a->get_is_read()) {
        read_sampler.end(&a->start_time);
    } else {
        write_sampler.end(&a->start_time);
    }
    done_fun(a);
}

pool_diskmgr_t::action_t *stats_diskmgr_2_t::produce_next_value() {
    action_t *a = source->pop();
    if (a->get_is_read()) {
        read_sampler.begin(&a->start_time);
    } else {
        write_sampler.begin(&a->start_time);
    }
    return a;
}
