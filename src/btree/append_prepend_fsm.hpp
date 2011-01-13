#ifndef __BTREE_APPEND_PREPEND_FSM_HPP__
#define __BTREE_APPEND_PREPEND_FSM_HPP__

#include "btree/modify_fsm.hpp"

#include "btree/coro_wrappers.hpp"

class btree_append_prepend_fsm_t :
    public btree_modify_fsm_t
{
public:
    explicit btree_append_prepend_fsm_t(data_provider_t *data, bool append, store_t::append_prepend_callback_t *cb)
        : btree_modify_fsm_t(), data(data), append(append), callback(cb)
    { }

    bool operate(btree_value *old_value, large_buf_t *old_large_value, btree_value **new_value, large_buf_t **new_large_buf) {
        if (!old_value) {
            result = result_not_found;
            co_get_data_provider_value(data, NULL);
            return false;
        }

        size_t new_size = old_value->value_size() + data->get_size();
        if (new_size > MAX_VALUE_SIZE) {
            result = result_too_large;
            co_get_data_provider_value(data, NULL);
            return false;
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
        bool success = co_get_data_provider_value(data, &buffer_group);
        if (!success) {
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
            return false;
        }

        *new_value = &value;
        *new_large_buf = large_value;
        return true;
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
            default:
                unreachable();
        }
        
        delete this;
    }

protected:
    void actually_acquire_large_value(large_buf_t *lb, const large_buf_ref& lbref) {
        if (append) {
            co_acquire_large_value_rhs(lb, lbref, rwi_write);
        } else {
            co_acquire_large_value_lhs(lb, lbref, rwi_write);
        }
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

void co_btree_append_prepend(btree_key *key, btree_key_value_store_t *store, data_provider_t *data, bool append, store_t::append_prepend_callback_t *cb) {
    btree_append_prepend_fsm_t *fsm = new btree_append_prepend_fsm_t(data, append, cb);
    fsm->run(store, key);
}

void btree_append_prepend(btree_key *key, btree_key_value_store_t *store, data_provider_t *data, bool append, store_t::append_prepend_callback_t *cb) {
    coro_t::spawn(co_btree_append_prepend, key, store, data, append, cb);
}

#endif // __BTREE_APPEND_PREPEND_FSM_HPP__
