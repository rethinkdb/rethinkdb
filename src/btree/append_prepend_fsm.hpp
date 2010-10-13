#ifndef __BTREE_APPEND_PREPEND_FSM_HPP__
#define __BTREE_APPEND_PREPEND_FSM_HPP__

#include "btree/fsm.hpp"
#include "btree/modify_fsm.hpp"

// When large-value append/prepend is involved, there are five possible cases:
// small + small = small
// small + small = large
// small + large = large
// large + small = large
// large + large = large

class btree_append_prepend_fsm_t : public btree_modify_fsm_t,
                                   public large_value_completed_callback,
                                   public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, btree_append_prepend_fsm_t > {
    typedef btree_fsm_t::transition_result_t transition_result_t;
public:
    explicit btree_append_prepend_fsm_t(btree_key *key, btree_key_value_store_t *store, request_callback_t *req, bool got_large, unsigned int size, byte *data, bool append)
        : btree_modify_fsm_t(key, store), req(req), got_large(got_large), extra_size(size), append(append), read_success(false) {
        if (!got_large) {
            assert(size <= MAX_IN_NODE_VALUE_SIZE);
            memcpy(extra_data, data, size);
        }
    }

    transition_result_t operate(btree_value *_old_value, large_buf_t *old_large_value, btree_value **_new_value) {
        old_value = _old_value;
        new_value = _new_value;

        if (!old_value) {
            if (got_large) {
                fill_large_value_msg_t *msg = new fill_large_value_msg_t(return_cpu, req->rh, this, extra_size);
                if (continue_on_cpu(return_cpu, msg)) call_later_on_this_cpu(msg);
                return btree_fsm_t::transition_incomplete;
            }
            return btree_fsm_t::transition_ok;
        }

        valuecpy(&value, old_value);
        new_size = old_value->value_size() + extra_size;
        if (new_size > MAX_VALUE_SIZE) {
            return btree_fsm_t::transition_ok;
        }

        if (new_size <= MAX_IN_NODE_VALUE_SIZE) { // small + small = small
            assert(!old_value->is_large());
            assert(!got_large);
            // XXX This does unnecessary copying now.
            if (append) {
                memcpy(value.value() + value.value_size(), extra_data, extra_size);
            } else { // prepend
                memmove(value.value() + extra_size, value.value(), value.value_size());
                memcpy(value.value(), extra_data, extra_size);
            }

            return btree_fsm_t::transition_ok;
        }

        // Prepare the large value if necessary.
        if (!old_value->is_large()) { // small + _ = large; allocate a new large value and copy existing value into it.
            large_value = new large_buf_t(this->transaction);
            large_value->allocate(new_size);
            if (append) large_value->fill_at(0, old_value->value(), old_value->value_size());
            else        large_value->fill_at(extra_size, old_value->value(), old_value->value_size());
        } else { // large + _ = large
            large_value = old_large_value;
            if (append) large_value->append(extra_size);
            else        large_value->prepend(extra_size);
        }

        // Fill the large value with the new data if necessary.
        if (!got_large) { // _ + small = large; copy the received data into the large value;
            if (append) large_value->fill_at(old_value->value_size(), extra_data, extra_size);
            else        large_value->fill_at(0,                       extra_data, extra_size);
            return btree_fsm_t::transition_ok;
        } else { // _ + large = large; we got a large value so we're going to read it directly from the socket.
            fill_large_value_msg_t *msg;
            if (append) msg = new fill_large_value_msg_t(large_value, return_cpu, req->rh, this, old_value->value_size(), extra_size);
            else        msg = new fill_large_value_msg_t(large_value, return_cpu, req->rh, this, 0, extra_size);
            // continue_on_cpu() returns true if we are already on that cpu,
            // but we don't want to call the callback immediately in that case
            // anyway.
            if (continue_on_cpu(return_cpu, msg))  {
                call_later_on_this_cpu(msg);
            }
            return btree_fsm_t::transition_incomplete;
        }
    }

    void on_operate_completed() {
        if (!old_value) {
            this->update_needed = false;
            this->status_code = (!got_large || read_success)
                              ? btree_fsm_t::S_NOT_FOUND
                              : btree_fsm_t::S_READ_FAILURE;
            return;
        }
        if (new_size > MAX_VALUE_SIZE) {
            this->status_code = btree_fsm_t::S_TOO_LARGE;
            this->update_needed = false;
            return;
        }
        if (!got_large || read_success) {
            do_append_read_successful(); // No possibility of a read error.
        } else { // We read data from the socket and failed (e.g. we didn't get \r\n after the input in text mode). Undo it.
            this->status_code = btree_fsm_t::S_READ_FAILURE;
            this->update_needed = false;
            if (!old_value->is_large()) { // The old value was small, so we just keep it and delete the large value.
                large_value->mark_deleted();
                large_value->release();
                delete large_value;
                large_value = NULL;
            } else {
                // Some bufs in the large value will have been set dirty (and
                // so new copies will be rewritten unmodified to disk), but
                // that's not really a problem because it only happens on
                // erroneous input.
                if (append) large_value->unappend(extra_size);
                else        large_value->unprepend(extra_size);
            }
        }
    }

    void on_large_value_completed(bool _read_success) {
        read_success = _read_success;
        this->step();
    }

private:
    void do_append_read_successful() {
        value.value_size(new_size);
        if (value.is_large()) {
            value.set_lv_index_block_id(large_value->get_index_block_id());
            if (!old_value->is_large()) { // If we're adding to an existing large value, modify_fsm will release it later, so we don't need to.
                large_value->release();
                delete large_value;
            }
        }
        *new_value = &value;
        this->status_code = btree_fsm_t::S_SUCCESS;
        this->update_needed = true;
    }


private:
    request_callback_t *req;

    bool got_large;
    //bool append_prepend_failed; // XXX: Rename this.

    uint32_t new_size;
    uint32_t extra_size;
    byte extra_data[MAX_IN_NODE_VALUE_SIZE];

    bool append; // If false, then prepend.

    union {
        byte value_memory[MAX_TOTAL_NODE_CONTENTS_SIZE+sizeof(btree_value)];
        btree_value value;
    };

    bool read_success; // XXX: Rename this.

    btree_value *old_value;
    btree_value **new_value;
    large_buf_t *large_value;
};

#endif // __BTREE_APPEND_PREPEND_FSM_HPP__
