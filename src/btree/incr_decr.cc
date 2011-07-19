#include "btree/incr_decr.hpp"

#include "btree/modify_oper.hpp"
#include "containers/buffer_group.hpp"
#include "buffer_cache/buf_lock.hpp"
#include "containers/scoped_malloc.hpp"


struct btree_incr_decr_oper_t : public btree_modify_oper_t {

    explicit btree_incr_decr_oper_t(bool increment, uint64_t delta)
        : increment(increment), delta(delta)
    { }

    bool operate(transaction_t *txn, scoped_malloc<memcached_value_t>& value) {
        // If the key didn't exist before, we fail.
        if (!value) {
            result.res = incr_decr_result_t::idr_not_found;
            return false;
        }

        // If we can't parse the value as a number, we fail.
        bool valid;
        uint64_t number;

        blob_t b(value->value_ref(), blob::btree_maxreflen);
        rassert(50 <= blob::btree_maxreflen);
        if (b.valuesize() < 50) {
            buffer_group_t buffergroup;
            blob_acq_t acqs;
            b.expose_region(txn, rwi_read, 0, b.valuesize(), &buffergroup, &acqs);
            rassert(buffergroup.num_buffers() == 1);

            char buffer[50];
            memcpy(buffer, buffergroup.get_buffer(0).data, buffergroup.get_buffer(0).size);
            buffer[buffergroup.get_buffer(0).size] = '\0';
            char *endptr;
            number = strtoull_strict(buffer, &endptr, 10);
            valid = (endptr != buffer);
        } else {
            valid = false;
        }

        if (!valid) {
            result.res = incr_decr_result_t::idr_not_numeric;
            return false;
        }

        // If we overflow when doing an increment, set number to 0
        // (this is as memcached does it as of version 1.4.5).  For
        // decrements, set to 0 on underflows.
        if (increment) {
            if (number + delta < number) {
                number = 0;
            } else {
                number = number + delta;
            }
        } else {
            if (number - delta > number) {
                number = 0;
            } else {
                number = number - delta;
            }
        }

        result.res = incr_decr_result_t::idr_success;
        result.new_value = number;

        char tmp[50];
        int chars_written = sprintf(tmp, "%llu", (long long unsigned)number);
        rassert(chars_written <= 49);
        b.unappend_region(txn, b.valuesize());
        b.append_region(txn, chars_written);
        rassert(b.valuesize() == chars_written, "expecting %ld == %d", b.valuesize(), chars_written);
        buffer_group_t group;
        blob_acq_t acqs;
        b.expose_region(txn, rwi_write, 0, b.valuesize(), &group, &acqs);
        rassert(group.num_buffers() == 1);
        rassert(group.get_buffer(0).size == chars_written, "expecting %zd == %d", group.get_buffer(0).size, chars_written);
        memcpy(group.get_buffer(0).data, tmp, chars_written);

        return true;
    }

    int compute_expected_change_count(UNUSED block_size_t block_size) {
        return 1;
    }

    bool increment;   // If false, then decrement
    uint64_t delta;   // Amount to increment or decrement by

    incr_decr_result_t result;
};

incr_decr_result_t btree_incr_decr(const store_key_t &key, btree_slice_t *slice, bool increment, uint64_t delta, castime_t castime, order_token_t token) {
    btree_incr_decr_oper_t oper(increment, delta);
    memcached_value_sizer_t sizer(slice->cache()->get_block_size());
    run_btree_modify_oper(&sizer, &oper, slice, key, castime, token);
    return oper.result;
}
