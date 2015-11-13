// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/persist/table_interface.hpp"

#include <algorithm>
#include <array>

#include "clustering/administration/persist/branch_history_manager.hpp"
#include "clustering/administration/persist/file_keys.hpp"
#include "clustering/administration/persist/raft_storage_interface.hpp"
#include "clustering/administration/perfmon_collection_repo.hpp"
#include "logger.hpp"
#include "rdb_protocol/store.hpp"
#include "serializer/log/log_serializer.hpp"
#include "serializer/merger.hpp"
#include "serializer/translator.hpp"

class real_multistore_ptr_t :
    public multistore_ptr_t {
public:
    real_multistore_ptr_t(
            const namespace_id_t &table_id,
            const serializer_filepath_t &path,
            scoped_ptr_t<real_branch_history_manager_t> &&bhm,
            const base_path_t &base_path,
            io_backender_t *io_backender,
            cache_balancer_t *cache_balancer,
            rdb_context_t *rdb_context,
            perfmon_collection_t *perfmon_collection_serializers,
            scoped_ptr_t<thread_allocation_t> &&serializer_thread,
            std::vector<scoped_ptr_t<thread_allocation_t> > &&store_threads,
            std::map<
                namespace_id_t, std::pair<real_multistore_ptr_t *, auto_drainer_t::lock_t>
            > *real_multistores) :
        branch_history_manager(std::move(bhm)),
        serializer_thread_allocation(std::move(serializer_thread)),
        store_thread_allocations(std::move(store_threads)),
        map_insertion_sentry(
            real_multistores, table_id, std::make_pair(this, drainer.lock()))
    {
        // TODO: If the server gets killed when starting up, we can
        // get a database in an invalid startup state.

        // TODO: This is quite suspicious in that we check if the file
        // exists and then assume it exists or does not exist when
        // loading or creating it.

        // TODO: We should use N slices on M serializers, not N slices
        // on 1 serializer.

        int res = access(path.permanent_path().c_str(), R_OK | W_OK);
        bool create = (res != 0);

        on_thread_t thread_switcher(serializer_thread_allocation->get_thread());
        filepath_file_opener_t file_opener(path, io_backender);

        if (create) {
            log_serializer_t::create(
                &file_opener,
                log_serializer_t::static_config_t());
        }

        // TODO: Could we handle failure when loading the serializer?  Right
        // now, we don't.

        scoped_ptr_t<serializer_t> inner_serializer(new log_serializer_t(
            log_serializer_t::dynamic_config_t(),
            &file_opener,
            perfmon_collection_serializers));
        serializer.init(new merger_serializer_t(
            std::move(inner_serializer),
            MERGER_SERIALIZER_MAX_ACTIVE_WRITES));

        std::vector<serializer_t *> ptrs;
        ptrs.push_back(serializer.get());
        if (create) {
            serializer_multiplexer_t::create(ptrs, CPU_SHARDING_FACTOR);
        }
        multiplexer.init(new serializer_multiplexer_t(ptrs));

        pmap(CPU_SHARDING_FACTOR, [&](int ix) {
            // TODO: Exceptions? If exceptions are being thrown in here, nothing is
            // handling them.

            on_thread_t thread_switcher_2(store_thread_allocations[ix]->get_thread());

            stores[ix].init(new store_t(
                cpu_sharding_subspace(ix),
                multiplexer->proxies[ix],
                cache_balancer,
                strprintf("shard_%d", ix),
                create,
                perfmon_collection_serializers,
                rdb_context,
                io_backender,
                base_path,
                table_id,
                update_sindexes_t::UPDATE));

            /* Initialize the metainfo if necessary */
            if (create) {
                order_source_t order_source;
                cond_t non_interruptor;
                write_token_t write_token;
                stores[ix]->new_write_token(&write_token);
                stores[ix]->set_metainfo(
                    region_map_t<binary_blob_t>(
                        region_t::universe(),
                        binary_blob_t(version_t::zero())),
                    order_source.check_in("real_multistore_ptr_t"),
                    &write_token,
                    write_durability_t::HARD,
                    &non_interruptor);
            }
        });

        if (create) {
            file_opener.move_serializer_file_to_permanent_location();
        }
    }

    ~real_multistore_ptr_t() {
        serializer_thread_allocation.reset();
        store_thread_allocations.clear();
        map_insertion_sentry.reset();
        drainer.drain();
        pmap(CPU_SHARDING_FACTOR, [this](int ix) {
            if (stores[ix].has()) {
                on_thread_t thread_switcher(stores[ix]->home_thread());
                stores[ix].reset();
            }
        });
        if (serializer.has()) {
            on_thread_t thread_switcher(serializer->home_thread());
            if (multiplexer.has()) {
                multiplexer.reset();
            }
            serializer.reset();
        }
    }

    branch_history_manager_t *get_branch_history_manager() {
        return branch_history_manager.get();
    }

    serializer_t *get_serializer() {
        return serializer.get_or_null();
    }

    store_view_t *get_cpu_sharded_store(size_t i) {
        return stores[i].get();
    }

    store_t *get_underlying_store(size_t i) {
        return stores[i].get();
    }

    bool is_gc_active() {
        rassert(!drainer.is_draining());
        if (serializer.has()) {
            return serializer->is_gc_active();
        } else {
            return false;
        }
    }

private:
    scoped_ptr_t<real_branch_history_manager_t> branch_history_manager;
    scoped_ptr_t<serializer_t> serializer;
    scoped_ptr_t<serializer_multiplexer_t> multiplexer;
    scoped_ptr_t<store_t> stores[CPU_SHARDING_FACTOR];

    scoped_ptr_t<thread_allocation_t> serializer_thread_allocation;
    std::vector<scoped_ptr_t<thread_allocation_t> > store_thread_allocations;

    auto_drainer_t drainer;
    map_insertion_sentry_t<
        namespace_id_t, std::pair<real_multistore_ptr_t *, auto_drainer_t::lock_t>
    > map_insertion_sentry;

    DISABLE_COPYING(real_multistore_ptr_t);
};

void real_table_persistence_interface_t::read_all_metadata(
        const std::function<void(
            const namespace_id_t &table_id,
            const table_active_persistent_state_t &state,
            raft_storage_interface_t<table_raft_state_t> *raft_storage,
            metadata_file_t::read_txn_t *metadata_read_txn)> &active_cb,
        const std::function<void(
            const namespace_id_t &table_id,
            const table_inactive_persistent_state_t &state,
            metadata_file_t::read_txn_t *metadata_read_txn)> &inactive_cb,
        signal_t *interruptor) {
    metadata_file_t::read_txn_t read_txn(metadata_file, interruptor);

    std::map<namespace_id_t, table_active_persistent_state_t> active_tables;
    read_txn.read_many<table_active_persistent_state_t>(
        mdprefix_table_active(),
        [&](const std::string &uuid_str, const table_active_persistent_state_t &state) {
            active_tables[str_to_uuid(uuid_str)] = state;
        },
        interruptor);
    storage_interfaces.clear();
    for (const auto &pair : active_tables) {
        storage_interfaces[pair.first].init(new table_raft_storage_interface_t(
            metadata_file, &read_txn, pair.first, interruptor));
        active_cb(
            pair.first, pair.second, storage_interfaces[pair.first].get(), &read_txn);
    }

    read_txn.read_many<table_inactive_persistent_state_t>(
        mdprefix_table_inactive(),
        [&](const std::string &uuid_str, const table_inactive_persistent_state_t &s) {
            inactive_cb(str_to_uuid(uuid_str), s, &read_txn);
        },
        interruptor);
}

void real_table_persistence_interface_t::write_metadata_active(
        const namespace_id_t &table_id,
        const table_active_persistent_state_t &state,
        const raft_persistent_state_t<table_raft_state_t> &raft_state,
        raft_storage_interface_t<table_raft_state_t> **raft_storage_out) {
    cond_t non_interruptor;
    storage_interfaces.erase(table_id);
    metadata_file_t::write_txn_t write_txn(metadata_file, &non_interruptor);
    write_txn.erase(
        mdprefix_table_inactive().suffix(uuid_to_str(table_id)),
        &non_interruptor);
    table_raft_storage_interface_t::erase(&write_txn, table_id);
    write_txn.write(
        mdprefix_table_active().suffix(uuid_to_str(table_id)),
        state,
        &non_interruptor);
    storage_interfaces[table_id].init(new table_raft_storage_interface_t(
        metadata_file, &write_txn, table_id, raft_state));
    *raft_storage_out = storage_interfaces[table_id].get();
}

void real_table_persistence_interface_t::write_metadata_inactive(
        const namespace_id_t &table_id,
        const table_inactive_persistent_state_t &state) {
    cond_t non_interruptor;
    storage_interfaces.erase(table_id);
    metadata_file_t::write_txn_t write_txn(metadata_file, &non_interruptor);
    write_txn.erase(
        mdprefix_table_active().suffix(uuid_to_str(table_id)),
        &non_interruptor);
    write_txn.write(
        mdprefix_table_inactive().suffix(uuid_to_str(table_id)),
        state,
        &non_interruptor);
    table_raft_storage_interface_t::erase(&write_txn, table_id);
    real_branch_history_manager_t::erase(&write_txn, table_id);
}

void real_table_persistence_interface_t::delete_metadata(
        const namespace_id_t &table_id) {
    cond_t non_interruptor;
    storage_interfaces.erase(table_id);
    metadata_file_t::write_txn_t write_txn(metadata_file, &non_interruptor);
    write_txn.erase(
        mdprefix_table_active().suffix(uuid_to_str(table_id)),
        &non_interruptor);
    write_txn.erase(
        mdprefix_table_inactive().suffix(uuid_to_str(table_id)),
        &non_interruptor);
    table_raft_storage_interface_t::erase(&write_txn, table_id);
    real_branch_history_manager_t::erase(&write_txn, table_id);
}

void real_table_persistence_interface_t::load_multistore(
        const namespace_id_t &table_id,
        metadata_file_t::read_txn_t *metadata_read_txn,
        scoped_ptr_t<multistore_ptr_t> *multistore_ptr_out,
        signal_t *interruptor,
        perfmon_collection_t *perfmon_collection_serializers) {
    scoped_ptr_t<real_branch_history_manager_t> bhm(
        new real_branch_history_manager_t(
            table_id, metadata_file, metadata_read_txn, interruptor));

    scoped_ptr_t<thread_allocation_t> serializer_thread(
        new thread_allocation_t(&thread_allocator));
    std::vector<scoped_ptr_t<thread_allocation_t> > store_threads;
    for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
        store_threads.emplace_back(new thread_allocation_t(&thread_allocator));
    }

    multistore_ptr_out->init(new real_multistore_ptr_t(
        table_id,
        file_name_for(table_id),
        std::move(bhm),
        base_path,
        io_backender,
        cache_balancer,
        rdb_context,
        perfmon_collection_serializers,
        std::move(serializer_thread),
        std::move(store_threads),
        &real_multistores));
}

void real_table_persistence_interface_t::create_multistore(
        const namespace_id_t &table_id,
        scoped_ptr_t<multistore_ptr_t> *multistore_ptr_out,
        signal_t *interruptor,
        perfmon_collection_t *perfmon_collection_serializers) {
    metadata_file_t::read_txn_t read_txn(metadata_file, interruptor);
    load_multistore(
        table_id, &read_txn, multistore_ptr_out, interruptor,
        perfmon_collection_serializers);
}

void real_table_persistence_interface_t::destroy_multistore(
        const namespace_id_t &table_id,
        scoped_ptr_t<multistore_ptr_t> *multistore_ptr_in) {
    guarantee(multistore_ptr_in->has());
    multistore_ptr_in->reset();

    std::string filepath = file_name_for(table_id).permanent_path();
    logNTC("Removing file %s\n", filepath.c_str());
    const int res = ::unlink(filepath.c_str());
    guarantee_err(res == 0 || get_errno() == ENOENT,
                  "unlink failed for file %s", filepath.c_str());
}

serializer_filepath_t real_table_persistence_interface_t::file_name_for(
        const namespace_id_t &table_id) {
    return serializer_filepath_t(base_path, uuid_to_str(table_id));
}

bool real_table_persistence_interface_t::is_gc_active() const {
    for (int thread = 0; thread < get_num_db_threads(); ++thread) {
        std::map<serializer_t *, auto_drainer_t::lock_t> serializers_copy;

        // Note the copy in the loop below is intentional, one of the members is a
        // `auto_drainer_t::lock_t` that we want to hold.
        for (auto real_multistore : real_multistores) {
            serializer_t *serializer =
                real_multistore.second.first->get_serializer();
            if (serializer == nullptr ||
                    serializer->home_thread() != threadnum_t(thread)) {
                continue;
            }
            serializers_copy.insert(
                std::make_pair(serializer, real_multistore.second.second));
        }

        {
            on_thread_t on_thread((threadnum_t(thread)));
            for (auto const &serializer : serializers_copy) {
                if (serializer.first->is_gc_active()) {
                    return true;
                }
            }
        }
    }

    return false;
}
