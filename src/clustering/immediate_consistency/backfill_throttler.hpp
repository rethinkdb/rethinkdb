// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILL_THROTTLER_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILL_THROTTLER_HPP_

#include <map>

#include "errors.hpp"
#include <boost/optional.hpp>

#include "concurrency/interruptor.hpp"
#include "concurrency/new_semaphore.hpp"
#include "containers/scoped.hpp"
#include "rpc/connectivity/peer_id.hpp"
#include "threading.hpp"

class backfill_throttler_t : public home_thread_mixin_t {
public:
    class priority_t {
    public:
        enum class critical_t { NO, YES };
        bool operator<(const priority_t &other) const {
            /* Prrocess critical backfills before non-critical backfills; process
            backfills with few changes before backfills with many changes */
            return std::make_tuple(critical, -num_changes) <
                std::make_tuple(other.critical, -other.num_changes);
        }
        critical_t critical;
        int num_changes;
    };

    class lock_t : public home_thread_mixin_t {
    public:
        lock_t(backfill_throttler_t *p,
               const priority_t &_priority,
               signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) :
            priority(_priority), parent(p)
        {
            parent->enter(this, interruptor);
        }
        ~lock_t() {
            parent->exit(this);
        }
        signal_t *get_preempt_signal() {
            return &preempt_signal;
        }
        const priority_t priority;
    private:
        friend class backfill_throttler_t;
        backfill_throttler_t *parent;
        cond_t preempt_signal;
    };

protected:
    friend class lock_t;

    virtual ~backfill_throttler_t() { }

    virtual void enter(lock_t *lock, signal_t *interruptor) = 0;
    virtual void exit(lock_t *lock) = 0;

    /* This allows subclasses to signal locks' preempt signals even though
    `preempt_signal` is a private member of `lock_t` */
    void preempt(lock_t *lock) {
        lock->preempt_signal.pulse();
    }
};

#endif  // CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILL_THROTTLER_HPP_
