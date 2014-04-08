#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_BACKFILL_THROTTLER_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_BACKFILL_THROTTLER_HPP_

#include "concurrency/interruptor.hpp"
#include "concurrency/new_semaphore.hpp"
#include "containers/scoped.hpp"
#include "errors.hpp"
#include "threading.hpp"

class backfill_throttler_t : public home_thread_mixin_t {
public:
    backfill_throttler_t();

    class lock_t {
    public:
        lock_t(backfill_throttler_t *p, signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t);
        ~lock_t();
    private:
        friend class backfill_throttler_t;
        backfill_throttler_t *parent;
        scoped_ptr_t<new_semaphore_acq_t> global_acq;
    };

    static const int64_t GLOBAL_LIMIT = 16;
    // TODO! Add a per-peer limit

private:
    friend class lock_t;
    // TODO! Add a separate per-peer limit
    new_semaphore_t global_sem;

    DISABLE_COPYING(backfill_throttler_t);
};

#endif  // CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_BACKFILL_THROTTLER_HPP_
