#ifndef __BTREE_GET_CAS_FSM_HPP__
#define __BTREE_GET_CAS_FSM_HPP__

#include "btree/modify_fsm.hpp"

// This FSM is like get_fsm, except that it sets a CAS value if there isn't one
// already, so it has to be a subclass of modify_fsm. Potentially we can use a
// regular get_fsm for this (that replaces itself with this one if a CAS value
// hasn't been set, for instance), but depending on how CAS is used, that may be
// unnecessary.

class btree_get_cas_fsm_t : public btree_modify_fsm_t,
                            public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, btree_get_cas_fsm_t> {
    typedef btree_fsm_t::transition_result_t transition_result_t;
public:
    explicit btree_get_cas_fsm_t(btree_key *_key, btree_key_value_store_t *store) : btree_modify_fsm_t(_key, store) {}
    
    transition_result_t operate(btree_value *old_value, large_buf_t *old_large_buf, btree_value **new_value) {
        if (old_value) {
            assert(!old_value->large_value());
            this->status_code = btree_fsm_t::S_SUCCESS;

            valuecpy(&value, old_value);
            *new_value = &value;
            if (!value.has_cas()) { // We have always been at war with Eurasia.
                value.set_cas(1); // Turns the flag on and makes room. modify_fsm will set an actual CAS later.
                this->update_needed = true;
                return btree_fsm_t::transition_ok;
            }
            this->update_needed = false;
            return btree_fsm_t::transition_ok;
        } else {
            this->status_code = btree_fsm_t::S_NOT_FOUND;
            this->update_needed = false;
            return btree_fsm_t::transition_ok; // Nothing was changed.
        }
    }

    union {
        byte value_memory[MAX_TOTAL_NODE_CONTENTS_SIZE+sizeof(btree_value)];
        btree_value value;
    };
};

#endif // __BTREE_GET_CAS_FSM_HPP__
