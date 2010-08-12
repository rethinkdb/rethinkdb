
#ifndef __BTREE_SET_FSM_HPP__
#define __BTREE_SET_FSM_HPP__

#include "btree/modify_fsm.hpp"

template <class config_t>
class btree_set_fsm : public btree_modify_fsm<config_t>,
                      public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, btree_set_fsm<config_t> >
{

public:
    explicit btree_set_fsm(btree_key *_key, byte *data, unsigned int length, btree_value::mcflags_t mcflags, btree_value::exptime_t exptime, bool add_ok, bool replace_ok, btree_value::cas_t _req_cas, bool check_cas)
        : btree_modify_fsm<config_t>(_key),
          add_ok(add_ok),
          replace_ok(replace_ok),
          check_cas(check_cas)
        {
        req_cas = _req_cas;
        value.metadata_flags = 0;
        value.value_size(length);
        value.set_mcflags(mcflags);
        value.set_exptime(exptime);
        memcpy(value.value(), data, length);
    }
    
    bool operate(btree_value *old_value, btree_value **new_value) {
        if ((!old_value && !add_ok) || (old_value && !replace_ok)) {
            this->status_code = btree_fsm<config_t>::S_NOT_STORED;
            return false;
        }
        if (!old_value) {
            get_cpu_context()->worker->curr_items++;
            get_cpu_context()->worker->total_items++;
        }
        if (check_cas) {
            if (old_value && old_value->has_cas() && old_value->cas() != req_cas) {
                this->status_code = btree_fsm<config_t>::S_EXISTS;
                return false;
            }
        }
        if (old_value && old_value->has_cas()) {
            value.set_cas(1); // Turns the flag on and makes room. modify_fsm will set an actual CAS later.
        }
        *new_value = &value;
        this->status_code = btree_fsm<config_t>::S_SUCCESS;
        return true;
    }

private:
    bool add_ok, replace_ok, check_cas;
    btree_value::cas_t req_cas;
    union {
        char value_memory[MAX_TOTAL_NODE_CONTENTS_SIZE+sizeof(btree_value)];
        btree_value value;
    };
};

#endif // __BTREE_SET_FSM_HPP__
