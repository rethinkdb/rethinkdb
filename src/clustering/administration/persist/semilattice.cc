// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/persist/semilattice.hpp"

#include "clustering/administration/metadata.hpp"
#include "concurrency/wait_any.hpp"

template <class metadata_t>
semilattice_persister_t<metadata_t>::semilattice_persister_t(
        metadata_file_t *f,
        const metadata_file_t::key_t<metadata_t> &k,
        boost::shared_ptr<semilattice_read_view_t<metadata_t> > v) :
    file(f), key(k), view(v),
    flush_again(new cond_t),
    subs(std::bind(&semilattice_persister_t::on_change, this), v)
{
    coro_t::spawn_sometime(std::bind(&semilattice_persister_t::dump_loop,
        this, auto_drainer_t::lock_t(&drainer)));
}

template <class metadata_t>
void semilattice_persister_t<metadata_t>::dump_loop(
        auto_drainer_t::lock_t keepalive) {
    try {
        for (;;) {
            {
                metadata_file_t::write_txn_t txn(file, keepalive.get_drain_signal());
                txn.write(key, view->get(), keepalive.get_drain_signal());
            }
            {
                wait_any_t c(flush_again.get(), &stop);
                wait_interruptible(&c, keepalive.get_drain_signal());
            }
            if (flush_again->is_pulsed()) {
                scoped_ptr_t<cond_t> tmp(new cond_t);
                flush_again.swap(tmp);
            } else {
                break;
            }
        }
    } catch (const interrupted_exc_t &) {
        // do nothing
    }
    stopped.pulse();
}

template <class metadata_t>
void semilattice_persister_t<metadata_t>::on_change() {
    if (!flush_again->is_pulsed()) {
        flush_again->pulse();
    }
}

template class semilattice_persister_t<cluster_semilattice_metadata_t>;
template class semilattice_persister_t<auth_semilattice_metadata_t>;

