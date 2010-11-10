#ifndef __BTREE_SET_FSM_HPP__
#define __BTREE_SET_FSM_HPP__

#include "btree/modify_fsm.hpp"

class btree_set_fsm_t : public btree_modify_fsm_t,
                        public large_value_completed_callback
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

    explicit btree_set_fsm_t(btree_key *key, btree_key_value_store_t *store, request_callback_t *req, bool got_large, uint32_t length, byte *data, set_type_t type, btree_value::mcflags_t mcflags, btree_value::exptime_t exptime, btree_value::cas_t req_cas)
        : btree_modify_fsm_t(key, store), length(length), req(req), got_large(got_large), type(type), req_cas(req_cas), read_success(false), set_failed(false), large_value(NULL) {
        
        pm_cmd_set++;
        // XXX This does unnecessary setting and copying.
        value.metadata_flags = 0;
        value.value_size(0);
        value.set_mcflags(mcflags);
        value.set_exptime(exptime);
        value.value_size(length);
        if (!got_large) {
            memcpy(value.value(), data, length);
        }
    }

    transition_result_t operate(btree_value *old_value, large_buf_t *old_large_value, btree_value **_new_value) {
        new_value = _new_value;

        if ((old_value && type == set_type_add) || (!old_value && type == set_type_replace)) {
            this->status_code = btree_fsm_t::S_NOT_STORED;
            set_failed = true;
        }
        if (type == set_type_cas) { // TODO: CAS stats
            if (!old_value || !old_value->has_cas() || old_value->cas() != req_cas) {
                set_failed = true;
                this->status_code = old_value ? btree_fsm_t :: S_EXISTS : btree_fsm_t::S_NOT_FOUND;
            }
        }

        if (set_failed) {
            this->update_needed = false;
            if (got_large) {
                assert(length <= MAX_VALUE_SIZE);
                fill_large_value_msg_t *msg = new fill_large_value_msg_t(return_cpu, req->rh, this, length);
                if (continue_on_cpu(return_cpu, msg)) call_later_on_this_cpu(msg);
                return btree_fsm_t::transition_incomplete;
            }
            read_success = true;
            return btree_fsm_t::transition_ok;
            // TODO: If we implement support for the delete queue (although
            // memcached 1.4.5 doesn't support it): When a value is in the
            // memcached delete queue, you can neither ADD nor REPLACE it, but
            // you *can* SET it.
        }

        if (type == set_type_cas || (old_value && old_value->has_cas())) {
            value.set_cas(0xCA5ADDED); // Turns the flag on and makes room. modify_fsm will set an actual CAS later. TODO: We should probably have a separate function for this.
        }

        if (got_large) {
            assert (length <= MAX_VALUE_SIZE);
            large_value = new large_buf_t(this->transaction);
            large_value->allocate(length);
            value.set_lv_index_block_id(large_value->get_index_block_id());

            fill_large_value_msg_t *msg = new fill_large_value_msg_t(large_value, return_cpu, req->rh, this, 0, length);

            // continue_on_cpu() returns true if we are already on that cpu, but we don't want to
            // call the callback immediately in that case anyway.
            if (continue_on_cpu(return_cpu, msg))  {
                call_later_on_this_cpu(msg);
            }
            return btree_fsm_t::transition_incomplete;
        }

        read_success = true;
        return btree_fsm_t::transition_ok;
    }

    void on_operate_completed() {
        if (set_failed) {
            this->update_needed = false;
            if (!read_success) this->status_code = btree_fsm_t::S_READ_FAILURE;
            return;
        }
        this->update_needed = read_success;
        if (read_success) {
            *new_value = &value;
            this->status_code = btree_fsm_t::S_SUCCESS;
        } else {
            if (got_large) {
                assert(large_value);
                large_value->mark_deleted();
                this->status_code = btree_fsm_t::S_READ_FAILURE;
            } else {
                // status_code is already set.
                assert(this->status_code != btree_fsm_t::S_UNKNOWN);
            }
        }
        if (large_value) {
            large_value->release();
            delete large_value;
            large_value = NULL;
        }
    }

    void on_large_value_completed(bool _read_success) {
        read_success = _read_success;
        this->step(); // XXX This should go elsewhere.
    }

private:
    uint32_t length;

    request_callback_t *req;

    bool got_large;

    set_type_t type;
    btree_value::cas_t req_cas;

    bool read_success; // XXX: Rename this.

    bool set_failed;

    btree_value **new_value;

    union {
        char value_memory[MAX_TOTAL_NODE_CONTENTS_SIZE+sizeof(btree_value)];
        btree_value value;
    };
    large_buf_t *large_value;
};

#endif // __BTREE_SET_FSM_HPP__
