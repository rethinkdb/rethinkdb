// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef SERIALIZER_LOG_CONFIG_HPP_
#define SERIALIZER_LOG_CONFIG_HPP_

#include <string>

#include "config/args.hpp"
#include "containers/archive/archive.hpp"
#include "serializer/types.hpp"
#include "rpc/serialize_macros.hpp"

/* Configuration for the serializer that can change from run to run */

struct log_serializer_dynamic_config_t {
    log_serializer_dynamic_config_t() {
        gc_low_ratio = DEFAULT_GC_LOW_RATIO;
        gc_high_ratio = DEFAULT_GC_HIGH_RATIO;
        read_ahead = true;
        io_batch_factor = DEFAULT_IO_BATCH_FACTOR;
    }

    /* When the proportion of garbage blocks hits gc_high_ratio, then the serializer will collect
    garbage until it reaches gc_low_ratio. */
    double gc_low_ratio, gc_high_ratio;

    /* The (minimal) batch size of i/o requests being taken from a single i/o account.
    It is a factor because the actual batch size is this factor multiplied by the
    i/o priority of the account. */
    int32_t io_batch_factor;

    /* Enable reading more data than requested to let the cache warmup more quickly esp. on rotational drives */
    bool read_ahead;

    RDB_MAKE_ME_SERIALIZABLE_4(gc_low_ratio, gc_high_ratio, io_batch_factor, \
                               read_ahead);
};

/* This is equivalent to log_serializer_static_config_t below, but is an on-disk
structure. Changes to this change the on-disk database format! */
struct log_serializer_on_disk_static_config_t {
    uint64_t block_size_;
    uint64_t extent_size_;

    // Some helpers
    uint64_t blocks_per_extent() const { return extent_size_ / block_size_; }
    uint64_t extent_index(int64_t offset) const { return offset / extent_size_; }

    // Minimize calls to these.
    max_block_size_t max_block_size() const { return max_block_size_t::unsafe_make(block_size_); }
    uint64_t extent_size() const { return extent_size_; }
};

/* Configuration for the serializer that is set when the database is created */
struct log_serializer_static_config_t : public log_serializer_on_disk_static_config_t {
    log_serializer_static_config_t() {
        extent_size_ = DEFAULT_EXTENT_SIZE;
        block_size_ = DEFAULT_BTREE_BLOCK_SIZE;
    }

    RDB_MAKE_ME_SERIALIZABLE_2(block_size_, extent_size_);
};

#endif /* SERIALIZER_LOG_CONFIG_HPP_ */

