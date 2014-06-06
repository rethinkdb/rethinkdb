#ifndef ARCH_IO_DISK_STATS_2_HPP_
#define ARCH_IO_DISK_STATS_2_HPP_

#include <string>

#include "arch/io/disk/pool.hpp"
#include "perfmon/perfmon.hpp"

struct stats_diskmgr_2_action_t : public pool_diskmgr_t::action_t {
    ticks_t start_time;
};

void debug_print(printf_buffer_t *buf,
                 const stats_diskmgr_2_action_t &action);

struct stats_diskmgr_2_t : private passive_producer_t<pool_diskmgr_t::action_t *> {
    typedef stats_diskmgr_2_action_t action_t;

    /* Give the `stats_diskmgr_2_t` constructor a
    `passive_producer_t<action_t *>`; it will get its operations from there. It
    will call `done_fun` on each one when it's done. */
    stats_diskmgr_2_t(
            perfmon_collection_t *stats, const std::string &name,
            passive_producer_t<action_t *> *_source);

    std::function<void (action_t *)> done_fun;

    passive_producer_t<pool_diskmgr_t::action_t *> *const producer;
    void done(pool_diskmgr_t::action_t *p);

private:
    pool_diskmgr_t::action_t *produce_next_value();

    passive_producer_t<action_t *> *source;
    perfmon_duration_sampler_t read_sampler, write_sampler;
    perfmon_multi_membership_t stats_membership;
};


#endif  // ARCH_IO_DISK_STATS_2_HPP_
