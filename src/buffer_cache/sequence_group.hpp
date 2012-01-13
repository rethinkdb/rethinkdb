#ifndef BUFFER_CACHE_SEQUENCE_GROUP_HPP_
#define BUFFER_CACHE_SEQUENCE_GROUP_HPP_

#include "concurrency/coro_fifo.hpp"

// Corresponds to a group of operations that must have their order
// preserved.  (E.g. operations coming in over the same memcached
// connection.)  Used by co_begin_transaction to make sure operations
// don't get reordered when throttling write operations.
class sequence_group_t {
public:
    sequence_group_t() { }
    ~sequence_group_t() { }

    coro_fifo_t fifo;
private:
    DISABLE_COPYING(sequence_group_t);
};



#endif  // BUFFER_CACHE_SEQUENCE_GROUP_HPP_
