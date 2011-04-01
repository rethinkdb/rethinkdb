#ifndef __BTREE_SLICE_DISPATCHING_TO_MASTER_HPP__
#define __BTREE_SLICE_DISPATCHING_TO_MASTER_HPP__

#include "btree/slice.hpp"
#include "store.hpp"
#include "replication/master.hpp"

class mutation_dispatcher_t {
public:
    mutation_dispatcher_t() { }
    // Dispatches a change, and returns a modified mutation_t.
    virtual mutation_t dispatch_change(const mutation_t& m, castime_t castime) = 0;
    virtual ~mutation_dispatcher_t() { }
private:
    DISABLE_COPYING(mutation_dispatcher_t);
};

class null_dispatcher_t : public mutation_dispatcher_t {
public:
    null_dispatcher_t() { }
    mutation_t dispatch_change(const mutation_t& m, UNUSED castime_t castime) { return m; }
private:
    DISABLE_COPYING(null_dispatcher_t);
};

class master_dispatcher_t : public mutation_dispatcher_t {
public:
    master_dispatcher_t(int slice_home_thread, replication::master_t *master);
    mutation_t dispatch_change(const mutation_t& m, castime_t castime);
private:
    int slice_home_thread_;
    replication::master_t *master_;
    DISABLE_COPYING(master_dispatcher_t);
};


class backfill_callback_t;

class btree_slice_dispatching_to_master_t : public set_store_t {
public:
    btree_slice_dispatching_to_master_t(btree_slice_t *slice, snag_ptr_t<replication::master_t>& master);
    ~btree_slice_dispatching_to_master_t() {
        debugf("Destroying a btree_slice_dispatching_to_master_t.\n");
    }

    /* set_store_t interface. */

    mutation_result_t change(const mutation_t& m, castime_t castime);
    void spawn_backfill(repli_timestamp since_when, backfill_callback_t *callback);

private:
    btree_slice_t *slice_;
    snag_ptr_t<replication::master_t> master_;
    boost::scoped_ptr<mutation_dispatcher_t> mutation_dispatcher_;
    DISABLE_COPYING(btree_slice_dispatching_to_master_t);
};

#endif  // __BTREE_SLICE_DISPATCHING_TO_MASTERSTORE_HPP__
