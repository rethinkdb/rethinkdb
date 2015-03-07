// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_MANAGER_SINDEX_MANAGER_HPP_
#define CLUSTERING_TABLE_MANAGER_SINDEX_MANAGER_HPP_

#include "clustering/table_contract/cpu_sharding.hpp"
#include "clustering/administration/tables/table_metadata.hpp"
#include "concurrency/pump_coro.hpp"
#include "concurrency/watchable.hpp"

/* The `sindex_manager_t` is responsible for reading the sindex description from the
`table_config_t` and adding, dropping, and renaming sindexes on the `store_t` to match
the description. */

class sindex_manager_t {
public:
    sindex_manager_t(
        multistore_ptr_t *multistore,
        const clone_ptr_t<watchable_t<table_config_t> > &table_config);

private:
    void update_blocking(signal_t *interruptor);
    multistore_ptr_t *const multistore;
    clone_ptr_t<watchable_t<table_config_t> > const table_config;
    pump_coro_t update_pumper;
    watchable_t<table_config_t>::subscription_t table_config_subs;
};

#endif /* CLUSTERING_TABLE_MANAGER_SINDEX_MANAGER_HPP_ */

