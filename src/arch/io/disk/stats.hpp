// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_IO_DISK_STATS_HPP_
#define ARCH_IO_DISK_STATS_HPP_

#include <string>

#include "utils.hpp"
#include <boost/function.hpp>

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

template<class payload_t>
struct stats_diskmgr_2_t :
    /* private; access it through the `producer` field instead */
    private passive_producer_t<payload_t *>
{
    struct action_t : public payload_t {
        ticks_t start_time;
    };

    /* Give the `stats_diskmgr_2_t` constructor a
    `passive_producer_t<action_t *>`; it will get its operations from there. It
    will call `done_fun` on each one when it's done. */
    stats_diskmgr_2_t(
            perfmon_collection_t *stats, const std::string &name,
            passive_producer_t<action_t *> *_source) :
        passive_producer_t<payload_t *>(_source->available),
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

    passive_producer_t<payload_t *> * const producer;
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
    payload_t *produce_next_value() {
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
