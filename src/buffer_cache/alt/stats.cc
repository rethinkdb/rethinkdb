// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "buffer_cache/alt/stats.hpp"

#include "perfmon/perfmon.hpp"

alt_cache_stats_t::alt_cache_stats_t(perfmon_collection_t *parent)
    : cache_collection(),
      cache_membership(parent, &cache_collection, "cache"),
      cache_collection_membership(&cache_collection,
                                  NULLPTR) { }

