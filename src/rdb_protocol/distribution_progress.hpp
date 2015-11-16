// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_DISTRIBUTION_PROGRESS_HPP_
#define RDB_PROTOCOL_DISTRIBUTION_PROGRESS_HPP_

#include <map>

#include "btree/keys.hpp"
#include "rpc/serialize_macros.hpp"

class store_view_t;
class signal_t;

/* Can be used for estimating the progress of an ordered table traversal. The way this
works is that we perform a distribution query on construction. Then you can call
`estimate_progress` with the current traversal key boundary, and we estimate how many
keys are left to be traversed and divide it by the total number of keys.

You can also serialize this, so you can send the distribution over to a different server
and estimate progress there. */
class distribution_progress_estimator_t {
public:
    distribution_progress_estimator_t(store_view_t *store, signal_t *interruptor);

    // Default constructor only for serialization
    distribution_progress_estimator_t() : distribution_counts_sum(0) { }

    // Returns a value between 0.0 and 1.0
    double estimate_progress(const store_key_t &bound) const;

    RDB_DECLARE_ME_SERIALIZABLE(distribution_progress_estimator_t);

private:
    std::map<store_key_t, int64_t> distribution_counts;
    int64_t distribution_counts_sum;
};


#endif // RDB_PROTOCOL_DISTRIBUTION_PROGRESS_HPP_
