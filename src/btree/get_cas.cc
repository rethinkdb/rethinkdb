#include "btree/get_cas.hpp"

#include "btree/modify_oper.hpp"
#include "concurrency/cond_var.hpp"
#include "btree/get.hpp"   // For value_data_provider_t

// This function is like get(), except that it sets a CAS value if there isn't
// one already, so it has to be a btree_modify_oper_t. Potentially we can use a
// regular get() for this (that replaces itself with this one if a CAS value
// hasn't been set, for instance), but depending on how CAS is used, that may
// be unnecessary.

struct btree_get_cas_oper_t : public btree_modify_oper_t, public home_thread_mixin_t {

    btree_get_cas_oper_t(promise_t<store_t::get_result_t, threadsafe_cond_t> *res)
        : res(res) { }

    bool operate(transaction_t *txn, btree_value *old_value, large_buf_t *old_large_buf, btree_value **new_value, large_buf_t **new_large_buf) {

        if (!old_value) {
            /* If not found, there's nothing to do */
            res->pulse(store_t::get_result_t());
            return false;
        }

        // Duplicate the value and put a CAS on it if necessary

        valuecpy(&value, old_value);   // Can we fix this extra copy?
        bool there_was_cas_before = value.has_cas();
        if (!value.has_cas()) { // We have always been at war with Eurasia.
            value.set_cas(slice->gen_cas());
            this->cas_already_set = true;
        }

        // Deliver the value to the client via the promise_t we got

        boost::shared_ptr<value_data_provider_t> dp(new value_data_provider_t);
        valuecpy(&dp->small_part, &value);
        if (value.is_large()) {
            dp->large_part = old_large_buf;
            // Need to block on the caller so we don't free the large value before it's done
            threadsafe_cond_t to_signal_when_done;
            dp->to_signal_when_done = &to_signal_when_done;
            res->pulse(store_t::get_result_t(dp, value.mcflags(), value.cas()));
            dp.reset();   // So the destructor can get called
            to_signal_when_done.wait();
        } else {
            res->pulse(store_t::get_result_t(dp, value.mcflags(), value.cas()));
        }

        // Return the new value to the code that will put it into the tree

        if (there_was_cas_before) {
            return false; // We didn't actually fail, but we made no change
        } else {
            *new_value = &value;
            *new_large_buf = old_large_buf;
            return true;
        }
    }

    store_t::get_result_t result;
    promise_t<store_t::get_result_t, threadsafe_cond_t> *res;

    union {
        byte value_memory[MAX_BTREE_VALUE_SIZE];
        btree_value value;
    };
};

void co_btree_get_cas(const btree_key *key, btree_slice_t *slice,
        promise_t<store_t::get_result_t, threadsafe_cond_t> *res) {
    btree_get_cas_oper_t oper(res);
    run_btree_modify_oper(&oper, slice, key);
}

store_t::get_result_t btree_get_cas(const btree_key *key, btree_slice_t *slice) {
    block_pm_duration get_timer(&pm_cmd_get);
    promise_t<store_t::get_result_t, threadsafe_cond_t> res;
    coro_t::spawn(co_btree_get_cas, key, slice, &res);
    return res.wait();
}
