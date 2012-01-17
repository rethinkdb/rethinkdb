#ifndef BUFFER_CACHE_SEQUENCE_GROUP_HPP_
#define BUFFER_CACHE_SEQUENCE_GROUP_HPP_

#include "concurrency/coro_fifo.hpp"

#include "db_thread_info.hpp"

class per_slice_sequence_group_t {
public:
    per_slice_sequence_group_t() { }
    ~per_slice_sequence_group_t() { }

    coro_fifo_t fifo;
private:
    DISABLE_COPYING(per_slice_sequence_group_t);
};

// This function undeclared, defined in key_value_store.cc.  It's
// safer than using the modulo operator directly here.
int compute_shard_home_thread(int shard_number, int num_db_threads);

// Corresponds to a group of operations that must have their order
// preserved.  (E.g. operations coming in over the same memcached
// connection.)  Used by co_begin_transaction to make sure operations
// don't get reordered when throttling write operations.
class sequence_group_t {
public:
    // TODO FIFO: Don't even think about running the database while n_slices has a default argument.
    sequence_group_t(int n_slices = 128) : slice_groups(new per_slice_sequence_group_t[n_slices]) {
        int num_db_threads = get_num_db_threads();
        for (int i = 0; i < n_slices; ++i) {
            slice_groups[i].fifo.rethread(compute_shard_home_thread(i, num_db_threads));
        }
    }
    ~sequence_group_t() { delete[] slice_groups; }

    per_slice_sequence_group_t *slice_groups;
private:
    DISABLE_COPYING(sequence_group_t);
};





#endif  // BUFFER_CACHE_SEQUENCE_GROUP_HPP_
