#include "btree/get_cas.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/runtime/coroutines.hpp"
#include "btree/modify_oper.hpp"
#include "concurrency/promise.hpp"
#include "btree/btree_data_provider.hpp"

// TODO: Use a shared_ptr to the transaction instead of a death_signalling_data_provider_t.

struct death_signalling_data_provider_t : public data_provider_t {

    death_signalling_data_provider_t(boost::shared_ptr<data_provider_t> dp, cond_t *c) :
        dp(dp), pulse_on_death(c) { }
    ~death_signalling_data_provider_t() {
        pulse_on_death->pulse();
    }

    size_t get_size() const {
        return dp->get_size();
    }
    const const_buffer_group_t *get_data_as_buffers() {
        return dp->get_data_as_buffers();
    }
    void get_data_into_buffers(const buffer_group_t *bg) {
        dp->get_data_into_buffers(bg);
    }

private:
    boost::shared_ptr<data_provider_t> dp;
    cond_t *pulse_on_death;
};

// This function is like get(), except that it sets a CAS value if there isn't
// one already, so it has to be a btree_modify_oper_t. Potentially we can use a
// regular get() for this (that replaces itself with this one if a CAS value
// hasn't been set, for instance), but depending on how CAS is used, that may
// be unnecessary.

struct btree_get_cas_oper_t : public btree_modify_oper_t<memcached_value_t>, public home_thread_mixin_t {
    btree_get_cas_oper_t(cas_t proposed_cas_, promise_t<get_result_t> *res_)
        : proposed_cas(proposed_cas_), res(res_) { }

    bool operate(transaction_t *txn, scoped_malloc<memcached_value_t>& value) {
        if (!value) {
            // If not found, there's nothing to do.
            res->pulse(get_result_t());
            return false;
        }

        bool there_was_cas_before = value->has_cas();
        cas_t cas_to_report;
        if (there_was_cas_before) {
            // How convenient, there already was a CAS.
            cas_to_report = value->cas();
        } else {
            // This doesn't set the CAS -- it just makes room for the
            // CAS, and run_btree_modify_oper() sets the CAS.
            value->add_cas(txn->get_cache()->get_block_size());
            cas_to_report = proposed_cas;
        }

        // Deliver the value to the client via the promise_t we got.
        boost::shared_ptr<value_data_provider_t> dp(value_data_provider_t::create(value.get(), txn));
        res->pulse(get_result_t(dp, value->mcflags(), cas_to_report));

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

void co_btree_get_cas(const store_key_t &key, castime_t castime, btree_slice_t *slice,
                      promise_t<get_result_t> *res, order_token_t token) {
    btree_get_cas_oper_t oper(castime.proposed_cas, res);
    memcached_value_sizer_t sizer(slice->cache()->get_block_size());
    run_btree_modify_oper<memcached_value_t>(&sizer, &oper, slice, key, castime, token);
}

get_result_t btree_get_cas(const store_key_t &key, btree_slice_t *slice, castime_t castime, order_token_t token) {
    promise_t<get_result_t> res;
    coro_t::spawn_now(boost::bind(co_btree_get_cas, key, castime, slice, &res, token));
    return res.wait();
}
