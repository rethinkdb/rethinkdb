#ifndef __BTREE_APPEND_PREPEND_FSM_HPP__
#define __BTREE_APPEND_PREPEND_FSM_HPP__

#include "btree/fsm.hpp"
#include "btree/modify_fsm.hpp"

template <class config_t>
class btree_append_prepend_fsm : public btree_modify_fsm<config_t>,
                                 public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, btree_append_prepend_fsm<config_t> > {
public:
    explicit btree_append_prepend_fsm(btree_key *_key, byte *data, unsigned int size, bool append)
        : btree_modify_fsm<config_t>(_key),
          append(append) {
        // This isn't actually used as a value -- just for the size and conte.
        temp_value.size = size;
        memcpy(temp_value.contents, data, size);
    }

    bool operate(btree_value *old_value, btree_value **new_value) {
        if (!old_value) return false;

        assert(old_value->size + temp_value.size <= MAX_IN_NODE_VALUE_SIZE);

        if (append) {
            memcpy(old_value->contents + old_value->size, temp_value.contents, temp_value.size);
        } else { // prepend
            memmove(old_value->contents + temp_value.size, old_value->contents, old_value->size);
            memcpy(old_value->contents, temp_value.contents, temp_value.size);
        }
        old_value->size += temp_value.size;
        *new_value = old_value;
        return true;
    }


private:
        bool append; // If false, then prepend
        union {
            char temp_value_memory[MAX_IN_NODE_VALUE_SIZE+sizeof(btree_value)];
            btree_value temp_value;
        };
};

#endif // __BTREE_APPEND_PREPEND_FSM_HPP__
