#ifndef __BTREE_GET_CAS_FSM_HPP__
#define __BTREE_GET_CAS_FSM_HPP__

#include "btree/modify_fsm.hpp"

template <class config_t>
class btree_get_cas_fsm : public btree_modify_fsm<config_t>,
                          public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, btree_get_cas_fsm<config_t> > {
public:
    explicit btree_get_cas_fsm(btree_key *_key) : btree_modify_fsm<config_t>(_key) {}
    
    bool operate(btree_value *old_value, btree_value **new_value) {
        value = old_value;
        if (value) {
            found = true;
            *new_value = value;
            if (!value->has_cas()) { // We have always been at war with Eurasia.
                assert(value->size + sizeof(uint64_t) <= MAX_TOTAL_NODE_CONTENTS_SIZE);
                value->add_cas(); // Will be set in modify_fsm.
                return true;
            }
            return false;
        } else {
            found = false;
            return false; // Nothing was changed.
        }
    }

    bool found;

    btree_value *value;
};

#endif // __BTREE_GET_CAS_FSM_HPP__
