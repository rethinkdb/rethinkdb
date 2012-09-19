#ifndef CLUSTERING_ADMINISTRATION_PERSIST_HPP_
#define CLUSTERING_ADMINISTRATION_PERSIST_HPP_

#include <string>

#include "buffer_cache/types.hpp"
#include "clustering/administration/metadata.hpp"
#include "containers/scoped.hpp"
#include "rpc/semilattice/view.hpp"
#include "serializer/types.hpp"

template <class> class branch_history_manager_t;

namespace metadata_persistence {

class file_in_use_exc_t : public std::exception {
public:
    const char *what() const throw () {
        return "metadata file is being used by another rethinkdb process";
    }
};

class persistent_file_t {
public:
    persistent_file_t(io_backender_t *io_backender, const std::string &filename, perfmon_collection_t *perfmon_parent);
    persistent_file_t(io_backender_t *io_backender, const std::string &filename, perfmon_collection_t *perfmon_parent, const machine_id_t &machine_id, const cluster_semilattice_metadata_t &initial_metadata);
    ~persistent_file_t();

    machine_id_t read_machine_id();

    cluster_semilattice_metadata_t read_metadata();
    void update_metadata(const cluster_semilattice_metadata_t &metadata);

    branch_history_manager_t<mock::dummy_protocol_t> *get_dummy_branch_history_manager();
    branch_history_manager_t<memcached_protocol_t> *get_memcached_branch_history_manager();
    branch_history_manager_t<rdb_protocol_t> *get_rdb_branch_history_manager();

private:
    template <class protocol_t> class persistent_branch_history_manager_t;

    /* Shared between constructors */
    void construct_serializer_and_cache(io_backender_t *io_backender, bool create, const std::string &filename, perfmon_collection_t *perfmon_parent);
    void construct_branch_history_managers(bool create);

    scoped_ptr_t<standard_serializer_t> serializer;
    scoped_ptr_t<cache_t> cache;
    mirrored_cache_config_t cache_dynamic_config;

    scoped_ptr_t<persistent_branch_history_manager_t<mock::dummy_protocol_t> > dummy_branch_history_manager;
    scoped_ptr_t<persistent_branch_history_manager_t<memcached_protocol_t> > memcached_branch_history_manager;
    scoped_ptr_t<persistent_branch_history_manager_t<rdb_protocol_t> > rdb_branch_history_manager;
};

class semilattice_watching_persister_t {
public:
    semilattice_watching_persister_t(
            persistent_file_t *persistent_file_,
            boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > view);

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

    persistent_file_t *persistent_file;
    machine_id_t machine_id;
    boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > view;

    scoped_ptr_t<cond_t> flush_again;

    cond_t stop, stopped;
    auto_drainer_t drainer;

    semilattice_read_view_t<cluster_semilattice_metadata_t>::subscription_t subs;

    DISABLE_COPYING(semilattice_watching_persister_t);
};

}   /* namespace metadata_persistence */

#endif /* CLUSTERING_ADMINISTRATION_PERSIST_HPP_ */
