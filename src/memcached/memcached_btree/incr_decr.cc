// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "memcached/memcached_btree/incr_decr.hpp"

#include <inttypes.h>

#include "buffer_cache/alt/alt.hpp"
#include "buffer_cache/alt/blob.hpp"
#include "containers/buffer_group.hpp"
#include "containers/printf_buffer.hpp"
#include "containers/scoped.hpp"
#include "memcached/memcached_btree/modify_oper.hpp"
#include "repli_timestamp.hpp"

struct memcached_incr_decr_oper_t : public memcached_modify_oper_t {

    memcached_incr_decr_oper_t(bool _increment, uint64_t _delta)
        : increment(_increment), delta(_delta)
    { }

    bool operate(buf_parent_t leaf,
                 scoped_malloc_t<memcached_value_t> *value) {
        // If the key didn't exist before, we fail.
        if (!value->has()) {
            result.res = incr_decr_result_t::idr_not_found;
            return false;
        }

        // If we can't parse the value as a number, we fail.
        bool valid;
        uint64_t number;

        blob_t b(leaf.cache()->get_block_size(),
                      (*value)->value_ref(), blob::btree_maxreflen);
        rassert(50 <= blob::btree_maxreflen);
        if (b.valuesize() < 50) {
            buffer_group_t buffergroup;
            blob_acq_t acqs;
            b.expose_region(leaf, access_t::read,
                            0, b.valuesize(), &buffergroup, &acqs);
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
        b.clear(leaf);
        b.append_region(leaf, tmp.size());
        buffer_group_t group;
        blob_acq_t acqs;
        b.expose_region(leaf, access_t::write,
                        0, b.valuesize(), &group, &acqs);
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

incr_decr_result_t
memcached_incr_decr(const store_key_t &key, btree_slice_t *slice, bool increment,
                    uint64_t delta, cas_t proposed_cas, exptime_t effective_time,
                    repli_timestamp_t timestamp, superblock_t *superblock) {
    memcached_incr_decr_oper_t oper(increment, delta);
    run_memcached_modify_oper(&oper, slice, key, proposed_cas, effective_time,
                              timestamp, superblock);
    return oper.result;
}

