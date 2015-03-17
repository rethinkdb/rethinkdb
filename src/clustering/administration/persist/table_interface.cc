// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/persist/table_interface.hpp"

#include <array>

#include "clustering/administration/issues/outdated_index.hpp"
#include "clustering/administration/persist/branch_history_manager.hpp"
#include "clustering/administration/persist/file_keys.hpp"
#include "logger.hpp"
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
            outdated_index_issue_tracker_t *outdated_index_issue_tracker,
            perfmon_collection_t *serializers_perfmon_collection,
            threadnum_t serializer_thread,
            const std::vector<threadnum_t> &store_threads) :
        branch_history_manager(std::move(bhm))
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

        on_thread_t thread_switcher(serializer_thread);
        filepath_file_opener_t file_opener(path, io_backender);

        if (create) {
            standard_serializer_t::create(
                &file_opener,
                standard_serializer_t::static_config_t());
        }

        // TODO: Could we handle failure when loading the serializer?  Right
        // now, we don't.

        scoped_ptr_t<serializer_t> inner_serializer(new standard_serializer_t(
            standard_serializer_t::dynamic_config_t(),
            &file_opener,
            serializers_perfmon_collection));
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

            on_thread_t thread_switcher_2(store_threads[ix]);
    
            // Only pass this down to the first store
            scoped_ptr_t<outdated_index_report_t> index_report;
            if (ix == 0) {
                index_report = outdated_index_issue_tracker->create_report(table_id);
            }
    
            stores[ix].init(new store_t(
                multiplexer->proxies[ix],
                cache_balancer,
                strprintf("shard_%d", ix),
                create,
                // TODO: Can we pass serializers_perfmon_collection across threads like
                // this?
                serializers_perfmon_collection,
                rdb_context,
                io_backender,
                base_path,
                std::move(index_report),
                table_id));

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
                    &non_interruptor);
            }
        });

        if (create) {
            file_opener.move_serializer_file_to_permanent_location();
        }
    }

    ~real_multistore_ptr_t() {
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

    store_view_t *get_cpu_sharded_store(size_t i) {
        return stores[i].get();
    }

    store_t *get_underlying_store(size_t i) {
        return stores[i].get();
    }

private:
    scoped_ptr_t<real_branch_history_manager_t> branch_history_manager;
    scoped_ptr_t<serializer_t> serializer;
    scoped_ptr_t<serializer_multiplexer_t> multiplexer;
    scoped_ptr_t<store_t> stores[CPU_SHARDING_FACTOR];
};

void real_table_persistence_interface_t::read_all_tables(
        const std::function<void(
            const namespace_id_t &table_id,
            const table_persistent_state_t &state,
            scoped_ptr_t<multistore_ptr_t> &&multistore_ptr)> &callback,
        signal_t *interruptor) {
    std::map<namespace_id_t, table_persistent_state_t> tables;
    metadata_file_t::read_txn_t read_txn(metadata_file, interruptor);
    read_txn.read_many<table_persistent_state_t>(
        mdprefix_table_persistent_state(),
        [&](const std::string &uuid_str, const table_persistent_state_t &state) {
            namespace_id_t table_id = str_to_uuid(uuid_str);
            tables.insert(std::make_pair(table_id, state));
        },
        interruptor);
    for (const auto &pair : tables) {
        scoped_ptr_t<multistore_ptr_t> multistore;
        make_multistore(pair.first, interruptor, &multistore);
        callback(pair.first, pair.second, std::move(multistore));
    }
}

void real_table_persistence_interface_t::add_table(
        const namespace_id_t &table,
        const table_persistent_state_t &state,
        scoped_ptr_t<multistore_ptr_t> *multistore_ptr_out,
        signal_t *interruptor) {
    /* Record the existence of the table in the metadata file before creating the table's
    data files, so that if we crash, we won't leak any data files */
    {
        metadata_file_t::write_txn_t write_txn(metadata_file, interruptor);
        write_txn.write(
            mdprefix_table_persistent_state().suffix(uuid_to_str(table)),
            state,
            interruptor);
    }
    make_multistore(table, interruptor, multistore_ptr_out);
}

void real_table_persistence_interface_t::update_table(
        const namespace_id_t &table,
        const table_persistent_state_t &state,
        signal_t *interruptor) {
    metadata_file_t::write_txn_t write_txn(metadata_file, interruptor);
    write_txn.write(
        mdprefix_table_persistent_state().suffix(uuid_to_str(table)),
        state,
        interruptor);
}

void real_table_persistence_interface_t::remove_table(
        const namespace_id_t &table,
        signal_t *interruptor) {
    /* Remove the table's data files before removing the record of the table's existence
    from the metadata file, so that if we crash, we won't leak any data files */
    std::string filepath = file_name_for(table).permanent_path();
    logNTC("Removing file %s\n", filepath.c_str());
    const int res = ::unlink(filepath.c_str());
    guarantee_err(res == 0 || get_errno() == ENOENT,
                  "unlink failed for file %s", filepath.c_str());

    /* Also, remove the branch history before removing the main record, so we don't leak
    the branch history */
    real_branch_history_manager_t::erase(table, metadata_file, interruptor);

    metadata_file_t::write_txn_t write_txn(metadata_file, interruptor);
    write_txn.erase(
        mdprefix_table_persistent_state().suffix(uuid_to_str(table)),
        interruptor);
}

serializer_filepath_t real_table_persistence_interface_t::file_name_for(
        const namespace_id_t &table_id) {
    return serializer_filepath_t(base_path, uuid_to_str(table_id));
}

threadnum_t real_table_persistence_interface_t::pick_thread() {
    thread_counter = (thread_counter + 1) % get_num_db_threads();
    return threadnum_t(thread_counter);
}

void real_table_persistence_interface_t::make_multistore(
        const namespace_id_t &table_id,
        signal_t *interruptor,
        scoped_ptr_t<multistore_ptr_t> *multistore_ptr_out) {
    scoped_ptr_t<real_branch_history_manager_t> bhm(
        new real_branch_history_manager_t(table_id, metadata_file, interruptor));

    threadnum_t serializer_thread = pick_thread();
    std::vector<threadnum_t> store_threads;
    for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
        store_threads.push_back(pick_thread());
    }
    
    multistore_ptr_out->init(new real_multistore_ptr_t(
        table_id,
        file_name_for(table_id),
        std::move(bhm),
        base_path,
        io_backender,
        cache_balancer,
        rdb_context,
        outdated_index_issue_tracker,
        &get_global_perfmon_collection(), // RSI(raft): Better perfmon structure
        serializer_thread,
        store_threads));
}

