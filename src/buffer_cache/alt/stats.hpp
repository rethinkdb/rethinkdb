// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef BUFFER_CACHE_ALT_STATS_HPP_
#define BUFFER_CACHE_ALT_STATS_HPP_

#include "perfmon/perfmon.hpp"

class alt_cache_stats_t {
public:
    explicit alt_cache_stats_t(perfmon_collection_t *parent);

    perfmon_collection_t cache_collection;
    perfmon_membership_t cache_membership;

    /*
      LSI: insert perfmons here
    */

    perfmon_multi_membership_t cache_collection_membership;
};


#endif  // BUFFER_CACHE_ALT_STATS_HPP_
