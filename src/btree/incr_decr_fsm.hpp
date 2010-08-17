
#ifndef __BTREE_INCR_DECR_FSM_HPP__
#define __BTREE_INCR_DECR_FSM_HPP__

#include "btree/modify_fsm.hpp"

template <class config_t>
class btree_incr_decr_fsm : public btree_modify_fsm<config_t>,
                            public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, btree_incr_decr_fsm<config_t> >
{

public:
    explicit btree_incr_decr_fsm(btree_key *_key, bool increment, long long delta)
        : btree_modify_fsm<config_t>(_key),
          increment(increment),
          delta(delta)
        {}
    
    bool operate(btree_value *old_value, btree_value **new_value) {
        // If the key didn't exist before, we fail
        if (!old_value) {
            this->status_code = btree_fsm<config_t>::S_NOT_FOUND;
            return false;
        }
        valuecpy(&temp_value, old_value);

        new_number = strtoull(old_value->contents, NULL, 10);
        // TODO: Handle non-numeric values.

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
        temp_value.value_size(chars_written);
        
        *new_value = &temp_value;
        
        this->status_code = btree_fsm<config_t>::S_SUCCESS;
        return true;
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
