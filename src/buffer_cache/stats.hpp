// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef BUFFER_CACHE_STATS_HPP_
#define BUFFER_CACHE_STATS_HPP_

#include "perfmon/perfmon.hpp"
#include "buffer_cache/page_cache.hpp"

class alt_cache_stats_t : public home_thread_mixin_t {
public:
    explicit alt_cache_stats_t(alt::page_cache_t *_page_cache,
                               perfmon_collection_t *parent);

    alt::page_cache_t *page_cache;

    perfmon_collection_t cache_collection;
    perfmon_membership_t cache_membership;

    class perfmon_value_t : public perfmon_t {
    public:
        explicit perfmon_value_t(alt_cache_stats_t *_parent);
        void *begin_stats();
        void visit_stats(void *);
        ql::datum_t end_stats(void *);
    private:
        alt_cache_stats_t *parent;
        DISABLE_COPYING(perfmon_value_t);
    };
    perfmon_value_t in_use_bytes;
    perfmon_membership_t in_use_bytes_membership;


    perfmon_multi_membership_t cache_collection_membership;
};


#endif  // BUFFER_CACHE_STATS_HPP_
