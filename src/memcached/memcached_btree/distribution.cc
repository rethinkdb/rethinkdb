// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "memcached/memcached_btree/distribution.hpp"
#include "btree/get_distribution.hpp"

#if SLICE_ALT
distribution_result_t
memcached_distribution_get(btree_slice_t *slice, int max_depth,
                           const store_key_t &left_key,
                           exptime_t, superblock_t *superblock) {
#else
distribution_result_t memcached_distribution_get(btree_slice_t *slice, int max_depth, const store_key_t &left_key,
        exptime_t, transaction_t *txn, superblock_t *superblock) {
#endif
    int64_t key_count_out;
    std::vector<store_key_t> key_splits;
#if SLICE_ALT
    get_btree_key_distribution(slice, superblock, max_depth,
                               &key_count_out, &key_splits);
#else
    get_btree_key_distribution(slice, txn, superblock, max_depth, &key_count_out, &key_splits);
#endif

    distribution_result_t res;

    int64_t keys_per_bucket;
    if (key_splits.empty()) {
        keys_per_bucket = key_count_out;
    } else  {
        keys_per_bucket = std::max<int64_t>(key_count_out / key_splits.size(), 1);
    }
    res.key_counts[left_key] = keys_per_bucket;

    for (std::vector<store_key_t>::iterator it  = key_splits.begin();
                                            it != key_splits.end();
                                            ++it) {
        res.key_counts[*it] = keys_per_bucket;
    }

    return res;
}
