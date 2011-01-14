#ifndef __BTREE_INCR_DECR_FSM_HPP__
#define __BTREE_INCR_DECR_FSM_HPP__

#include "btree/modify_fsm.hpp"

class btree_incr_decr_fsm_t : public btree_modify_fsm_t
{
public:
    explicit btree_incr_decr_fsm_t(bool increment, unsigned long long delta, store_t::incr_decr_callback_t *cb)
        : btree_modify_fsm_t(),
          increment(increment),
          delta(delta),
          callback(cb)
    { }

    bool operate(transaction_t *txn, btree_value *old_value, large_buf_t *old_large_buf, btree_value **new_value, large_buf_t **new_large_buf) {
        // If the key didn't exist before, we fail
        if (!old_value) {
            result = result_not_found;
            return false;
        }
        
        // If we can't parse the key as a number, we fail
        bool valid;
        if (old_value->size < 50) {
            char buffer[50];
            memcpy(buffer, old_value->value(), old_value->size);
            buffer[old_value->size] = 0;
            char *endptr;
            new_number = strtoull_strict(buffer, &endptr, 10);
            if (endptr == buffer) valid = false;
            else valid = true;
        } else {
            valid = false;
        }
        if (!valid) {
            result = result_not_numeric;
            return false;
        }

        
        // If we overflow when doing an increment, set number to 0 (this is as memcached does it as of version 1.4.5)
        // for decrements, set to 0 on underflows
        if (increment) {
            if (new_number + delta < new_number) new_number = 0;
            else new_number += delta;
        } else {
            if (new_number - delta > new_number) new_number = 0;
            else new_number -= delta;
        }
        
        // We write into our member variable 'temp_value' because the buffer we return must remain
        // valid until the btree FSM is destroyed. That's why we can't allocate a buffer on the
        // stack.
        
        valuecpy(&temp_value, old_value);
        int chars_written = sprintf(temp_value.value(), "%llu", (unsigned long long)new_number);
        assert(chars_written <= MAX_IN_NODE_VALUE_SIZE); // Not really necessary.
        temp_value.value_size(chars_written);
        
        result = result_success;
        *new_value = &temp_value;
        *new_large_buf = NULL;
        return true;
    }
    
    void call_callback_and_delete() {
        switch (result) {
            case result_success:
                callback->success(new_number);
                break;
            case result_not_found:
                callback->not_found();
                break;
            case result_not_numeric:
                callback->not_numeric();
                break;
            default:
                unreachable();
        }
        
        delete this;
    }
    
private:
    bool increment;   // If false, then decrement
    unsigned long long delta;   // Amount to increment or decrement by
    store_t::incr_decr_callback_t *callback;
    
    /* Used as temporary storage, so that the value we return from operate() doesn't become invalid
    before modify_fsm is done with it. */
    union {
        char temp_value_memory[MAX_TOTAL_NODE_CONTENTS_SIZE+sizeof(btree_value)];
        btree_value temp_value;
    };
    
    enum result_t {
        result_success,
        result_not_found,
        result_not_numeric
    } result;
    uint64_t new_number;
};

void co_btree_incr_decr(btree_key *key, btree_key_value_store_t *store, bool increment, unsigned long long delta, store_t::incr_decr_callback_t *cb) {
    btree_incr_decr_fsm_t *fsm = new btree_incr_decr_fsm_t(increment, delta, cb);
    fsm->run(store, key);
}

void btree_incr_decr(btree_key *key, btree_key_value_store_t *store, bool increment, unsigned long long delta, store_t::incr_decr_callback_t *cb) {
    coro_t::spawn(co_btree_incr_decr, key, store, increment, delta, cb);
}

#endif // __BTREE_SET_FSM_HPP__
