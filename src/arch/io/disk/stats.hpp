// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef ARCH_IO_DISK_STATS_HPP_
#define ARCH_IO_DISK_STATS_HPP_

#include <string>

#include "utils.hpp"
#include <boost/function.hpp>

#include "arch/io/disk/pool.hpp"
#include "perfmon/types.hpp"

/* There are two types of stat-collectors in the disk stack. One type is a passive
consumer and active producer of disk operations. The other type is an active consumer
and passive producer. */

template<class payload_t>
struct stats_diskmgr_t {
    stats_diskmgr_t(perfmon_collection_t *stats, const std::string &name) :
        read_sampler(secs_to_ticks(1)),
        write_sampler(secs_to_ticks(1)),
        stats_membership(stats,
            &read_sampler, (name + "_read").c_str(),
            &write_sampler, (name + "_write").c_str(),
                         NULLPTR)
    { }

    struct action_t : public payload_t {
        ticks_t start_time;
    };

    void submit(action_t *a) {
        if (a->get_is_read()) {
            read_sampler.begin(&a->start_time);
        } else {
            write_sampler.begin(&a->start_time);
        }
        submit_fun(a);
    }

    boost::function<void (action_t *)> done_fun;

    boost::function<void (payload_t *)> submit_fun;

    void done(payload_t *p) {
        action_t *a = static_cast<action_t *>(p);
        if (a->get_is_read()) {
            read_sampler.end(&a->start_time);
        } else {
            write_sampler.end(&a->start_time);
        }
        done_fun(a);
    }

private:
    perfmon_duration_sampler_t read_sampler, write_sampler;
    perfmon_multi_membership_t stats_membership;
};

template <class payload_t>
struct stats_diskmgr_2_action_t : public payload_t {
    ticks_t start_time;
};

template <class payload_t>
void debug_print(printf_buffer_t *buf,
                 const stats_diskmgr_2_action_t<payload_t> &action) {
    buf->appendf("stats_diskmgr_2_action{start_time=%" PRIu64 "}<",
                 action.start_time);
    const payload_t &parent_action = action;
    debug_print(buf, parent_action);
}

struct stats_diskmgr_2_t : private passive_producer_t<pool_diskmgr_t::action_t *> {
    typedef stats_diskmgr_2_action_t<pool_diskmgr_t::action_t> action_t;

    /* Give the `stats_diskmgr_2_t` constructor a
    `passive_producer_t<action_t *>`; it will get its operations from there. It
    will call `done_fun` on each one when it's done. */
    stats_diskmgr_2_t(
            perfmon_collection_t *stats, const std::string &name,
            passive_producer_t<action_t *> *_source) :
        passive_producer_t<pool_diskmgr_t::action_t *>(_source->available),
        producer(this),
        source(_source),
        read_sampler(secs_to_ticks(1)),
        write_sampler(secs_to_ticks(1)),
        stats_membership(stats,
            &read_sampler, (name + "_read").c_str(),
            &write_sampler, (name + "_write").c_str(),
            NULLPTR)
        { }
    boost::function<void (action_t *)> done_fun;

    passive_producer_t<pool_diskmgr_t::action_t *> *const producer;
    void done(pool_diskmgr_t::action_t *p) {
        action_t *a = static_cast<action_t *>(p);
        if (a->get_is_read()) {
            read_sampler.end(&a->start_time);
        } else {
            write_sampler.end(&a->start_time);
        }
        done_fun(a);
    }

private:
    pool_diskmgr_t::action_t *produce_next_value() {
        action_t *a = source->pop();
        if (a->get_is_read()) {
            read_sampler.begin(&a->start_time);
        } else {
            write_sampler.begin(&a->start_time);
        }
        return a;
    }
    passive_producer_t<action_t *> *source;
    perfmon_duration_sampler_t read_sampler, write_sampler;
    perfmon_multi_membership_t stats_membership;
};

#endif /* ARCH_IO_DISK_STATS_HPP_ */
