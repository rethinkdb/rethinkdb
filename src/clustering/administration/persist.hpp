// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_PERSIST_HPP_
#define CLUSTERING_ADMINISTRATION_PERSIST_HPP_

#include <string>

#include "buffer_cache/mirrored/config.hpp"
#include "buffer_cache/types.hpp"
#include "clustering/administration/metadata.hpp"
#include "containers/scoped.hpp"
#include "rpc/semilattice/view.hpp"
#include "serializer/types.hpp"

template <class> class branch_history_manager_t;
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
    void get_write_transaction(object_buffer_t<transaction_t> *txn_out,
                               const std::string &info);

    void get_read_transaction(object_buffer_t<transaction_t> *txn_out,
                              const std::string &info);

    block_size_t get_cache_block_size() const;

private:
    /* Shared between constructors */
    void construct_serializer_and_cache(bool create, serializer_file_opener_t *file_opener,
                                        perfmon_collection_t *perfmon_parent);

    order_source_t cache_order_source;  // order_token_t::ignore?
    scoped_ptr_t<standard_serializer_t> serializer;
    scoped_ptr_t<cache_t> cache;
    mirrored_cache_config_t cache_dynamic_config;
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
                              perfmon_collection_t *perfmon_parent, const machine_id_t &machine_id,
                              const cluster_semilattice_metadata_t &initial_metadata);
    ~cluster_persistent_file_t();

    machine_id_t read_machine_id();

    cluster_semilattice_metadata_t read_metadata();
    void update_metadata(const cluster_semilattice_metadata_t &metadata);

    branch_history_manager_t<mock::dummy_protocol_t> *get_dummy_branch_history_manager();
    branch_history_manager_t<memcached_protocol_t> *get_memcached_branch_history_manager();
    branch_history_manager_t<rdb_protocol_t> *get_rdb_branch_history_manager();

private:
    void construct_branch_history_managers(bool create);

    template <class protocol_t> class persistent_branch_history_manager_t;

    friend class persistent_branch_history_manager_t<mock::dummy_protocol_t>;
    friend class persistent_branch_history_manager_t<memcached_protocol_t>;
    friend class persistent_branch_history_manager_t<rdb_protocol_t>;

    scoped_ptr_t<persistent_branch_history_manager_t<mock::dummy_protocol_t> > dummy_branch_history_manager;
    scoped_ptr_t<persistent_branch_history_manager_t<memcached_protocol_t> > memcached_branch_history_manager;
    scoped_ptr_t<persistent_branch_history_manager_t<rdb_protocol_t> > rdb_branch_history_manager;
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
    machine_id_t machine_id;
    typename boost::shared_ptr<semilattice_read_view_t<metadata_t> > view;

    scoped_ptr_t<cond_t> flush_again;

    cond_t stop, stopped;
    auto_drainer_t drainer;

    typename semilattice_read_view_t<metadata_t>::subscription_t subs;

    DISABLE_COPYING(semilattice_watching_persister_t);
};

}   /* namespace metadata_persistence */

#endif /* CLUSTERING_ADMINISTRATION_PERSIST_HPP_ */
