#include "btree/append_prepend.hpp"
#include "btree/modify_oper.hpp"
#include "buffer_cache/co_functions.hpp"

struct btree_append_prepend_oper_t : public btree_modify_oper_t {

    btree_append_prepend_oper_t(data_provider_t *data_, bool append_)
        : data(data_), append(append_)
    { }

    bool operate(const boost::shared_ptr<transactor_t>& txor, btree_value *old_value, boost::scoped_ptr<large_buf_t>& old_large_buflock, btree_value **new_value, boost::scoped_ptr<large_buf_t>& new_large_buflock) {
        try {
            if (!old_value) {
                result = apr_not_found;
                return false;
            }

            size_t new_size = old_value->value_size() + data->get_size();
            if (new_size > MAX_VALUE_SIZE) {
                result = apr_too_large;
                return false;
            }

            // Copy flags, exptime, etc.

            valuecpy(&value, old_value);
            if (!old_value->is_large()) {
                // HACK: we set the value.size in advance preparation when
                // the old value was not large.  If the old value _IS_
                // large, we leave it with the old size, and then we
                // adjust value.size by refsize_adjustment below.

                // The vaule_size setter only behaves correctly when we're
                // setting a new large value.
                value.value_size(new_size, slice->cache().get_block_size());
            }

            buffer_group_t buffer_group;
            bool is_old_large_value = false;

            // Figure out where the data is going to need to go and prepare a place for it

            if (new_size <= MAX_IN_NODE_VALUE_SIZE) { // small -> small
                rassert(!old_value->is_large());
                // XXX This does unnecessary copying now.
                if (append) {
                    buffer_group.add_buffer(data->get_size(), value.value() + old_value->value_size());
                } else { // prepend
                    memmove(value.value() + data->get_size(), value.value(), old_value->value_size());
                    buffer_group.add_buffer(data->get_size(), value.value());
                }
            } else {
                // Prepare the large value if necessary.
                if (!old_value->is_large()) { // small -> large; allocate a new large value and copy existing value into it.
                    large_buflock.reset(new large_buf_t(txor->transaction(), value.lb_ref(), btree_value::lbref_limit, rwi_write));
                    large_buflock->allocate(new_size);
                    if (append) large_buflock->fill_at(0, old_value->value(), old_value->value_size());
                    else        large_buflock->fill_at(data->get_size(), old_value->value(), old_value->value_size());
                    is_old_large_value = false;
                } else { // large -> large; expand existing large value
                    memcpy(&value, old_value, MAX_BTREE_VALUE_SIZE);
                    large_buflock.swap(old_large_buflock);
                    large_buflock->HACK_root_ref(value.lb_ref());
                    int refsize_adjustment;

                    if (append) {
                        // TIED large_buflock TO value.
                        large_buflock->append(data->get_size(), &refsize_adjustment);
                    }
                    else {
                        large_buflock->prepend(data->get_size(), &refsize_adjustment);
                    }

                    value.size += refsize_adjustment;
                    is_old_large_value = true;
                }

                // Figure out the pointers and sizes where data needs to go

                uint32_t fill_size = data->get_size();
                uint32_t start_pos = append ? old_value->value_size() : 0;

                large_buflock->bufs_at(start_pos, fill_size, false, &buffer_group);
            }

            // Dispatch the data request

            result = apr_success;
            try {
                data->get_data_into_buffers(&buffer_group);
            } catch (data_provider_failed_exc_t) {
                if (large_buflock) {
                    if (is_old_large_value) {
                        // Some bufs in the large value will have been set dirty (and
                        // so new copies will be rewritten unmodified to disk), but
                        // that's not really a problem because it only happens on
                        // erroneous input.

                        int refsize_adjustment;
                        if (append) {
                            // TIED large_buflock TO value
                            large_buflock->unappend(data->get_size(), &refsize_adjustment);
                        }
                        else {
                            // TIED large_buflock TO value
                            large_buflock->unprepend(data->get_size(), &refsize_adjustment);
                        }
                        value.size += refsize_adjustment;
                    } else {
                        // The old value was small, so we just keep it and delete the large value
                        boost::scoped_ptr<large_buf_t> empty;
                        large_buflock->mark_deleted();
                        large_buflock.swap(empty);
                    }
                }
                throw;
            }

            *new_value = &value;
            new_large_buflock.swap(large_buflock);
            return true;

        } catch (data_provider_failed_exc_t) {
            result = apr_data_provider_failed;
            return false;
        }
    }

    int compute_expected_change_count(const size_t block_size) {
        if (data->get_size() < MAX_IN_NODE_VALUE_SIZE) {
            return 1;
        } else {
            size_t size = ceil_aligned(data->get_size(), block_size);
            // one for the leaf node plus the number of blocks required to hold the large value
            return 1 + size / block_size;
        }
    }

    void actually_acquire_large_value(large_buf_t *lb) {
        if (append) {
            co_acquire_large_buf_rhs(lb);
        } else {
            co_acquire_large_buf_lhs(lb);
        }
    }

    append_prepend_result_t result;

    data_provider_t *data;
    bool append;   // true = append, false = prepend

    union {
        byte value_memory[MAX_BTREE_VALUE_SIZE];
        btree_value value;
    };
    boost::scoped_ptr<large_buf_t> large_buflock;
};

append_prepend_result_t btree_append_prepend(const store_key_t &key, btree_slice_t *slice, data_provider_t *data, bool append, castime_t castime) {
    btree_append_prepend_oper_t oper(data, append);
    run_btree_modify_oper(&oper, slice, key, castime);
    return oper.result;
}
