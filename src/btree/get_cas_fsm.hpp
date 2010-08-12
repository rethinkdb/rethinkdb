#ifndef __BTREE_GET_CAS_FSM_HPP__
#define __BTREE_GET_CAS_FSM_HPP__

#include "btree/modify_fsm.hpp"

// This FSM is like get_fsm, except that it sets a CAS value if there isn't one
// already, so it has to be a subclass of modify_fsm. Potentially we can use a
// regular get_fsm for this (that replaces itself with this one if a CAS value
// hasn't been set, for instance), but depending on how CAS is used, that may be
// unnecessary.

template <class config_t>
class btree_get_cas_fsm : public btree_modify_fsm<config_t>,
                          public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, btree_get_cas_fsm<config_t> > {
public:
    explicit btree_get_cas_fsm(btree_key *_key) : btree_modify_fsm<config_t>(_key) {}
    
    bool operate(btree_value *old_value, btree_value **new_value) {
        if (old_value) {
            found = true;

            value.size = old_value->size;
            value.metadata_flags = old_value->metadata_flags;
            memcpy(value.contents, old_value->contents, value.size);
            *new_value = &value;
            if (!value.has_cas()) { // We have always been at war with Eurasia.
                value.set_cas(1); // Turns the flag on and makes room. modify_fsm will set an actual CAS later.
                return true;
            }
            return false;
        } else {
            found = false;
            return false; // Nothing was changed.
        }
    }

    bool found;

    union {
        byte value_memory[MAX_TOTAL_NODE_CONTENTS_SIZE+sizeof(btree_value)];
        btree_value value;
    };
};

#endif // __BTREE_GET_CAS_FSM_HPP__
