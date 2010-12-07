#ifndef __BTREE_GET_CAS_FSM_HPP__
#define __BTREE_GET_CAS_FSM_HPP__

#include "btree/modify_fsm.hpp"

// This FSM is like get_fsm, except that it sets a CAS value if there isn't one
// already, so it has to be a subclass of modify_fsm. Potentially we can use a
// regular get_fsm for this (that replaces itself with this one if a CAS value
// hasn't been set, for instance), but depending on how CAS is used, that may be
// unnecessary.

class btree_get_cas_fsm_t :
    public btree_modify_fsm_t,
    public store_t::get_callback_t::done_callback_t
{
    typedef btree_fsm_t::transition_result_t transition_result_t;
public:
    explicit btree_get_cas_fsm_t(btree_key *_key, btree_key_value_store_t *store, store_t::get_callback_t *cb)
        : btree_modify_fsm_t(_key, store), callback(cb), in_operate(false)
    {
        pm_cmd_get.begin(&start_time);
        do_transition(NULL);
    }
    
    ~btree_get_cas_fsm_t() {
        pm_cmd_get.end(&start_time);
    }
    
    void operate(btree_value *old_value, large_buf_t *old_large_buf) {
        
        if (!old_value) {
            result = result_not_found;
            have_failed_operating();
        
        } else {
            valuecpy(&value, old_value);

            there_was_cas_before = value.has_cas();
            if (!value.has_cas()) { // We have always been at war with Eurasia.
                value.set_cas(slice->gen_cas());
                this->cas_already_set = true;
            }

            if (value.is_large()) {
                result = result_large_value;
                // Prepare the buffer group
                for (int i = 0; i < old_large_buf->get_num_segments(); i++) {
                    uint16_t size;
                    const void *data = old_large_buf->get_segment(i, &size);
                    buffer_group.add_buffer(size, data);
                }
                
                in_operate = true;   // So we intercept on_cpu_switch()
                if (continue_on_cpu(home_cpu, this)) on_cpu_switch();
                
            } else {
                result = result_small_value;
                done_with_operate();
            }
        }
    }
    
    void done_with_operate() {
        if (there_was_cas_before) {
            have_failed_operating();   // We didn't actually fail, but we made no change
        } else {
            have_finished_operating(&value);
        }
    }
    
    void on_cpu_switch() {
        if (in_operate) {
            if (!have_delivered_value) {
                callback->value(&buffer_group, this, value.mcflags(), value.cas());
            } else {
                in_operate = false;
                done_with_operate();
            }
        } else {
            btree_modify_fsm_t::on_cpu_switch();
        }
    }
    
    void have_copied_value() {
        switch (result) {
            case result_not_found:
                crash("WTF?");       // RSI
            case result_small_value:
                delete this;
                break;
            case result_large_value:
                have_delivered_value = true;
                if (continue_on_cpu(slice->home_cpu, this)) on_cpu_switch();
                break;
        }
    }
    
    void call_callback_and_delete() {
        switch (result) {
            case result_not_found:
                callback->not_found();
                delete this;
                break;
            case result_small_value:
                buffer_group.add_buffer(value.value_size(), value.value());
                callback->value(&buffer_group, this, value.mcflags(), value.cas());
                break;
            case result_large_value:
                // We already delivered our callback during operate().
                delete this;
                break;
        }
    }
    
private:
    store_t::get_callback_t *callback;
    union {
        byte value_memory[MAX_TOTAL_NODE_CONTENTS_SIZE+sizeof(btree_value)];
        btree_value value;
    };
    const_buffer_group_t buffer_group;
    bool in_operate;
    bool have_delivered_value;
    
    ticks_t start_time;
    bool there_was_cas_before;
    enum result_t {
        result_not_found,
        result_small_value,
        result_large_value,
    } result;
};

#endif // __BTREE_GET_CAS_FSM_HPP__
