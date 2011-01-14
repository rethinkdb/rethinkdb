#ifndef __BTREE_GET_CAS_FSM_HPP__
#define __BTREE_GET_CAS_FSM_HPP__

#include "btree/modify_oper.hpp"

// This function is like get(), except that it sets a CAS value if there isn't
// one already, so it has to be a btree_modify_oper_t. Potentially we can use a
// regular get() for this (that replaces itself with this one if a CAS value
// hasn't been set, for instance), but depending on how CAS is used, that may
// be unnecessary.

class btree_get_cas_oper_t : public btree_modify_oper_t, public home_thread_mixin_t {
public:
    explicit btree_get_cas_oper_t(store_t::get_callback_t *cb)
        : btree_modify_oper_t(), callback(cb), in_operate(false)
    {
        pm_cmd_get.begin(&start_time);
    }
    
    ~btree_get_cas_oper_t() {
        pm_cmd_get.end(&start_time);
    }

    bool operate(transaction_t *txn, btree_value *old_value, large_buf_t *old_large_buf, btree_value **new_value, large_buf_t **new_large_buf) {
        if (!old_value) {
            result = result_not_found;
            return false;
        }

        valuecpy(&value, old_value);
        large_value = old_large_buf;

        there_was_cas_before = value.has_cas();
        if (!value.has_cas()) { // We have always been at war with Eurasia.
            value.set_cas(slice->gen_cas());
            this->cas_already_set = true;
        }

        if (value.is_large()) {
            result = result_large_value;
            // Prepare the buffer group
            for (int64_t i = 0; i < old_large_buf->get_num_segments(); i++) {
                uint16_t size;
                const void *data = old_large_buf->get_segment(i, &size);
                buffer_group.add_buffer(size, data);
            }

            { // TODO: Actually use RAII.
                coro_t::move_to_thread(home_thread);
                co_value(callback, &buffer_group, value.mcflags(), value.cas());
                coro_t::move_to_thread(slice->home_thread);
            }
        } else {
            result = result_small_value;
        }

        if (there_was_cas_before) {
            return false; // We didn't actually fail, but we made no change
        } else {
            *new_value = &value;
            *new_large_buf = large_value;
            return true;
        }
    }
    
    void call_callback() {
        switch (result) {
            case result_not_found:
                callback->not_found();
                break;
            case result_small_value:
                buffer_group.add_buffer(value.value_size(), value.value());
                co_value(callback, &buffer_group, value.mcflags(), value.cas());
                break;
            case result_large_value:
                // We already delivered our callback during operate().
                break;
            default:
                unreachable();
        }
    }
    
private:
    store_t::get_callback_t *callback;
    union {
        byte value_memory[MAX_TOTAL_NODE_CONTENTS_SIZE+sizeof(btree_value)];
        btree_value value;
    };
    large_buf_t *large_value;
    const_buffer_group_t buffer_group;
    bool in_operate;
    bool have_delivered_value;
    
    ticks_t start_time;
    bool there_was_cas_before;
    enum result_t {
        result_not_found,
        result_small_value,
        result_large_value,
    } result;
};

void co_btree_get_cas(btree_key *key, btree_key_value_store_t *store, store_t::get_callback_t *cb) {
    btree_get_cas_oper_t *oper = new btree_get_cas_oper_t(cb);
    run_btree_modify_oper(oper, store, key);
}

void btree_get_cas(btree_key *key, btree_key_value_store_t *store, store_t::get_callback_t *cb) {
    coro_t::spawn(co_btree_get_cas, key, store, cb);
}

#endif // __BTREE_GET_CAS_FSM_HPP__
