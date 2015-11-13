// Copyright 2010-2015 RethinkDB, all rights reserved
#include "clustering/table_manager/sindex_manager.hpp"

#include "concurrency/cross_thread_signal.hpp"
#include "concurrency/pmap.hpp"
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

std::map<std::string, std::pair<sindex_config_t, sindex_status_t> >
sindex_manager_t::get_status(signal_t *interruptor) const {
    /* First, we make a list of all the sindexes in the config. Then, we iterate over
    the actual sindexes in the stores and try to match them to the sindexes in the
    config. If we find a match, we accumulate the `sindex_status_t`s. If there's a sindex
    in the config with no matching actual sindex, we set `ready = false`. */

    std::map<std::string, std::pair<sindex_config_t, sindex_status_t> > res;
    table_config->apply_read([&](const table_config_t *config) {
        for (const auto &pair : config->sindexes) {
            auto it = res.insert(
                std::make_pair(pair.first, std::make_pair(pair.second,
                                                          sindex_status_t()))).first;
            it->second.second.outdated =
                (pair.second.func_version != reql_version_t::LATEST);
        }
    });

    pmap(static_cast<int64_t>(0), static_cast<int64_t>(CPU_SHARDING_FACTOR),
    [&](int64_t i) {
        std::map<std::string, std::pair<sindex_config_t, sindex_status_t> > store_state;
        store_t *store = multistore->get_underlying_store(i);
        cross_thread_signal_t ct_interruptor(interruptor, store->home_thread());
        {
            on_thread_t thread_switcher(store->home_thread());
            store_state = store->sindex_list(&ct_interruptor);
        }

        for (auto &&pair : res) {
            auto it = store_state.find(pair.first);
            /* Note that we treat an index with the wrong definition like a missing
            index. */
            if (it != store_state.end() && it->second.first == pair.second.first) {
                pair.second.second.accum(it->second.second);
            } else {
                pair.second.second.ready = false;
            }
        }
    });

    return res;
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
            /* Note that this crashes if the index already exists. Luckily we are running
            in a `pump_coro_t`, and we should be the only thing that's creating indexes
            on the store so this shouldn't be an issue. */
            store->sindex_create(pair.first, pair.second, &ct_interruptor);
        }
    }
}

