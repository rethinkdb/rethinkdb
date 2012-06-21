#ifndef CLUSTERING_ADMINISTRATION_PERSIST_HPP_
#define CLUSTERING_ADMINISTRATION_PERSIST_HPP_

#include "clustering/administration/issues/local.hpp"
#include "clustering/administration/metadata.hpp"
#include "rpc/semilattice/view.hpp"
#include "clustering/administration/issues/json.hpp"
#include "buffer_cache/blob.hpp"
#include "buffer_cache/mirrored/mirrored.hpp"
#include "buffer_cache/mirrored/config.hpp"
#include "buffer_cache/types.hpp"
#include "serializer/log/log_serializer.hpp"
#include "buffer_cache/semantic_checking.hpp"

namespace metadata_persistence {

class persistent_file_t {
public:
    persistent_file_t(const std::string& filename, bool create, perfmon_collection_t *perfmon_parent);

    void update(const machine_id_t &machine_id, const cluster_semilattice_metadata_t &semilattice, bool create = false);
    void read(machine_id_t *machine_id_out, cluster_semilattice_metadata_t *semilattice_out);
private:
    boost::scoped_ptr<standard_serializer_t> serializer;
    boost::scoped_ptr<cache_t> cache;
    mirrored_cache_config_t cache_dynamic_config;
};

class semilattice_watching_persister_t {
public:
    semilattice_watching_persister_t(
            persistent_file_t *persistent_file_,
            machine_id_t machine_id,
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

    boost::scoped_ptr<cond_t> flush_again;

    cond_t stop, stopped;
    auto_drainer_t drainer;

    semilattice_read_view_t<cluster_semilattice_metadata_t>::subscription_t subs;

    DISABLE_COPYING(semilattice_watching_persister_t);
};

}   /* namespace metadata_persistence */

#endif /* CLUSTERING_ADMINISTRATION_PERSIST_HPP_ */
