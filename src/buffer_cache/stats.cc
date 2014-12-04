// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "buffer_cache/stats.hpp"

#include "perfmon/perfmon.hpp"

alt_cache_stats_t::alt_cache_stats_t(alt::page_cache_t *_page_cache,
                                     perfmon_collection_t *parent) :
    page_cache(_page_cache),
    cache_collection(),
    cache_membership(parent, &cache_collection, "cache"),
    in_use_bytes(this),
    in_use_bytes_membership(&cache_collection,
                            &in_use_bytes, "in_use_bytes"),
    cache_collection_membership(&cache_collection) { }

alt_cache_stats_t::perfmon_value_t::perfmon_value_t(alt_cache_stats_t *_parent) :
    parent(_parent) { }

void *alt_cache_stats_t::perfmon_value_t::begin_stats() {
    return new uint64_t;
}

void alt_cache_stats_t::perfmon_value_t::visit_stats(void *ptr) {
    if (get_thread_id() == parent->home_thread()) {
        uint64_t *value = reinterpret_cast<uint64_t *>(ptr);
        *value = parent->page_cache->evicter().in_memory_size();
    }
}

ql::datum_t alt_cache_stats_t::perfmon_value_t::end_stats(void *ptr) {
    uint64_t *value = reinterpret_cast<uint64_t *>(ptr);
    ql::datum_t res(static_cast<double>(*value));
    delete value;
    return res;
}
