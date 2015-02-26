// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_PERSIST_SEMILATTICE_HPP_
#define CLUSTERING_ADMINISTRATION_PERSIST_SEMILATTICE_HPP_

#include "clustering/administration/persist/file.hpp"

template <class metadata_t>
class semilattice_persister_t {
public:
    semilattice_persister_t(
        metadata_file_t *file,
        const metadata_file_t::key_t<metadata_t> &key,
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

    metadata_file_t * const file;
    metadata_file_t::key_t<metadata_t> key;
    boost::shared_ptr<semilattice_read_view_t<metadata_t> > view;

    scoped_ptr_t<cond_t> flush_again;

    cond_t stop, stopped;
    auto_drainer_t drainer;

    typename semilattice_read_view_t<metadata_t>::subscription_t subs;

    DISABLE_COPYING(semilattice_watching_persister_t);
};

#endif /* CLUSTERING_ADMINISTRATION_PERSIST_SEMILATTICE_HPP_ */

