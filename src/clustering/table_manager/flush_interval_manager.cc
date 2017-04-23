// Copyright 2010-2015 RethinkDB, all rights reserved
#include "clustering/table_manager/flush_interval_manager.hpp"

#include "clustering/administration/issues/outdated_index.hpp"

#include "concurrency/cross_thread_signal.hpp"
#include "concurrency/pmap.hpp"
#include "rdb_protocol/store.hpp"

flush_interval_manager_t::flush_interval_manager_t(
        multistore_ptr_t *multistore_,
        const clone_ptr_t<watchable_t<table_config_t> > &table_config_) :
    multistore(multistore_), table_config(table_config_),
    update_pumper([this](signal_t *interruptor) { update_blocking(interruptor); }),
    table_config_subs([this]() { update_pumper.notify(); })
{
    watchable_t<table_config_t>::freeze_t freeze(table_config);
    table_config_subs.reset(table_config, &freeze);
    update_pumper.notify();
}

void flush_interval_manager_t::update_blocking(signal_t *interruptor) {
    flush_interval_t flush_interval;
    table_config->apply_read([&](const table_config_t *config) {
        // HSI: Oh definitely read the value out of the config, thank you.
        flush_interval = get_flush_interval(*config);
    });

    for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
        store_t *store = multistore->get_underlying_store(i);
        cross_thread_signal_t ct_interruptor(interruptor, store->home_thread());
        on_thread_t thread_switcher(store->home_thread());

        store->configure_flush_interval(flush_interval);
    }
}

