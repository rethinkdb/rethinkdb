#ifndef __BTREE_SLICE_DISPATCHING_TO_MASTER_HPP__
#define __BTREE_SLICE_DISPATCHING_TO_MASTER_HPP__

#include "btree/slice.hpp"
#include "store.hpp"
#include "replication/master.hpp"


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
    void nop_back_on_masters_thread(repli_timestamp timestamp, cond_t *cond, int *counter);

private:
    btree_slice_t *slice_;
    snag_ptr_t<replication::master_t> master_;

    DISABLE_COPYING(btree_slice_dispatching_to_master_t);
};

#endif  // __BTREE_SLICE_DISPATCHING_TO_MASTERSTORE_HPP__
