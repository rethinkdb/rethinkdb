#ifndef __BTREE_SLICE_DISPATCHING_TO_MASTER_HPP__
#define __BTREE_SLICE_DISPATCHING_TO_MASTER_HPP__

#include "btree/slice.hpp"
#include "store.hpp"
#include "replication/master.hpp"


class backfill_callback_t;

class btree_slice_dispatching_to_master_t : public set_store_t {
public:
    btree_slice_dispatching_to_master_t(btree_slice_t *slice, replication::master_t *master)
        : slice_(slice), master_(master) { }

    /* set_store_t interface. */

    mutation_result_t change(const mutation_t &m, castime_t castime);
    void backfill(repli_timestamp since_when, backfill_callback_t *callback) {
        on_thread_t th(slice_->home_thread);
        slice_->backfill(since_when, callback);
    }

private:
    btree_slice_t *slice_;
    replication::master_t *master_;

    DISABLE_COPYING(btree_slice_dispatching_to_master_t);
};

#endif  // __BTREE_SLICE_DISPATCHING_TO_MASTERSTORE_HPP__
