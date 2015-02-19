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
    backfill_throttler_t();

    class lock_t {
    public:
        lock_t(backfill_throttler_t *p, peer_id_t from_peer,
               signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);
        ~lock_t();
    private:
        void reset();
        backfill_throttler_t *parent;
        // An optional to handle cases where reset() is called twice.
        // That shouldn't happen, unless the constructor throws an exception and
        // the ~lock_t() destructor is still executed somewhow. But let's better be
        // on the safe side here.
        boost::optional<peer_id_t> peer;
        scoped_ptr_t<new_semaphore_acq_t> global_acq;
        scoped_ptr_t<new_semaphore_acq_t> peer_acq;
    };

private:
    friend class lock_t;
    new_semaphore_t global_sem;
    std::map<peer_id_t, scoped_ptr_t<new_semaphore_t> > peer_sems;
    std::map<peer_id_t, intptr_t> peer_sems_refcount;

    DISABLE_COPYING(backfill_throttler_t);
};

#endif  // CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILL_THROTTLER_HPP_
