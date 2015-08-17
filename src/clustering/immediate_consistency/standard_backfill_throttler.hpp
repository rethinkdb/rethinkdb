// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_STANDARD_BACKFILL_THROTTLER_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_STANDARD_BACKFILL_THROTTLER_HPP_

#include <set>

#include "clustering/immediate_consistency/backfill_throttler.hpp"
#include "concurrency/new_mutex.hpp"

/* `standard_backfill_throttler_t` is the `backfill_throttler_t` that is used in
production. It allows a fixed number of backfills total (currently 8); if there are more
than 8 backfills trying to run, it will always allow the highest-priority backfills to go
first, preempting the lower-priority backfills if necessary. */

class standard_backfill_throttler_t : public backfill_throttler_t {
public:
    standard_backfill_throttler_t() { }
    ~standard_backfill_throttler_t();

private:
    void enter(lock_t *lock, signal_t *interruptor);
    void exit(lock_t *lock);

    std::multimap<priority_t, std::pair<lock_t *, cond_t *> > waiting;
    std::set<std::pair<priority_t, lock_t *> > active;

    new_mutex_t mutex;
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_STANDARD_BACKFILL_THROTTLER_HPP_ */

