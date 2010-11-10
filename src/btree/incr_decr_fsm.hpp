#ifndef __BTREE_INCR_DECR_FSM_HPP__
#define __BTREE_INCR_DECR_FSM_HPP__

#include "btree/modify_fsm.hpp"

class btree_incr_decr_fsm_t : public btree_modify_fsm_t
{
    typedef btree_fsm_t::transition_result_t transition_result_t;
public:
    explicit btree_incr_decr_fsm_t(btree_key *_key, btree_key_value_store_t *store, bool increment, long long delta)
        : btree_modify_fsm_t(_key, store),
          increment(increment),
          delta(delta)
        {}
    
    transition_result_t operate(btree_value *old_value, large_buf_t *old_large_buf, btree_value **new_value) {
        // If the key didn't exist before, we fail
        if (!old_value) {
            this->status_code = btree_fsm_t::S_NOT_FOUND;
            this->update_needed = false;
            return btree_fsm_t::transition_ok;
        }
        valuecpy(&temp_value, old_value);

        char *endptr;
        new_number = strtoull(old_value->value(), &endptr, 10);
        if (endptr == old_value->value()) {
            this->status_code = btree_fsm_t::S_NOT_NUMERIC;
            this->update_needed = false;
            return btree_fsm_t::transition_ok;
        }

       /*  NOTE: memcached actually does a few things differently:
        *   - If you say `decr 1 -50`, memcached will set 1 to 0 no matter
        *      what it's value is. We on the other hand will add 50 to 1.
        *
        *   - Also, if you say 'incr 1 -50' in memcached and the value
        *     goes below 0, memcached will wrap around. We just set the value to 0.
        */
        
        if (increment) {
            if (delta < 0 && new_number + delta > new_number) new_number = 0;
            else new_number += delta;
        } else {
            if (delta > 0 && new_number - delta > new_number) new_number = 0;
            else new_number -= delta;
        }
        
        // We write into our member variable 'temp_value' because the buffer we return must remain
        // valid until the btree FSM is destroyed. That's why we can't allocate a buffer on the
        // stack.
        
        int chars_written = sprintf(temp_value.value(), "%llu", (unsigned long long)new_number);
        assert(chars_written <= MAX_IN_NODE_VALUE_SIZE); // Not really necessary.
        temp_value.value_size(chars_written);
        
        *new_value = &temp_value;
        
        this->status_code = btree_fsm_t::S_SUCCESS;
        this->update_needed = true;
        return btree_fsm_t::transition_ok;
    }

private:
    bool increment;   // If false, then decrement
    long long delta;   // Amount to increment or decrement by
    
    /* Used as temporary storage, so that the value we return from operate() doesn't become invalid
    before modify_fsm is done with it. */
    union {
        char temp_value_memory[MAX_TOTAL_NODE_CONTENTS_SIZE+sizeof(btree_value)];
        btree_value temp_value;
    };

public:
    /* When the FSM is finished, 'status_code' will be S_SUCCESS if the key was
     * found and new_number will hold the value that it was set to. */
    uint64_t new_number;
};

#endif // __BTREE_SET_FSM_HPP__
