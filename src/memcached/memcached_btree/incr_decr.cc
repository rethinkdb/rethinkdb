// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "memcached/memcached_btree/incr_decr.hpp"

#include <inttypes.h>

#if SLICE_ALT
#include "buffer_cache/alt/alt.hpp"
#include "buffer_cache/alt/alt_blob.hpp"
#endif
#include "containers/buffer_group.hpp"
#include "containers/printf_buffer.hpp"
#include "containers/scoped.hpp"
#include "memcached/memcached_btree/modify_oper.hpp"
#include "repli_timestamp.hpp"

struct memcached_incr_decr_oper_t : public memcached_modify_oper_t {

    memcached_incr_decr_oper_t(bool _increment, uint64_t _delta)
        : increment(_increment), delta(_delta)
    { }

#if SLICE_ALT
    bool operate(alt::alt_buf_parent_t leaf,
                 scoped_malloc_t<memcached_value_t> *value) {
#else
    bool operate(transaction_t *txn, scoped_malloc_t<memcached_value_t> *value) {
#endif
        // If the key didn't exist before, we fail.
        if (!value->has()) {
            result.res = incr_decr_result_t::idr_not_found;
            return false;
        }

        // If we can't parse the value as a number, we fail.
        bool valid;
        uint64_t number;

#if SLICE_ALT
        alt::blob_t b(leaf.cache()->get_block_size(),
                      (*value)->value_ref(), alt::blob::btree_maxreflen);
#else
        blob_t b(txn->get_cache()->get_block_size(),
                 (*value)->value_ref(), blob::btree_maxreflen);
#endif
        rassert(50 <= blob::btree_maxreflen);
        if (b.valuesize() < 50) {
            buffer_group_t buffergroup;
#if SLICE_ALT
            alt::blob_acq_t acqs;
            b.expose_region(leaf, alt::alt_access_t::read,
                            0, b.valuesize(), &buffergroup, &acqs);
#else
            blob_acq_t acqs;
            b.expose_region(txn, rwi_read, 0, b.valuesize(), &buffergroup, &acqs);
#endif
            rassert(buffergroup.num_buffers() == 1);

            char buffer[50];
            memcpy(buffer, buffergroup.get_buffer(0).data, buffergroup.get_buffer(0).size);
            buffer[buffergroup.get_buffer(0).size] = '\0';
            const char *endptr;
            number = strtou64_strict(buffer, &endptr, 10);
            valid = (endptr != buffer);
        } else {
            valid = false;
            number = 0;  // Appease -Wconditional-uninitialized.
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

        printf_buffer_t tmp("%" PRIu64, number);
#if SLICE_ALT
        b.clear(leaf);
        b.append_region(leaf, tmp.size());
#else
        b.clear(txn);
        b.append_region(txn, tmp.size());
#endif
        buffer_group_t group;
#if SLICE_ALT
        alt::blob_acq_t acqs;
        b.expose_region(leaf, alt::alt_access_t::write,
                        0, b.valuesize(), &group, &acqs);
#else
        blob_acq_t acqs;
        b.expose_region(txn, rwi_write, 0, b.valuesize(), &group, &acqs);
#endif
        rassert(group.num_buffers() == 1);
        rassert(group.get_buffer(0).size == tmp.size(), "expecting %zd == %d", group.get_buffer(0).size, tmp.size());
        memcpy(group.get_buffer(0).data, tmp.data(), tmp.size());

        return true;
    }

    int compute_expected_change_count(UNUSED block_size_t block_size) {
        return 1;
    }

    bool increment;   // If false, then decrement
    uint64_t delta;   // Amount to increment or decrement by

    incr_decr_result_t result;
};

#if SLICE_ALT
incr_decr_result_t
memcached_incr_decr(const store_key_t &key, btree_slice_t *slice, bool increment,
                    uint64_t delta, cas_t proposed_cas, exptime_t effective_time,
                    repli_timestamp_t timestamp, superblock_t *superblock) {
#else
incr_decr_result_t memcached_incr_decr(const store_key_t &key, btree_slice_t *slice, bool increment, uint64_t delta, cas_t proposed_cas, exptime_t effective_time, repli_timestamp_t timestamp, transaction_t *txn, superblock_t *superblock) {
#endif
    memcached_incr_decr_oper_t oper(increment, delta);
#if SLICE_ALT
    run_memcached_modify_oper(&oper, slice, key, proposed_cas, effective_time,
                              timestamp, superblock);
#else
    run_memcached_modify_oper(&oper, slice, key, proposed_cas, effective_time, timestamp, txn, superblock);
#endif
    return oper.result;
}

