// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_PERSIST_HPP_
#define CLUSTERING_ADMINISTRATION_PERSIST_HPP_

#include <string>

#include "buffer_cache/types.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/table_manager/table_meta_manager.hpp"
#include "containers/scoped.hpp"
#include "rpc/semilattice/view.hpp"
#include "serializer/types.hpp"

class cache_balancer_t;
class cache_conn_t;
class cache_t;
class txn_t;

class branch_history_manager_t;
class io_backender_t;

namespace metadata_persistence {

class file_in_use_exc_t : public std::exception {
public:
    const char *what() const throw () {
        return "metadata file is being used by another rethinkdb process";
    }
};

template <class metadata_t>
class persistent_file_t {
public:
    persistent_file_t(io_backender_t *io_backender, const serializer_filepath_t &filename,
                      perfmon_collection_t *perfmon_parent, bool create);
    virtual ~persistent_file_t();

    virtual metadata_t read_metadata() = 0;
    virtual void update_metadata(const metadata_t &metadata) = 0;

protected:
    void get_write_transaction(object_buffer_t<txn_t> *txn_out);
    void get_read_transaction(object_buffer_t<txn_t> *txn_out);

    block_size_t get_cache_block_size() const;

private:
    /* Shared between constructors */
    void construct_serializer_and_cache(bool create, serializer_file_opener_t *file_opener,
                                        perfmon_collection_t *perfmon_parent);

    scoped_ptr_t<standard_serializer_t> serializer;
    scoped_ptr_t<cache_balancer_t> balancer;
    scoped_ptr_t<cache_t> cache;
    scoped_ptr_t<cache_conn_t> cache_conn;
};


class auth_persistent_file_t : public persistent_file_t<auth_semilattice_metadata_t> {
public:
    auth_persistent_file_t(io_backender_t *io_backender, const serializer_filepath_t &filename,
                           perfmon_collection_t *perfmon_parent);
    auth_persistent_file_t(io_backender_t *io_backender, const serializer_filepath_t &filename,
                           perfmon_collection_t *perfmon_parent,
                           const auth_semilattice_metadata_t &initial_metadata);
    ~auth_persistent_file_t();

    auth_semilattice_metadata_t read_metadata();
    void update_metadata(const auth_semilattice_metadata_t &metadata);
};

class cluster_persistent_file_t : public persistent_file_t<cluster_semilattice_metadata_t> {
public:
    cluster_persistent_file_t(io_backender_t *io_backender, const serializer_filepath_t &filename,
                              perfmon_collection_t *perfmon_parent);
    cluster_persistent_file_t(io_backender_t *io_backender, const serializer_filepath_t &filename,
                              perfmon_collection_t *perfmon_parent, const server_id_t &server_id,
                              const cluster_semilattice_metadata_t &initial_metadata);
    ~cluster_persistent_file_t();

    server_id_t read_server_id();

    cluster_semilattice_metadata_t read_metadata();
    void update_metadata(const cluster_semilattice_metadata_t &metadata);

    branch_history_manager_t *get_rdb_branch_history_manager();

private:
    void construct_branch_history_managers(bool create);

    class persistent_branch_history_manager_t;

    scoped_ptr_t<persistent_branch_history_manager_t> rdb_branch_history_manager;
};

template <class metadata_t>
class semilattice_watching_persister_t {
public:
    semilattice_watching_persister_t(
            persistent_file_t<metadata_t> *persistent_file_,
            boost::shared_ptr<semilattice_read_view_t<metadata_t> > view);

    /* `stop_and_flush()` finishes flushing the current value to disk but stops
    responding to future changes. It's usually called right before the
    destructor. */
    void stop_and_flush(signal_t *interruptor) THROWS_NOTHING {
        subs.reset();
        stop.pulse();
        wait_interruptible(&stopped, interruptor);
    }

private:
    void dump_loop(auto_drainer_t::lock_t lock);
    void on_change();

    persistent_file_t<metadata_t> *persistent_file;
    server_id_t server_id;
    boost::shared_ptr<semilattice_read_view_t<metadata_t> > view;

    scoped_ptr_t<cond_t> flush_again;

    cond_t stop, stopped;
    auto_drainer_t drainer;

    typename semilattice_read_view_t<metadata_t>::subscription_t subs;

    DISABLE_COPYING(semilattice_watching_persister_t);
};

class dummy_table_meta_persistence_interface_t :
    public table_meta_persistence_interface_t {
public:
    void read_all_tables(
            UNUSED const std::function<void(
                const namespace_id_t &table_id,
                const table_meta_persistent_state_t &state,
                scoped_ptr_t<multistore_ptr_t> &&multistore_ptr)> &callback,
            UNUSED signal_t *interruptor) {
        /* do nothing */
    }
    void add_table(
            UNUSED const namespace_id_t &table,
            UNUSED const table_meta_persistent_state_t &state,
            UNUSED scoped_ptr_t<multistore_ptr_t> *multistore_ptr_out,
            UNUSED signal_t *interruptor) {
        /* do nothing */
    }
    void update_table(
            UNUSED const namespace_id_t &table,
            UNUSED const table_meta_persistent_state_t &state,
            UNUSED signal_t *interruptor) {
        /* do nothing */
    }
    void remove_table(
            UNUSED const namespace_id_t &table,
            UNUSED signal_t *interruptor) {
        /* do nothing */
    }
};

}   /* namespace metadata_persistence */

#endif /* CLUSTERING_ADMINISTRATION_PERSIST_HPP_ */
