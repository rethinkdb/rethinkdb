#include "btree/get_cas.hpp"

#include "btree/modify_oper.hpp"
#include "concurrency/promise.hpp"
#include "btree/btree_data_provider.hpp"

// TODO: Use a shared_ptr to the transactor instead of a death_signalling_data_provider_t.

struct death_signalling_data_provider_t : public data_provider_t {

    death_signalling_data_provider_t(unique_ptr_t<data_provider_t> dp, cond_t *c) :
        dp(dp), pulse_on_death(c) { }
    ~death_signalling_data_provider_t() {
        pulse_on_death->pulse();
    }

    size_t get_size() const {
        return dp->get_size();
    }
    const const_buffer_group_t *get_data_as_buffers() throw (data_provider_failed_exc_t) {
        return dp->get_data_as_buffers();
    }
    void get_data_into_buffers(const buffer_group_t *bg) throw (data_provider_failed_exc_t) {
        dp->get_data_into_buffers(bg);
    }

private:
    unique_ptr_t<data_provider_t> dp;
    cond_t *pulse_on_death;
};

// This function is like get(), except that it sets a CAS value if there isn't
// one already, so it has to be a btree_modify_oper_t. Potentially we can use a
// regular get() for this (that replaces itself with this one if a CAS value
// hasn't been set, for instance), but depending on how CAS is used, that may
// be unnecessary.

struct btree_get_cas_oper_t : public btree_modify_oper_t, public home_thread_mixin_t {
    btree_get_cas_oper_t(cas_t proposed_cas_, promise_t<get_result_t> *res_)
        : proposed_cas(proposed_cas_), res(res_) { }

    bool operate(const boost::shared_ptr<transactor_t>& txor, btree_value *old_value, boost::scoped_ptr<large_buf_t>& old_large_buflock, btree_value **new_value, boost::scoped_ptr<large_buf_t>& new_large_buflock) {
        if (!old_value) {
            /* If not found, there's nothing to do */
            res->pulse(get_result_t());
            return false;
        }

        // Duplicate the value and put a CAS on it if necessary
        valuecpy(&value, old_value);   // Can we fix this extra copy?
        bool there_was_cas_before = value.has_cas();
        cas_t cas_to_report;
        if (value.has_cas()) {
            // How convenient; there already was a CAS.
            cas_to_report = value.cas();
        } else {
            // We have always been at war with Eurasia.
            /* This doesn't set the CAS--it just makes room for the CAS, and
            run_btree_modify_oper() sets the CAS. */
            value.add_cas();
            cas_to_report = proposed_cas;
        }

        // Need to block on the caller so we don't free the large value before it's done
        // Deliver the value to the client via the promise_t we got
        unique_ptr_t<value_data_provider_t> dp(value_data_provider_t::create(&value, txor));
        if (value.is_large()) {
            // When dp2 is destroyed, it will signal to_signal_when_done.
            cond_t to_signal_when_done;
            unique_ptr_t<death_signalling_data_provider_t> dp2(
                new death_signalling_data_provider_t(dp, &to_signal_when_done));
            res->pulse(get_result_t(dp2, value.mcflags(), cas_to_report));
            to_signal_when_done.wait();
        } else {
            res->pulse(get_result_t(dp, value.mcflags(), cas_to_report));
        }

        // Return the new value to the code that will put it into the tree
        if (there_was_cas_before) {
            return false; // We didn't actually fail, but we made no change
        } else {
            *new_value = &value;
            new_large_buflock.swap(old_large_buflock);
            return true;
        }
    }

    int compute_expected_change_count(UNUSED const size_t block_size) {
        return 1;
    }

    cas_t proposed_cas;
    get_result_t result;
    promise_t<get_result_t> *res;

    union {
        char value_memory[MAX_BTREE_VALUE_SIZE];
        btree_value value;
    };
};

void co_btree_get_cas(const store_key_t &key, castime_t castime, btree_slice_t *slice,
                      sequence_group_t *seq_group,
                      promise_t<get_result_t> *res, order_token_t token) {
    btree_get_cas_oper_t oper(castime.proposed_cas, res);
    run_btree_modify_oper(&oper, slice, seq_group, key, castime, token);
}

get_result_t btree_get_cas(const store_key_t &key, btree_slice_t *slice, sequence_group_t *seq_group, castime_t castime, order_token_t token) {
    promise_t<get_result_t> res;
    coro_t::spawn_now(boost::bind(co_btree_get_cas, key, castime, slice, seq_group, &res, token));
    return res.wait();
}
