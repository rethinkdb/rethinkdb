// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_MANAGER_FLUSH_INTERVAL_MANAGER_HPP_
#define CLUSTERING_TABLE_MANAGER_FLUSH_INTERVAL_MANAGER_HPP_

#include "clustering/table_contract/cpu_sharding.hpp"
#include "clustering/administration/tables/table_metadata.hpp"
#include "concurrency/pump_coro.hpp"
#include "concurrency/watchable.hpp"

/* The `flush_interval_manager_t` is responsible for reading the flush interval
description from the `table_config_t` updating the flush interval on the `store_t`. */

class flush_interval_manager_t {
public:
    flush_interval_manager_t(
        multistore_ptr_t *multistore,
        const clone_ptr_t<watchable_t<table_config_t> > &table_config);

private:
    void update_blocking(signal_t *interruptor);

    multistore_ptr_t *const multistore;
    clone_ptr_t<watchable_t<table_config_t> > const table_config;

    /* Destructor order matters: The `table_config_subs` must be destroyed before the
    `update_pumper` because it calls `update_pumper.notify()`. But `update_pumper` must
    be destroyed before the other variables because it runs `update_blocking()`, which
    accesses the other variables. */
    pump_coro_t update_pumper;

    watchable_t<table_config_t>::subscription_t table_config_subs;
};

#endif /* CLUSTERING_TABLE_MANAGER_FLUSH_INTERVAL_MANAGER_HPP_ */

