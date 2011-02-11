#ifndef __BTREE_SLICE_DISPATCHING_TO_MASTER_HPP__
#define __BTREE_SLICE_DISPATCHING_TO_MASTER_HPP__

#include "btree/slice.hpp"
#include "store.hpp"
#include "replication/master.hpp"

class btree_slice_dispatching_to_master_t : public get_store_t, public set_store_t {
public:
    btree_slice_dispatching_to_master_t(btree_slice_t *slice, snag_ptr_t<replication::master_t> master)
        : slice_(slice), master_(master) { }

    ~btree_slice_dispatching_to_master_t() {
        debugf("destroying btree_slice_dispatching_to_master_t\n");
    }

    /* get_store_t interface */

    get_result_t get(store_key_t *key);
    rget_result_ptr_t rget(store_key_t *start, store_key_t *end, bool left_open, bool right_open);

    /* set_store_t interface. */

    get_result_t get_cas(store_key_t *key, castime_t castime);
    set_result_t sarc(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t castime, add_policy_t add_policy, replace_policy_t replace_policy, cas_t old_cas);

    incr_decr_result_t incr_decr(incr_decr_kind_t kind, store_key_t *key, uint64_t amount, castime_t castime);
    append_prepend_result_t append_prepend(append_prepend_kind_t kind ,store_key_t *key, data_provider_t *data, castime_t castime);

    delete_result_t delete_key(store_key_t *key, repli_timestamp timestamp);

private:
    btree_slice_t *slice_;
    snag_ptr_t<replication::master_t> master_;

    DISABLE_COPYING(btree_slice_dispatching_to_master_t);
};

#endif  // __BTREE_SLICE_DISPATCHING_TO_MASTERSTORE_HPP__
