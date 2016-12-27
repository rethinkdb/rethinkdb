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
        read_ahead = true;
        // This is probably too low, thanks to status quo bias (the status quo having
        // been to never compute checksums).
        checksum_threshold = 65536;
    }

    /* Enable reading more data than requested to let the cache warmup more quickly
       esp. on rotational drives */
    bool read_ahead;
    /* The threshold above which we don't checksum blocks -- instead we fdatasync before
       writing the serializer superblock.  Designed to make single-document writes
       fast. */
    uint32_t checksum_threshold;
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
    max_block_size_t max_block_size() const {
        return max_block_size_t::unsafe_make(block_size_);
    }
    uint64_t extent_size() const { return extent_size_; }
};

/* Configuration for the serializer that is set when the database is created */
struct log_serializer_static_config_t : public log_serializer_on_disk_static_config_t {
    log_serializer_static_config_t() {
        extent_size_ = DEFAULT_EXTENT_SIZE;
        block_size_ = DEFAULT_BTREE_BLOCK_SIZE;
    }
};

RDB_MAKE_SERIALIZABLE_2(log_serializer_static_config_t,
                        block_size_, extent_size_);

#endif /* SERIALIZER_LOG_CONFIG_HPP_ */

