
#ifndef __BTREE_SET_FSM_HPP__
#define __BTREE_SET_FSM_HPP__

#include "btree/modify_fsm.hpp"

template <class config_t>
class btree_set_fsm : public btree_modify_fsm<config_t>,
                      public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, btree_set_fsm<config_t> >
{

public:
    explicit btree_set_fsm(btree_key *_key, byte *data, unsigned int length, bool add_ok, bool replace_ok)
        : btree_modify_fsm<config_t>(_key),
          add_ok(add_ok),
          replace_ok(replace_ok)
        {
        value.size = length;
        memcpy(&value.contents, data, length);
    }
    
    bool operate(btree_value *old_value, btree_value **new_value) {
        if ((!old_value && !add_ok) || (old_value && !replace_ok)) return false;
        *new_value = &value;
        return true;
    }

private:
    bool add_ok, replace_ok;
    union {
        char value_memory[MAX_IN_NODE_VALUE_SIZE+sizeof(btree_value)];
        btree_value value;
    };
};

#endif // __BTREE_SET_FSM_HPP__
