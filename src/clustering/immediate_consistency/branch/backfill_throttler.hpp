#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_BACKFILL_THROTTLER_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_BACKFILL_THROTTLER_HPP_

#include "errors.hpp"

#include <boost/ptr_container/ptr_map.hpp>

#include "concurrency/interruptor.hpp"
#include "concurrency/new_semaphore.hpp"
#include "containers/scoped.hpp"
#include "rpc/connectivity/connectivity.hpp"
#include "threading.hpp"

class backfill_throttler_t : public home_thread_mixin_t {
public:
    backfill_throttler_t();

    class lock_t {
    public:
        lock_t(backfill_throttler_t *p, peer_id_t from_peer,
               signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);
        ~lock_t();
    private:
        void reset();
        backfill_throttler_t *parent;
        scoped_ptr_t<new_semaphore_acq_t> global_acq;
        scoped_ptr_t<new_semaphore_acq_t> peer_acq;
    };

    static const int64_t GLOBAL_LIMIT = 16;
    static const int64_t PER_PEER_LIMIT = 4;

private:
    friend class lock_t;
    new_semaphore_t global_sem;
    // TODO (daniel): Evict peer_sems that have become unused.
    boost::ptr_map<peer_id_t, new_semaphore_t> peer_sems;

    DISABLE_COPYING(backfill_throttler_t);
};

#endif  // CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_BACKFILL_THROTTLER_HPP_
