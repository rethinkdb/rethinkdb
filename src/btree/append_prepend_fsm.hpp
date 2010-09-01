#ifndef __BTREE_APPEND_PREPEND_FSM_HPP__
#define __BTREE_APPEND_PREPEND_FSM_HPP__

#include "btree/fsm.hpp"
#include "btree/modify_fsm.hpp"

template <class config_t>
class btree_append_prepend_fsm : public btree_modify_fsm<config_t>,
                                 public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, btree_append_prepend_fsm<config_t> > {
    typedef typename config_t::large_buf_t large_buf_t;
    typedef typename config_t::btree_fsm_t btree_fsm_t;
    typedef typename btree_fsm_t::transition_result_t transition_result_t;
public:
    explicit btree_append_prepend_fsm(btree_key *_key, byte *data, unsigned int size, bool append)
        : btree_modify_fsm<config_t>(_key),
          append(append) {
        // This isn't actually used as a btree value -- just for the size and contents.
        temp_value.metadata_flags = 0;
        temp_value.value_size(size);
        memcpy(temp_value.value(), data, size);
    }

    transition_result_t operate(btree_value *old_value, large_buf_t *old_large_buf, btree_value **new_value) {
        if (!old_value) {
            this->status_code = btree_fsm_t::S_NOT_FOUND;
            this->update_needed = false;
            return btree_fsm_t::transition_ok;
        }
        assert(!old_value->large_value());

        assert(old_value->mem_size() + temp_value.value_size() <= MAX_TOTAL_NODE_CONTENTS_SIZE);

        valuecpy(&value, old_value);

        if (append) {
            memcpy(value.value() + value.value_size(), temp_value.contents, temp_value.size);
        } else { // prepend
            memmove(value.value() + temp_value.size, value.value(), value.value_size());
            memcpy(value.value(), temp_value.contents, temp_value.size);
        }
        value.value_size(old_value->value_size() + temp_value.value_size());
        value.size += temp_value.size;
        *new_value = &value;
        this->status_code = btree_fsm_t::S_SUCCESS;
        this->update_needed = true;
        return btree_fsm_t::transition_ok;
    }


private:
        bool append; // If false, then prepend
        union {
            byte temp_value_memory[MAX_TOTAL_NODE_CONTENTS_SIZE+sizeof(btree_value)];
            btree_value temp_value;
        };
        union {
            byte value_memory[MAX_TOTAL_NODE_CONTENTS_SIZE+sizeof(btree_value)];
            btree_value value;
        };
};

#endif // __BTREE_APPEND_PREPEND_FSM_HPP__
