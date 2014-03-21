// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "memcached/memcached_btree/set.hpp"

#include "buffer_cache/alt/alt.hpp"
#include "buffer_cache/alt/blob.hpp"
#include "containers/buffer_group.hpp"
#include "containers/data_buffer.hpp"
#include "math.hpp"
#include "memcached/memcached_btree/modify_oper.hpp"
#include "time.hpp"

struct memcached_set_oper_t : public memcached_modify_oper_t {
    memcached_set_oper_t(const counted_t<data_buffer_t>& _data,
                         mcflags_t _mcflags,
                         exptime_t _exptime,
                         add_policy_t ap,
                         replace_policy_t rp,
                         cas_t _req_cas)
        : data(_data), mcflags(_mcflags), exptime(_exptime),
          add_policy(ap), replace_policy(rp), req_cas(_req_cas) { }

    ~memcached_set_oper_t() { }

    bool operate(buf_parent_t leaf,
                 scoped_malloc_t<memcached_value_t> *value) {
        const block_size_t block_size = leaf.cache()->get_block_size();
        // We may be instructed to abort, depending on the old value.
        if (value->has()) {
            switch (replace_policy) {
            case replace_policy_yes:
                break;
            case replace_policy_no:
                result = sr_didnt_replace;
                return false;
            case replace_policy_if_cas_matches:
                if (!(*value)->has_cas() || (*value)->cas() != req_cas) {
                    result = sr_didnt_replace;
                    return false;
                }
                break;
            default:
                unreachable();
            }
        } else {
            switch (add_policy) {
            case add_policy_yes:
                break;
            case add_policy_no:
                result = sr_didnt_add;
                return false;
            default:
                unreachable();
            }
        }

        if (!value->has()) {
            *value = scoped_malloc_t<memcached_value_t>(MAX_MEMCACHED_VALUE_SIZE);
            memset(value->get(), 0, MAX_MEMCACHED_VALUE_SIZE);
        }

        // Whatever the case, shrink the old value.
        {
            blob_t b(block_size,
                          (*value)->value_ref(),
                          blob::btree_maxreflen);
            b.clear(leaf);
        }

        if (data->size() > MAX_VALUE_SIZE) {
            result = sr_too_large;
            value->reset();
            return true;
        }

        {
            scoped_malloc_t<memcached_value_t> tmp(MAX_MEMCACHED_VALUE_SIZE);
            if ((*value)->has_cas()) {
                // run_memcached_modify_oper will set an actual CAS later.
                metadata_write(&tmp->metadata_flags, tmp->contents, mcflags, exptime, 0xCA5ADDED);
            } else {
                metadata_write(&tmp->metadata_flags, tmp->contents, mcflags, exptime);
            }
            memcpy(tmp->value_ref(), (*value)->value_ref(),
                   blob::ref_size(block_size, (*value)->value_ref(),
                                       blob::btree_maxreflen));
            *value = std::move(tmp);
        }

        blob_t b(block_size, (*value)->value_ref(), blob::btree_maxreflen);

        b.append_region(leaf, data->size());
        buffer_group_t bg;
        blob_acq_t acq;
        b.expose_region(leaf, access_t::write,
                        0, data->size(), &bg, &acq);

        buffer_group_copy_data(&bg, data->buf(), data->size());

        result = sr_stored;
        return true;
    }

    virtual int compute_expected_change_count(block_size_t block_size) {
        if (data->size() < MAX_IN_NODE_VALUE_SIZE) {
            return 1;
        } else {
            size_t size = ceil_aligned(data->size(), block_size.value());
            // one for the leaf node plus the number of blocks required to hold the large value
            return 1 + size / block_size.value();
        }
    }

    ticks_t start_time;

    counted_t<data_buffer_t> data;
    mcflags_t mcflags;
    exptime_t exptime;
    add_policy_t add_policy;
    replace_policy_t replace_policy;
    cas_t req_cas;

    set_result_t result;
};

set_result_t memcached_set(const store_key_t &key,
                           btree_slice_t *slice,
                           const counted_t<data_buffer_t>& data,
                           mcflags_t mcflags,
                           exptime_t exptime,
                           add_policy_t add_policy,
                           replace_policy_t replace_policy,
                           cas_t req_cas,
                           cas_t proposed_cas,
                           exptime_t effective_time,
                           repli_timestamp_t timestamp,
                           superblock_t *superblock,
                           promise_t<superblock_t *> *pass_back_superblock) {
    memcached_set_oper_t oper(data, mcflags, exptime, add_policy, replace_policy,
                              req_cas);
    run_memcached_modify_oper(&oper, slice, key, proposed_cas, effective_time,
                              timestamp, superblock, pass_back_superblock);
    return oper.result;
}

