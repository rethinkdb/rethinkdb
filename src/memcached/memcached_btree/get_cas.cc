// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "memcached/memcached_btree/get_cas.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/runtime/coroutines.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "concurrency/promise.hpp"
#include "memcached/memcached_btree/btree_data_provider.hpp"
#include "memcached/memcached_btree/modify_oper.hpp"

// This function is like get(), except that it sets a CAS value if
// there isn't one already, so it has to be a
// memcached_modify_oper_t. Potentially we can use a regular get() for
// this (that replaces itself with this one if a CAS value hasn't been
// set, for instance), but depending on how CAS is used, that may be
// unnecessary.

struct memcached_get_cas_oper_t : public memcached_modify_oper_t, public home_thread_mixin_debug_only_t {
    memcached_get_cas_oper_t(cas_t _proposed_cas, promise_t<get_result_t> *_res)
        : proposed_cas(_proposed_cas), res(_res) { }

    bool operate(transaction_t *txn, scoped_malloc_t<memcached_value_t> *value) {
        if (!value->has()) {
            // If not found, there's nothing to do.
            res->pulse(get_result_t());
            return false;
        }

        bool there_was_cas_before = (*value)->has_cas();
        cas_t cas_to_report;
        if (there_was_cas_before) {
            // How convenient, there already was a CAS.
            cas_to_report = (*value)->cas();
        } else {
            // This doesn't set the CAS -- it just makes room for the
            // CAS, and run_memcached_modify_oper() sets the CAS.
            (*value)->add_cas(txn->get_cache()->get_block_size());
            cas_to_report = proposed_cas;
        }

        // Deliver the value to the client via the promise_t we got.
        counted_t<data_buffer_t> dp = value_to_data_buffer(value->get(), txn);
        res->pulse(get_result_t(dp, (*value)->mcflags(), cas_to_report));

        // Return whether we made a change to the value.
        return !there_was_cas_before;
    }

    int compute_expected_change_count(UNUSED block_size_t block_size) {
        return 1;
    }

    cas_t proposed_cas;
    get_result_t result;
    promise_t<get_result_t> *res;
};

void co_memcached_get_cas(const store_key_t &key, cas_t proposed_cas, exptime_t effective_time, repli_timestamp_t timestamp, btree_slice_t *slice,
                      promise_t<get_result_t> *res, transaction_t *txn, superblock_t *superblock) {

    memcached_get_cas_oper_t oper(proposed_cas, res);
    run_memcached_modify_oper(&oper, slice, key, proposed_cas, effective_time, timestamp, txn, superblock);
}

get_result_t memcached_get_cas(const store_key_t &key, btree_slice_t *slice, cas_t proposed_cas, exptime_t effective_time, repli_timestamp_t timestamp, transaction_t *txn, superblock_t *superblock) {
    promise_t<get_result_t> res;
    coro_t::spawn_now_dangerously(boost::bind(co_memcached_get_cas, boost::ref(key), proposed_cas, effective_time, timestamp, slice, &res, txn, boost::ref(superblock)));
    return res.wait();
}

