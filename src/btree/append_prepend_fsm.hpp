#ifndef __BTREE_APPEND_PREPEND_FSM_HPP__
#define __BTREE_APPEND_PREPEND_FSM_HPP__

#include "btree/fsm.hpp"
#include "btree/modify_fsm.hpp"

class btree_append_prepend_fsm_t :
    public btree_modify_fsm_t,
    public data_provider_t::done_callback_t
{
    typedef btree_fsm_t::transition_result_t transition_result_t;
public:
    explicit btree_append_prepend_fsm_t(btree_key *key, btree_key_value_store_t *store, data_provider_t *data, bool append, store_t::append_prepend_callback_t *cb)
        : btree_modify_fsm_t(key, store), data(data), append(append), callback(cb)
    {
        do_transition(NULL);
    }


    void operate(btree_value *old_value, large_buf_t *old_large_value) {

        if (!old_value) {
            result = result_not_found;
            data->get_value(NULL, this);
            return;
        }

        size_t new_size = old_value->value_size() + data->get_size();
        if (new_size > MAX_VALUE_SIZE) {
            result = result_too_large;
            data->get_value(NULL, this);
            return;
        }

        // Copy flags, exptime, etc.

        valuecpy(&value, old_value);
        value.value_size(new_size);
        
        // Figure out where the data is going to need to go and prepare a place for it
        
        if (new_size <= MAX_IN_NODE_VALUE_SIZE) { // small -> small
            assert(!old_value->is_large());
            // XXX This does unnecessary copying now.
            if (append) {
                buffer_group.add_buffer(data->get_size(), value.value() + old_value->value_size());
            } else { // prepend
                memmove(value.value() + data->get_size(), value.value(), old_value->value_size());
                buffer_group.add_buffer(data->get_size(), value.value());
            }
            large_value = NULL;
            
        } else {
            // Prepare the large value if necessary.
            if (!old_value->is_large()) { // small -> large; allocate a new large value and copy existing value into it.
                large_value = new large_buf_t(this->transaction);
                large_value->allocate(new_size, value.large_buf_ref_ptr());
                if (append) large_value->fill_at(0, old_value->value(), old_value->value_size());
                else        large_value->fill_at(data->get_size(), old_value->value(), old_value->value_size());
                is_old_large_value = false;
            } else { // large -> large; expand existing large value
                large_value = old_large_value;
                if (append) large_value->append(data->get_size(), value.large_buf_ref_ptr());
                else        large_value->prepend(data->get_size(), value.large_buf_ref_ptr());
                is_old_large_value = true;
            }
            
            // Figure out the pointers and sizes where data needs to go
            
            uint32_t fill_size = data->get_size();
            uint32_t start_pos = append ? old_value->value_size() : 0;
            
            int64_t ix = large_value->pos_to_ix(start_pos);
            uint16_t seg_pos = large_value->pos_to_seg_pos(start_pos);
            
            while (fill_size > 0) {
                uint16_t seg_len;
                byte_t *seg = large_value->get_segment_write(ix, &seg_len);

                assert(seg_len >= seg_pos);
                uint16_t seg_bytes_to_fill = std::min((uint32_t)(seg_len - seg_pos), fill_size);
                buffer_group.add_buffer(seg_bytes_to_fill, seg + seg_pos);
                fill_size -= seg_bytes_to_fill;
                seg_pos = 0;
                ix++;
            }
        }
        
        // Dispatch the data request
        
        result = result_success;
        data->get_value(&buffer_group, this);
    }
    
    void have_provided_value() {
        // Called by data provider when it has filled the buffers
        
        if (result == result_success) {
            have_finished_operating(&value, large_value);
        } else {
            have_failed_operating();
        }
    }
    
    void have_failed() {
        // Called by data provider if something goes wrong
        
        if (large_value) {
            if (is_old_large_value) {
                // Some bufs in the large value will have been set dirty (and
                // so new copies will be rewritten unmodified to disk), but
                // that's not really a problem because it only happens on
                // erroneous input.

                if (append) large_value->unappend(data->get_size(), value.large_buf_ref_ptr());
                else large_value->unprepend(data->get_size(), value.large_buf_ref_ptr());
            } else {
                // The old value was small, so we just keep it and delete the large value
                large_value->mark_deleted();
                large_value->release();
                delete large_value;
                large_value = NULL;
            }
        }
        
        result = result_data_provider_failed;
        have_failed_operating();
    }
    
    void call_callback_and_delete() {
        
        switch (result) {
            case result_success:
                callback->success();
                break;
            case result_too_large:
                callback->too_large();
                break;
            case result_not_found:
                callback->not_found();
                break;
            case result_data_provider_failed:
                callback->data_provider_failed();
                break;
        }
        
        delete this;
    }

private:
    enum result_t {
        result_success,
        result_not_found,
        result_too_large,
        result_data_provider_failed
    } result;
    
    data_provider_t *data;
    bool append;   // true = append, false = prepend
    store_t::append_prepend_callback_t *callback;

    union {
        byte value_memory[MAX_TOTAL_NODE_CONTENTS_SIZE+sizeof(btree_value)];
        btree_value value;
    };
    bool is_old_large_value;
    large_buf_t *large_value;
    buffer_group_t buffer_group;
};

#endif // __BTREE_APPEND_PREPEND_FSM_HPP__
