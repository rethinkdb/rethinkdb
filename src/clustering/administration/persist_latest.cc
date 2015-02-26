// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/persist_latest.hpp"

metadata_file_t::metadata_file_t(
        io_backender_t *io_backender,
        const serializer_filepath_t &filename,
        perfmon_collection_t *perfmon_parent,
        bool create) {
    filepath_file_opener_t file_opener(filename, io_backender);
    if (create) {
        standard_serializer_t::create(
            &file_opener,
            standard_serializer_t::static_config_t());
    }
    serializer.init(new standard_serializer_t(
        standard_serializer_t::dynamic_config_t(),
        &file_opener,
        perfmon_parent));
    if (!serializer->coop_lock_and_check()) {
        throw file_in_use_exc_t();
    }
    balancer.init(new dummy_cache_balancer_t(MEGABYTE));
    cache.init(new cache_t(serializer.get(), balancer.get(), perfmon_parent));
    cache_conn.init(new cache_conn_t(cache.get()));
    if (create) {
        
    }
}
