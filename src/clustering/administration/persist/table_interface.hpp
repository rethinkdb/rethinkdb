// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_PERSIST_TABLE_INTERFACE_HPP_
#define CLUSTERING_ADMINISTRATION_PERSIST_TABLE_INTERFACE_HPP_

#include "clustering/administration/perfmon_collection_repo.hpp"
#include "clustering/administration/persist/raft_storage_interface.hpp"
#include "clustering/table_manager/table_metadata.hpp"

class cache_balancer_t;
class metadata_file_t;
class real_multistore_ptr_t;
class table_raft_storage_interface_t;

class real_table_persistence_interface_t :
    public table_persistence_interface_t {
public:
    real_table_persistence_interface_t(
            io_backender_t *_io_backender,
            cache_balancer_t *_cache_balancer,
            const base_path_t &_base_path,
            rdb_context_t *_rdb_context,
            metadata_file_t *_metadata_file) :
        io_backender(_io_backender),
        cache_balancer(_cache_balancer),
        base_path(_base_path),
        rdb_context(_rdb_context),
        metadata_file(_metadata_file),
        /* We assign threads from the lowest thread number upwards. This is to reduce
        the potential for conflicting with cluster connection threads, which are
        assigned from the highest thread number downwards. */
        thread_allocator([](threadnum_t a, threadnum_t b) {
            return a.threadnum < b.threadnum;
        })
        { }

    void read_all_metadata(
        const std::function<void(
            const namespace_id_t &table_id,
            const table_active_persistent_state_t &state,
            raft_storage_interface_t<table_raft_state_t> *raft_storage,
            metadata_file_t::read_txn_t *metadata_read_txn)> &active_cb,
        const std::function<void(
            const namespace_id_t &table_id,
            const table_inactive_persistent_state_t &state,
            metadata_file_t::read_txn_t *metadata_read_txn)> &inactive_cb,
        signal_t *interruptor);
    void write_metadata_active(
        const namespace_id_t &table_id,
        const table_active_persistent_state_t &state,
        const raft_persistent_state_t<table_raft_state_t> &raft_state,
        raft_storage_interface_t<table_raft_state_t> **raft_storage_out);
    void write_metadata_inactive(
        const namespace_id_t &table_id,
        const table_inactive_persistent_state_t &state);
    void delete_metadata(
        const namespace_id_t &table_id);

    void load_multistore(
        const namespace_id_t &table_id,
        metadata_file_t::read_txn_t *metadata_read_txn,
        scoped_ptr_t<multistore_ptr_t> *multistore_ptr_out,
        signal_t *interruptor,
        perfmon_collection_t *perfmon_collection_serializers);
    void create_multistore(
        const namespace_id_t &table_id,
        scoped_ptr_t<multistore_ptr_t> *multistore_ptr_out,
        signal_t *interruptor,
        perfmon_collection_t *perfmon_collection_serializers);
    void destroy_multistore(
        const namespace_id_t &table_id,
        scoped_ptr_t<multistore_ptr_t> *multistore_ptr_in);

    bool is_gc_active() const;

private:
    serializer_filepath_t file_name_for(const namespace_id_t &table_id);
    threadnum_t pick_thread();

    io_backender_t * const io_backender;
    cache_balancer_t * const cache_balancer;
    base_path_t const base_path;
    rdb_context_t * const rdb_context;
    metadata_file_t * const metadata_file;

    std::map<
        namespace_id_t, std::pair<real_multistore_ptr_t *, auto_drainer_t::lock_t>
    > real_multistores;

    std::map<namespace_id_t, scoped_ptr_t<table_raft_storage_interface_t> >
        storage_interfaces;

    /* Used to distribute objects evenly over threads */
    thread_allocator_t thread_allocator;
};

#endif /* CLUSTERING_ADMINISTRATION_PERSIST_TABLE_INTERFACE_HPP_ */
