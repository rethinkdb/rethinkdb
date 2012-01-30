#include "errors.hpp"
#include <boost/make_shared.hpp>

/* `semilattice_field_read_view_t` is a `semilattice_read_view_t` that corresponds to
some sub-field of another `semilattice_read_view_t`. Pass the field to the
constructor as a member pointer. */

template<class outer_t, class inner_t>
class semilattice_field_read_view_t : public semilattice_read_view_t<inner_t> {

public:
    semilattice_field_read_view_t(inner_t outer_t::*f, boost::shared_ptr<semilattice_read_view_t<outer_t> > o) :
        field(f), outer(o) { }

    inner_t get() {
        return ((outer->get()).*field);
    }

    void sync_from(peer_id_t peer, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, sync_failed_exc_t) {
        outer->sync_from(peer, interruptor);
    }

    publisher_t<boost::function<void()> > *get_publisher() {
        return outer->get_publisher();
    }

private:
    inner_t outer_t::*field;
    boost::shared_ptr<semilattice_read_view_t<outer_t> > outer;
};

/* A `semilattice_field_readwrite_view_t` is like a `semilattice_field_read_view_t`
except that it also supports joining. */

template<class outer_t, class inner_t>
class semilattice_field_readwrite_view_t : public semilattice_readwrite_view_t<inner_t> {

public:
    semilattice_field_readwrite_view_t(inner_t outer_t::*f, boost::shared_ptr<semilattice_readwrite_view_t<outer_t> > o) :
        field(f), outer(o) { }

    inner_t get() {
        outer_t value(outer->get());
        return (value).*field;
    }

    void join(const inner_t &new_inner) {
        outer_t value = outer->get();
        semilattice_join(&(value.*field), new_inner);
        outer->join(value);
    }

    void sync_from(peer_id_t peer, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, sync_failed_exc_t) {
        outer->sync_from(peer, interruptor);
    }

    void sync_to(peer_id_t peer, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, sync_failed_exc_t) {
        outer->sync_to(peer, interruptor);
    }

    publisher_t<boost::function<void()> > *get_publisher() {
        return outer->get_publisher();
    }

private:
    inner_t outer_t::*field;
    boost::shared_ptr<semilattice_readwrite_view_t<outer_t> > outer;
};

/* Convenient wrappers around `semilattice_field_read[write]_view_t` */

template<class outer_t, class inner_t>
boost::shared_ptr<semilattice_read_view_t<inner_t> > metadata_field(
        inner_t outer_t::*field,
        boost::shared_ptr<semilattice_read_view_t<outer_t> > outer)
{
    return boost::make_shared<semilattice_field_read_view_t<outer_t, inner_t> >(
        field, outer);
}

template<class outer_t, class inner_t>
boost::shared_ptr<semilattice_readwrite_view_t<inner_t> > metadata_field(
        inner_t outer_t::*field,
        boost::shared_ptr<semilattice_readwrite_view_t<outer_t> > outer)
{
    return boost::make_shared<semilattice_field_readwrite_view_t<outer_t, inner_t> >(
        field, outer);
}
