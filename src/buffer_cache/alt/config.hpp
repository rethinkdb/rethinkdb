// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef BUFFER_CACHE_ALT_CONFIG_HPP_
#define BUFFER_CACHE_ALT_CONFIG_HPP_

#include "config/args.hpp"
#include "containers/archive/archive.hpp"
#include "rpc/serialize_macros.hpp"

// KSI: Maybe this config struct can just go away completely.  For now we have it to
// conform to some aspects of the interface of the mirrored cache, putting off until
// later whether certain configuration options may be removed.

class alt_cache_balancer_t;

class page_cache_config_t {
public:
    page_cache_config_t()
        : io_priority_reads(CACHE_READS_IO_PRIORITY),
          io_priority_writes(CACHE_WRITES_IO_PRIORITY),
          memory_limit(GIGABYTE),
          balancer(NULL) { }

    int32_t io_priority_reads;
    int32_t io_priority_writes;
    uint64_t memory_limit;
    alt_cache_balancer_t *balancer;

};

class alt_cache_config_t {
public:
    page_cache_config_t page_config;
};

#endif  // BUFFER_CACHE_ALT_CONFIG_HPP_
