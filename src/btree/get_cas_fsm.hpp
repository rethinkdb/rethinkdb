#ifndef __BTREE_GET_CAS_FSM_HPP__
#define __BTREE_GET_CAS_FSM_HPP__

#include "btree/modify_fsm.hpp"

// This FSM is like get_fsm, except that it sets a CAS value if there isn't one
// already, so it has to be a subclass of modify_fsm. Potentially we can use a
// regular get_fsm for this (that replaces itself with this one if a CAS value
// hasn't been set, for instance), but depending on how CAS is used, that may be
// unnecessary.

class btree_get_cas_fsm_t : public btree_modify_fsm_t,
                            public large_value_completed_callback
{
    typedef btree_fsm_t::transition_result_t transition_result_t;
public:
    explicit btree_get_cas_fsm_t(btree_key *_key, btree_key_value_store_t *store, request_callback_t *req)
        : btree_modify_fsm_t(_key, store), req(req), write_lv_msg(NULL), lv_state(lv_state_unknown)
    {
        pm_cmd_get.begin(&start_time);
    }
    
    ~btree_get_cas_fsm_t() {
        pm_cmd_get.end(&start_time);
    }
    
    transition_result_t operate(btree_value *old_value, large_buf_t *old_large_buf, btree_value **new_value) {
        this->update_needed = false;

        if (old_value) {
            this->status_code = btree_fsm_t::S_SUCCESS;

            valuecpy(&value, old_value);

            if (!value.has_cas()) { // We have always been at war with Eurasia.
                value.set_cas(slice->gen_cas());
                this->cas_already_set = true;
                *new_value = &value;
                this->update_needed = true;
            }

            if (value.is_large()) {
                lv_state = lv_state_ready;
                write_lv_msg = new write_large_value_msg_t(old_large_buf, this, return_cpu, req, this);
                write_lv_msg->dispatch();
                return btree_fsm_t::transition_incomplete;
            }

        } else {
            this->status_code = btree_fsm_t::S_NOT_FOUND;
        }
        return btree_fsm_t::transition_ok;
    }

    void on_large_value_completed(bool success) {
        assert(success);
        write_lv_msg = NULL;
        lv_state = lv_state_done;
        this->step();
    }

    void begin_lv_write() {
        lv_state = lv_state_writing;
        write_lv_msg->begin_write();
    }

    union {
        byte value_memory[MAX_TOTAL_NODE_CONTENTS_SIZE+sizeof(btree_value)];
        btree_value value;
    };

private:
    request_callback_t *req;
    ticks_t start_time;
public:
    write_large_value_msg_t *write_lv_msg;

    enum large_value_state {
        lv_state_unknown,
        lv_state_ready,
        lv_state_writing,
        lv_state_done
    } lv_state;
};

#endif // __BTREE_GET_CAS_FSM_HPP__
