// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_PERSIST_TABLE_INTERFACE_HPP_
#define CLUSTERING_ADMINISTRATION_PERSIST_TABLE_INTERFACE_HPP_

#include "clustering/administration/perfmon_collection_repo.hpp"
#include "clustering/table_manager/table_metadata.hpp"

class cache_balancer_t;
class metadata_file_t;
class outdated_index_issue_tracker_t;

class real_table_persistence_interface_t :
    public table_persistence_interface_t {
public:
    real_table_persistence_interface_t(
            io_backender_t *_io_backender,
            cache_balancer_t *_cache_balancer,
            const base_path_t &_base_path,
            outdated_index_issue_tracker_t *_outdated_index_issue_tracker,
            rdb_context_t *_rdb_context,
            metadata_file_t *_metadata_file) :
        io_backender(_io_backender),
        cache_balancer(_cache_balancer),
        base_path(_base_path),
        outdated_index_issue_tracker(_outdated_index_issue_tracker),
        rdb_context(_rdb_context),
        metadata_file(_metadata_file),
        thread_counter(0)
        { }

    void read_all_metadata(
        const std::function<void(
            const namespace_id_t &table_id,
            const table_persistent_state_t &state)> &callback,
        signal_t *interruptor);
    void write_metadata(
        const namespace_id_t &table_id,
        const table_persistent_state_t &state,
        signal_t *interruptor);
    void delete_metadata(
        const namespace_id_t &table_id,
        signal_t *interruptor);

    void load_multistore(
        const namespace_id_t &table_id,
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
        scoped_ptr_t<multistore_ptr_t> *multistore_ptr_in,
        signal_t *interruptor);

private:
    serializer_filepath_t file_name_for(const namespace_id_t &table_id);
    threadnum_t pick_thread();

    io_backender_t * const io_backender;
    cache_balancer_t * const cache_balancer;
    base_path_t const base_path;
    outdated_index_issue_tracker_t * const outdated_index_issue_tracker;
    rdb_context_t * const rdb_context;
    metadata_file_t * const metadata_file;

    /* `pick_thread()` uses this to distribute objects evenly over threads */
    int thread_counter;
};

#endif /* CLUSTERING_ADMINISTRATION_PERSIST_TABLE_INTERFACE_HPP_ */
