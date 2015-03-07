// Copyright 2010-2015 RethinkDB, all rights reserved
#include "clustering/table_manager/sindex_manager.hpp"

#include "concurrency/cross_thread_signal.hpp"
#include "rdb_protocol/store.hpp"

sindex_manager_t::sindex_manager_t(
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

void sindex_manager_t::update_blocking(signal_t *interruptor) {
    std::map<std::string, sindex_config_t> goal;
    table_config->apply_read([&](const table_config_t *config) {
        goal = config->sindexes;
    });

    for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
        store_t *store = multistore->get_underlying_store(i);
        cross_thread_signal_t ct_interruptor(interruptor, store->home_thread());
        on_thread_t thread_switcher(store->home_thread());

        std::map<std::string, sindex_config_t> current;
        for (const auto &pair : store->sindex_list(&ct_interruptor)) {
            current.insert(std::make_pair(pair.first, pair.second.first));
        }

        /* First we'll compute what changes to make to sindexes; then we'll make the
        changes. The reason to do this in two steps is that we compute the changes in a
        different order than we want to perform them. */
        std::set<std::string> to_drop;
        std::map<std::string, std::string> to_rename;
        std::map<std::string, sindex_config_t> to_create;

        /* If an existing index is missing or has a different name in `goal`, then add it
        to `to_drop`. We might remove it later, if we realize it was actually renamed
        instead of dropped. */
        for (const auto &cur_pair : current) {
            auto goal_it = goal.find(cur_pair.first);
            if (goal_it == goal.end() || goal_it->second != cur_pair.second) {
                to_drop.insert(cur_pair.first);
            }
        }

        /* Go through the indexes in `goal`, and add entries to `to_create`, Or, if there
        is an existing index with that definition that would otherwise be dropped, then
        rename the existing index instead of creating a new one. */
        for (const auto &goal_pair : goal) {
            auto cur_it = current.find(goal_pair.first);
            if (cur_it != current.end() && cur_it->second == goal_pair.second) {
                /* OK, we already have this sindex */
                continue;
            }
            /* See if there's a sindex in `to_drop` with the desired definition. If so,
            we'll rename it instead of dropping and recreating. */
            bool renamed = false;
            for (const std::string &other : to_drop) {
                if (current.at(other) == goal_pair.second) {
                    /* OK, we'll rename the index instead of dropping it */
                    to_rename.insert(std::make_pair(other, goal_pair.first));
                    to_drop.erase(other);
                    renamed = true;
                    break;
                }
            }
            /* If we weren't able to find a sindex to rename, we'll have to create a new
            one */
            if (!renamed) {
                to_create.insert(goal_pair);
            }
        }

        /* OK, time to actually execute the changes */
        for (const std::string &index : to_drop) {
            store->sindex_drop(index, &ct_interruptor);
        }
        store->sindex_rename_multi(to_rename, &ct_interruptor);
        for (const auto &pair : to_create) {
            store->sindex_create(pair.first, pair.second, &ct_interruptor);
        }
    }
}

