// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef ARCH_IO_DISK_STATS_HPP_
#define ARCH_IO_DISK_STATS_HPP_

#include <functional>
#include <string>

#include "arch/io/disk/pool.hpp"
#include "arch/io/disk/conflict_resolving.hpp"
#include "perfmon/types.hpp"

/* There are two types of stat-collectors in the disk stack. One type is a passive
consumer and active producer of disk operations. The other type is an active consumer
and passive producer. */

struct stats_diskmgr_t {
    stats_diskmgr_t(perfmon_collection_t *stats, const std::string &name);

    struct action_t : public conflict_resolving_diskmgr_action_t {
        ticks_t start_time;
    };

    void submit(action_t *a);

    std::function<void (action_t *)> done_fun;

    std::function<void (conflict_resolving_diskmgr_action_t *)> submit_fun;

    void done(conflict_resolving_diskmgr_action_t *p);

private:
    perfmon_duration_sampler_t read_sampler, write_sampler;
    perfmon_multi_membership_t stats_membership;
};

#endif /* ARCH_IO_DISK_STATS_HPP_ */
