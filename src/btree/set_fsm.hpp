#ifndef __BTREE_SET_FSM_HPP__
#define __BTREE_SET_FSM_HPP__

#include "btree/modify_fsm.hpp"

class btree_set_fsm_t : public btree_modify_fsm_t,
                        public large_value_completed_callback,
                        public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, btree_set_fsm_t> {
private:
    typedef btree_fsm_t::transition_result_t transition_result_t;

public:
    enum set_type_t {
        set_type_set,
        set_type_add,
        set_type_replace,
        set_type_cas
    };

    explicit btree_set_fsm_t(btree_key *key, request_callback_t *req, byte *data, uint32_t length, set_type_t type, btree_value::mcflags_t mcflags, btree_value::exptime_t exptime, btree_value::cas_t req_cas)
        : btree_modify_fsm_t(key), length(length), req(req), type(type), req_cas(req_cas), success(false), new_large_value(NULL) {
        // XXX This does unnecessary setting and copying.
        value.metadata_flags = 0;
        value.value_size(0);
        value.set_mcflags(mcflags);
        value.set_exptime(exptime);
        value.value_size(length);
        if (length <= MAX_IN_NODE_VALUE_SIZE) {
            assert(!value.large_value());
            memcpy(value.value(), data, length);
        }
    }

    transition_result_t operate(btree_value *old_value, large_buf_t *old_large_value, btree_value **_new_value) { //, large_buf_t **new_large_value) {//, large_buf_t **large_value) {
        new_value = _new_value;
        // TODO: If we return false, we still need to consume the value from the socket.
        if (length > MAX_IN_NODE_VALUE_SIZE) {
        }

        if ((old_value && type == set_type_add) || (!old_value && type == set_type_replace)) {
            this->status_code = btree_fsm_t::S_NOT_STORED;
            this->update_needed = false;
            return btree_fsm_t::transition_ok;
            // TODO: If the value was large, we still need to consume it.
            // TODO: If we implement support for the delete queue (although
            // memcached 1.4.5 doesn't support it): When a value is in the
            // memcached delete queue, you can neither ADD nor REPLACE it, but
            // you *can* SET it.
        }
        if (type == set_type_cas) { // TODO: CAS stats
            if (!old_value) {
                this->status_code = btree_fsm_t::S_NOT_FOUND;
                this->update_needed = false;
                return btree_fsm_t::transition_ok;
            }
            if (!old_value->has_cas() || old_value->cas() != req_cas) {
                this->status_code = btree_fsm_t::S_EXISTS;
                this->update_needed = false;
                return btree_fsm_t::transition_ok;
            }
            value.set_cas(0xCA5ADDED); // Turns the flag on and makes room. modify_fsm will set an actual CAS later. TODO: We should probably have a separate function for this.
        }
        
        if (length > MAX_IN_NODE_VALUE_SIZE) { // XXX Maybe this should be a bool from the request handler.
            new_large_value = new large_buf_t(this->transaction);
            new_large_value->allocate(length);
            value.set_lv_index_block_id(new_large_value->get_index_block_id());
            //*new_value = &value; // XXX Only do this if we filled the value successfully.
            
            read_large_value_msg_t *msg = new read_large_value_msg_t(new_large_value, req, this);
            
            // continue_on_cpu() returns true if we are already on that cpu, but we don't want to
            // call the callback immediately in that case anyway.
            if (continue_on_cpu(return_cpu, msg))  {
                call_later_on_this_cpu(msg);
            }
            
            // XXX Figure out where things are deleted.
            return btree_fsm_t::transition_incomplete;
        }

        *new_value = &value;
        this->status_code = btree_fsm_t::S_SUCCESS;
        this->update_needed = true;
        return btree_fsm_t::transition_ok;
    }

    void on_operate_completed() {
        if (new_large_value) {
            if (success) {
                *new_value = &value;
                new_large_value->set_dirty();
                new_large_value->release();
                delete new_large_value;
                new_large_value = NULL;
                this->status_code = btree_fsm_t::S_SUCCESS;
                this->update_needed = true;
            } else {
                // TODO: Make sure that allocate and delete in the same transaction is a no-op.
                new_large_value->mark_deleted();
                new_large_value->set_dirty();
                new_large_value->release();
                delete new_large_value;
                new_large_value = NULL;
                this->status_code = btree_fsm_t::S_UNKNOWN;
                this->update_needed = false;
            }
        }
    }

    void on_large_value_completed(bool _success) {
        success = _success;
        this->step(); // XXX This should go elsewhere.
    }

private:
    uint32_t length;

    request_callback_t *req;

    set_type_t type;
    btree_value::cas_t req_cas;

    bool success;

    btree_value **new_value;

    union {
        char value_memory[MAX_TOTAL_NODE_CONTENTS_SIZE+sizeof(btree_value)];
        btree_value value;
    };
    large_buf_t *new_large_value;
};

#endif // __BTREE_SET_FSM_HPP__
