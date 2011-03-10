#include <boost/shared_ptr.hpp>
#include "set.hpp"
#include "btree/modify_oper.hpp"
#include "buffer_cache/co_functions.hpp"

struct btree_set_oper_t : public btree_modify_oper_t {
    explicit btree_set_oper_t(data_provider_t *data, mcflags_t mcflags, exptime_t exptime,
            add_policy_t ap, replace_policy_t rp, cas_t req_cas)
        : btree_modify_oper_t(), data(data), mcflags(mcflags), exptime(exptime),
            add_policy(ap), replace_policy(rp), req_cas(req_cas)
    {
        pm_cmd_set.begin(&start_time);
    }

    ~btree_set_oper_t() {
        pm_cmd_set.end(&start_time);
    }

    bool operate(const boost::shared_ptr<transactor_t>& txor, btree_value *old_value, large_buf_lock_t& old_large_buflock, btree_value **new_value, large_buf_lock_t& new_large_buflock) {
        try {

            /* We may be instructed to abort depending on the old value */
            if (old_value) {
                switch (replace_policy) {
                    case replace_policy_yes:
                        break;
                    case replace_policy_no:
                        result = sr_didnt_replace;
                        return false;
                    case replace_policy_if_cas_matches:
                        if (!old_value->has_cas() || old_value->cas() != req_cas) {
                            result = sr_didnt_replace;
                            return false;
                        }
                        break;
                    default: unreachable();
                }
            } else {
                switch (add_policy) {
                    case add_policy_yes:
                        break;
                    case add_policy_no:
                        result = sr_didnt_add;
                        return false;
                    default: unreachable();
                }
            }

            if (data->get_size() > MAX_VALUE_SIZE) {
                result = sr_too_large;
                /* To be standards-compliant we must delete the old value when an effort is made to
                replace it with a value that is too large. */
                *new_value = NULL;
                return true;
            }

            value.value_size(0, slice->cache().get_block_size());
            if (old_value && old_value->has_cas()) {
                // Turns the flag on and makes
                // room. run_btree_modify_oper() will set an actual CAS
                // later. TODO: We should probably have a separate
                // function for this.
                metadata_write(&value.metadata_flags, value.contents, mcflags, exptime, 0xCA5ADDED);
            } else {
                metadata_write(&value.metadata_flags, value.contents, mcflags, exptime);
            }

            value.value_size(data->get_size(), slice->cache().get_block_size());

            large_buf_lock_t large_buflock;
            buffer_group_t buffer_group;

            rassert(data->get_size() <= MAX_VALUE_SIZE);
            if (data->get_size() <= MAX_IN_NODE_VALUE_SIZE) {
                buffer_group.add_buffer(data->get_size(), value.value());
                data->get_data_into_buffers(&buffer_group);
            } else {
                large_buflock.set(new large_buf_t(txor->transaction(), value.lb_ref(), btree_value::lbref_limit, rwi_write));
                large_buflock->allocate(data->get_size());

                large_buflock->bufs_at(0, data->get_size(), false, &buffer_group);

                try {
                    data->get_data_into_buffers(&buffer_group);
                } catch (...) {
                    large_buflock->mark_deleted();
                    throw;
                }
            }

            result = sr_stored;
            *new_value = &value;
            new_large_buflock.swap(large_buflock);
            return true;

        } catch (data_provider_failed_exc_t) {
            result = sr_data_provider_failed;
            return false;
        }
    }

    virtual void actually_acquire_large_value(large_buf_t *lb) {
        co_acquire_large_value_for_delete(lb);
    }

    ticks_t start_time;

    data_provider_t *data;
    mcflags_t mcflags;
    exptime_t exptime;
    add_policy_t add_policy;
    replace_policy_t replace_policy;
    cas_t req_cas;

    union {
        char value_memory[MAX_BTREE_VALUE_SIZE];
        btree_value value;
    };

    set_result_t result;
};

set_result_t btree_set(const store_key_t &key, btree_slice_t *slice,
        data_provider_t *data, mcflags_t mcflags, exptime_t exptime,
        add_policy_t add_policy, replace_policy_t replace_policy, cas_t req_cas,
        castime_t castime) {
    btree_set_oper_t oper(data, mcflags, exptime, add_policy, replace_policy, req_cas);
    run_btree_modify_oper(&oper, slice, key, castime);
    return oper.result;
}
