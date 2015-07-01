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

/* `backfill_throttler_t` controls which backfills are allowed to run when. It can block
backfills from starting and also preempt already-running backfills. It's abstract to make
unit testing easier; the concrete implementation used in production is always
`standard_backfill_throttler_t`. */

class backfill_throttler_t : public home_thread_mixin_t {
public:
    class priority_t {
    public:
        /* Backfills are marked as "critical" if the availability of a table depends on
        them. */
        enum class critical_t { NO, YES };
        bool operator<(const priority_t &other) const {
            /* Prrocess critical backfills before non-critical backfills */
            if (other.critical == critical_t::YES && critical == critical_t::NO) {
                return true;
            } else if (other.critical == critical_t::NO && critical == critical_t::YES) {
                return false;
            } else {
                /* If backfills are equally critical, prioritize the one that is likely
                to finish faster */
                return num_changes > other.num_changes;
            }
        }
        critical_t critical;
        uint64_t num_changes;
    };

    /* Every backfill constructs a `lock_t` before starting. If the lock's preempt signal
    is pulsed, the backfill will pause, destroy the `lock_t`, and construct another
    `lock_t` before resuming. */
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
