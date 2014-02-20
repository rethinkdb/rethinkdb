// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "memcached/memcached_btree/distribution.hpp"

#include "btree/get_distribution.hpp"

distribution_result_t
memcached_distribution_get(int max_depth,
                           const store_key_t &left_key,
                           exptime_t, superblock_t *superblock) {
    int64_t key_count_out;
    std::vector<store_key_t> key_splits;
    get_btree_key_distribution(superblock, max_depth,
                               &key_count_out, &key_splits);

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
