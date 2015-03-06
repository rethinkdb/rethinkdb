// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_PERSIST_SEMILATTICE_HPP_
#define CLUSTERING_ADMINISTRATION_PERSIST_SEMILATTICE_HPP_

#include "clustering/administration/persist/file.hpp"
#include "concurrency/pump_coro.hpp"
#include "rpc/semilattice/view.hpp"

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
        persist_pumper.flush(interruptor);
    }

private:
    void persist(signal_t *interruptor);

    metadata_file_t * const file;
    metadata_file_t::key_t<metadata_t> const key;
    boost::shared_ptr<semilattice_read_view_t<metadata_t> > const view;

    pump_coro_t persist_pumper;

    typename semilattice_read_view_t<metadata_t>::subscription_t subs;

    DISABLE_COPYING(semilattice_persister_t);
};

#endif /* CLUSTERING_ADMINISTRATION_PERSIST_SEMILATTICE_HPP_ */

