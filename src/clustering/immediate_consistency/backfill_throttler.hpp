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
        bool operator<(const priority_t &other) const {
            /* Prrocess critical backfills before non-critical backfills; process
            backfills with few changes before backfills with many changes */
            return std::tuple(critical, -num_changes) <
                std::tuple(other.critical, -other.num_changes);
        }
        bool critical;
        int num_changes;
    };

    class lock_t {
    public:
        lock_t(backfill_throttler_t *p,
               const peer_id_t &_peer,
               const priority_t &_priority,
               signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) :
            peer(_peer), priority(_priority), parent(p)
        {
            parent->enter(this, interruptor);
        }
        ~lock_t() {
            parent->exit(this);
        }
        signal_t *get_pause_signal() {
            return &pause_signal;
        }
        const peer_id_t peer;
        const priority_t priority;
    private:
        friend class backfill_throttler_t;
        backfill_throttler_t *parent;
        cond_t pause_signal;
    };

protected:
    friend class lock_t;

    ~backfill_throttler_t() { }

    virtual void enter(lock_t *lock, signal_t *interruptor) = 0;
    virtual void exit(lock_t *lock) = 0;

    /* This allows subclasses to signal locks' pause signals even though `pause_signal`
    is a private member of `lock_t` */
    void pause(lock_t *lock) {
        lock->pause_signal.pulse();
    }
};

#endif  // CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILL_THROTTLER_HPP_
