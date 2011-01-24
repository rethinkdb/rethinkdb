#include "btree/get_cas.hpp"

#include "btree/modify_oper.hpp"
#include "concurrency/cond_var.hpp"
#include "btree/coro_wrappers.hpp"


// This function is like get(), except that it sets a CAS value if there isn't
// one already, so it has to be a btree_modify_oper_t. Potentially we can use a
// regular get() for this (that replaces itself with this one if a CAS value
// hasn't been set, for instance), but depending on how CAS is used, that may
// be unnecessary.

struct btree_get_cas_oper_t : public btree_modify_oper_t, public home_thread_mixin_t {

    btree_get_cas_oper_t(promise_t<store_t::get_result_t> *res)
        : res(res) { }

    bool operate(transaction_t *txn, btree_value *old_value, large_buf_t *old_large_buf, btree_value **new_value, large_buf_t **new_large_buf) {
        if (!old_value) {
            result = result_not_found;
            return false;
        }

        valuecpy(&value, old_value);

        bool there_was_cas_before = value.has_cas();
        if (!value.has_cas()) { // We have always been at war with Eurasia.
            value.set_cas(slice->gen_cas());
            this->cas_already_set = true;
        }

        if (value.is_large()) {
            result = result_large_value;
            // Prepare the buffer group
            const_buffer_group_t buffer_group;
            for (int64_t i = 0; i < old_large_buf->get_num_segments(); i++) {
                uint16_t size;
                const void *data = old_large_buf->get_segment(i, &size);
                buffer_group.add_buffer(size, data);
            }
            {
                on_thread_t thread_switcher(home_thread);
                co_deliver_get_result(&buffer_group, value.mcflags(), value.cas(), res);
            }
        } else {
            result = result_small_value;
        }

        if (there_was_cas_before) {
            return false; // We didn't actually fail, but we made no change
        } else {
            *new_value = &value;
            *new_large_buf = old_large_buf;
            return true;
        }
    }

    promise_t<store_t::get_result_t> *res;
    union {
        byte value_memory[MAX_BTREE_VALUE_SIZE];
        btree_value value;
    };
    
    enum result_t {
        result_not_found,
        result_small_value,
        result_large_value,
    } result;
};

void co_btree_get_cas(const btree_key *key, btree_slice_t *slice, promise_t<store_t::get_result_t> *res) {
    btree_get_cas_oper_t oper(res);
    run_btree_modify_oper(&oper, slice, key);
    switch (oper.result) {
        case btree_get_cas_oper_t::result_not_found: {
            /* The value was not found. There's no need to wait for a reply from the thing that got
            the value */
            co_deliver_get_result(NULL, 0, 0, res);
            break;
        }
        case btree_get_cas_oper_t::result_small_value: {
            /* It's a small value. Construct a buffer group, deliver the result, and wait for the
            reply so we can finish and destroy ourself. */
            const_buffer_group_t bg;
            bg.add_buffer(oper.value.value_size(), oper.value.value());
            co_deliver_get_result(&bg, oper.value.mcflags(), oper.value.cas(), res);
            break;
        }
        case btree_get_cas_oper_t::result_large_value: {
            /* We already delivered the value--nothing to do! */
             break;
        }
        default: unreachable();
    }
}

store_t::get_result_t btree_get_cas(const btree_key *key, btree_slice_t *slice) {
    block_pm_duration get_timer(&pm_cmd_get);
    promise_t<store_t::get_result_t> res;
    coro_t::spawn(co_btree_get_cas, key, slice, &res);
    return res.wait();
}
