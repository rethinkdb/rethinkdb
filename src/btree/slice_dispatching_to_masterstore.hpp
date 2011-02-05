#ifndef __BTREE_SLICE_DISPATCHING_TO_MASTERSTORE_HPP__
#define __BTREE_SLICE_DISPATCHING_TO_MASTERSTORE_HPP__

#include "btree/slice.hpp"
#include "store.hpp"

class btree_slice_dispatching_to_masterstore_t : public store_t {
public:
    btree_slice_dispatching_to_masterstore_t(btree_slice_t *slice, replication::masterstore_t *masterstore)
        : slice_(slice), masterstore_(masterstore) { }

    ~btree_slice_dispatching_to_masterstore_t() {
        delete slice_;
    }

    /* store_t interface. */
    get_result_t get(store_key_t *key);
    get_result_t get_cas(store_key_t *key, castime_t castime);
    rget_result_t rget(store_key_t *start, store_key_t *end, bool left_open, bool right_open, uint64_t max_results);
    set_result_t set(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t castime);
    set_result_t add(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t castime);
    set_result_t replace(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t castime);
    set_result_t cas(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, cas_t unique, castime_t castime);
    incr_decr_result_t incr(store_key_t *key, unsigned long long amount, castime_t castime);
    incr_decr_result_t decr(store_key_t *key, unsigned long long amount, castime_t castime);
    append_prepend_result_t append(store_key_t *key, data_provider_t *data, castime_t castime);
    append_prepend_result_t prepend(store_key_t *key, data_provider_t *data, castime_t castime);
    delete_result_t delete_key(store_key_t *key, repli_timestamp timestamp);



private:
    btree_slice_t *slice_;
    replication::masterstore_t *masterstore_;

    uint32_t cas_counter_;
    castime_t gen_castime();
    castime_t generate_if_necessary(castime_t castime);
    repli_timestamp generate_if_necessary(repli_timestamp timestamp);

    DISABLE_COPYING(btree_slice_dispatching_to_masterstore_t);
};



#endif  // __BTREE_SLICE_DISPATCHING_TO_MASTERSTORE_HPP__
