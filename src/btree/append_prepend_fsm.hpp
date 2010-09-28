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
        : btree_modify_fsm_t(key, store), req(req), got_large(got_large), extra_size(size), append(append), success(false) {
        if (!got_large) {
            assert(size <= MAX_IN_NODE_VALUE_SIZE);
            memcpy(extra_data, data, size);
        }
    }

    transition_result_t operate(btree_value *_old_value, large_buf_t *old_large_value, btree_value **_new_value) {
        old_value = _old_value;
        new_value = _new_value;

        if (!old_value) {
            this->status_code = btree_fsm_t::S_NOT_FOUND;
            this->update_needed = false;
            return btree_fsm_t::transition_ok;
            // XXX: If this was a large value, we have to consume it and discard it. Figure out a nice way to do this...
        }

        valuecpy(&value, old_value);
        new_size = old_value->value_size() + extra_size;

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
            if (append) msg = new fill_large_value_msg_t(large_value, req, this, old_value->value_size(), extra_size);
            else        msg = new fill_large_value_msg_t(large_value, req, this, 0, extra_size);
            msg->return_cpu = get_cpu_context()->event_queue->message_hub.current_cpu;
            msg->send(this->return_cpu);
            return btree_fsm_t::transition_incomplete;
        }
    }

    void on_operate_completed() {
        if (!old_value) return; // XXX
        if (!got_large || success) {
            do_append_successful(); // No possibility of a read error.
            return;
        }
        if (!got_large || success) {
            do_append_successful();
        } else { // We read data from the socket and failed (e.g. we didn't get \r\n after the input in text mode). Undo it.
            if (!old_value->is_large()) { // The old value was small, so we just restore it.
                //this->status_code
                this->update_needed = false;
            } else {
            }
            assert(0);
        }
    }

    void on_large_value_completed(bool _success) {
        success = _success;
        this->step();
    }

private:
    void do_append_successful() {
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

    uint32_t new_size;
    uint32_t extra_size;
    byte extra_data[MAX_IN_NODE_VALUE_SIZE];

    bool append; // If false, then prepend.

    union {
        byte value_memory[MAX_TOTAL_NODE_CONTENTS_SIZE+sizeof(btree_value)];
        btree_value value;
    };

    bool success;

    btree_value *old_value;
    btree_value **new_value;
    large_buf_t *large_value;
};

#endif // __BTREE_APPEND_PREPEND_FSM_HPP__
