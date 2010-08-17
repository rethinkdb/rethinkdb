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
        // This isn't actually used as a btree value -- just for the size and contents.
        temp_value.size = size;
        memcpy(temp_value.contents, data, size);
    }

    bool operate(btree_value *old_value, btree_value **new_value) {
        if (!old_value) {
            this->status_code = btree_fsm<config_t>::S_NOT_FOUND;
            return false;
        }

        assert(old_value->size + temp_value.size <= MAX_TOTAL_NODE_CONTENTS_SIZE);

        if (append) {
            memcpy(old_value->value() + old_value->value_size(), temp_value.contents, temp_value.size);
        } else { // prepend
            memmove(old_value->value() + temp_value.size, old_value->value(), old_value->value_size());
            memcpy(old_value->value(), temp_value.contents, temp_value.size);
        }
        old_value->size += temp_value.size;
        *new_value = old_value;
        this->status_code = btree_fsm<config_t>::S_SUCCESS;
        return true;
    }


private:
        bool append; // If false, then prepend
        union {
            char temp_value_memory[MAX_TOTAL_NODE_CONTENTS_SIZE+sizeof(btree_value)];
            btree_value temp_value;
        };
};

#endif // __BTREE_APPEND_PREPEND_FSM_HPP__
