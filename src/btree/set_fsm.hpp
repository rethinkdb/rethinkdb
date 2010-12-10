#ifndef __BTREE_SET_FSM_HPP__
#define __BTREE_SET_FSM_HPP__

#include "btree/modify_fsm.hpp"

class btree_set_fsm_t :
    public btree_modify_fsm_t,
    public data_provider_t::done_callback_t
{
private:
    typedef btree_fsm_t::transition_result_t transition_result_t;

public:
    enum set_type_t {
        set_type_set,
        set_type_add,
        set_type_replace,
        set_type_cas
    };

    explicit btree_set_fsm_t(btree_key *key, btree_key_value_store_t *store, data_provider_t *data, set_type_t type, btree_value::mcflags_t mcflags, btree_value::exptime_t exptime, btree_value::cas_t req_cas, store_t::set_callback_t *cb)
        : btree_modify_fsm_t(key, store), data(data), type(type), mcflags(mcflags), exptime(exptime), req_cas(req_cas), large_value(NULL), callback(cb)
    {
        pm_cmd_set.begin(&start_time);
        do_transition(NULL);
    }
    
    ~btree_set_fsm_t() {
        pm_cmd_set.end(&start_time);
    }


    void operate(btree_value *old_value, large_buf_t *old_large_value) {

        if ((old_value && type == set_type_add) || (!old_value && type == set_type_replace)) {
            result = result_not_stored;
            data->get_value(NULL, this);
            return;
        }
        
        if (type == set_type_cas) { // TODO: CAS stats
            if (!old_value) {
                result = result_not_found;
                data->get_value(NULL, this);
                return;
            }
            if (!old_value->has_cas() || old_value->cas() != req_cas) {
                result = result_exists;
                data->get_value(NULL, this);
                return;
            }
        }
        
        if (data->get_size() > MAX_VALUE_SIZE) {
            result = result_too_large;
            data->get_value(NULL, this);
            return;
        }
        
        value.metadata_flags = 0;
        value.value_size(0);
        value.set_mcflags(mcflags);
        value.set_exptime(exptime);
        value.value_size(data->get_size());
        if (type == set_type_cas || (old_value && old_value->has_cas())) {
            value.set_cas(0xCA5ADDED); // Turns the flag on and makes room. modify_fsm will set an actual CAS later. TODO: We should probably have a separate function for this.
        }
        
        assert(data->get_size() <= MAX_VALUE_SIZE);
        if (data->get_size() <= MAX_IN_NODE_VALUE_SIZE) {
            buffer_group.add_buffer(data->get_size(), value.value());
        } else {
            large_value = new large_buf_t(this->transaction);
            large_value->allocate(data->get_size(), value.large_buf_ref_ptr());
            for (int64_t i = 0; i < large_value->get_num_segments(); i++) {
                uint16_t size;
                void *data = large_value->get_segment_write(i, &size);
                buffer_group.add_buffer(size, data);
            }
        }
        
        result = result_stored;
        data->get_value(&buffer_group, this);
    }
    
    void have_provided_value() {
        /* Called by the data provider when it filled the buffer we gave it. */
        
        if (result == result_stored) {
            have_finished_operating(&value, large_value);
        } else if (result == result_too_large) {
            /* To be standards-compliant we must delete the old value when an effort is made to
            replace it with a value that is too large. */
            have_finished_operating(NULL, NULL);
        } else {
            have_failed_operating();
        }
    }
    
    void have_failed() {
        /* Called by the data provider when something went wrong. */
        
        if (large_value) {
            large_value->mark_deleted();
            large_value->release();
            delete large_value;
            large_value = NULL;
        }
        
        result = result_data_provider_failed;
        have_failed_operating();
    }
    
    void call_callback_and_delete() {
        
        switch (result) {
            case result_stored:
                callback->stored();
                break;
            case result_not_stored:
                callback->not_stored();
                break;
            case result_not_found:
                callback->not_found();
                break;
            case result_exists:
                callback->exists();
                break;
            case result_too_large:
                callback->too_large();
                break;
            case result_data_provider_failed:
                callback->data_provider_failed();
                break;
        }
        
        delete this;
    }
    
private:
    ticks_t start_time;

    data_provider_t *data;
    set_type_t type;
    mcflags_t mcflags;
    exptime_t exptime;
    btree_value::cas_t req_cas;
    
    enum result_t {
        result_stored,
        result_not_stored,
        result_not_found,
        result_exists,
        result_too_large,
        result_data_provider_failed
    } result;

    union {
        char value_memory[MAX_TOTAL_NODE_CONTENTS_SIZE+sizeof(btree_value)];
        btree_value value;
    };
    large_buf_t *large_value;
    buffer_group_t buffer_group;
    
    store_t::set_callback_t *callback;
};

#endif // __BTREE_SET_FSM_HPP__
