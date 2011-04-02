#ifndef __BTREE_SLICE_DISPATCHING_TO_MASTER_HPP__
#define __BTREE_SLICE_DISPATCHING_TO_MASTER_HPP__

#include "btree/slice.hpp"
#include "store.hpp"
#include "replication/master.hpp"

// TODO: This belongs in the btree directory.
class mutation_dispatcher_t {
public:
    mutation_dispatcher_t() { }
    // Dispatches a change, and returns a modified mutation_t.
    virtual mutation_t dispatch_change(const mutation_t& m, castime_t castime) = 0;
    virtual ~mutation_dispatcher_t() { }
private:
    DISABLE_COPYING(mutation_dispatcher_t);
};

// TODO: I suppose this belongs in the btree directory.
class null_dispatcher_t : public mutation_dispatcher_t {
public:
    null_dispatcher_t() { }
    mutation_t dispatch_change(const mutation_t& m, UNUSED castime_t castime) { return m; }
private:
    DISABLE_COPYING(null_dispatcher_t);
};

// TODO: I suppose this belongs in the replication directory, or the server directory.
class master_dispatcher_t : public mutation_dispatcher_t {
public:
    master_dispatcher_t(replication::master_t *master);
    mutation_t dispatch_change(const mutation_t& m, castime_t castime);
private:
    replication::master_t *master_;
    DISABLE_COPYING(master_dispatcher_t);
};


#endif  // __BTREE_SLICE_DISPATCHING_TO_MASTERSTORE_HPP__
