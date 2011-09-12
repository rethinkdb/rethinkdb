#include "errors.hpp"
#include <boost/make_shared.hpp>

/* `metadata_field_read_view_t` is a `metadata_read_view_t` that corresponds to
some sub-field of another `metadata_read_view_t`. Pass the field to the
constructor as a member pointer. */

template<class outer_t, class inner_t>
class metadata_field_read_view_t : public metadata_read_view_t<inner_t> {

public:
    metadata_field_read_view_t(inner_t outer_t::*f, boost::shared_ptr<metadata_read_view_t<outer_t> > o) :
        field(f), outer(o) { }

    inner_t get() {
        return ((outer->get()).*field);
    }

    publisher_t<boost::function<void()> > *get_publisher() {
        return outer->get_publisher();
    }

private:
    inner_t outer_t::*field;
    boost::shared_ptr<metadata_read_view_t<outer_t> > outer;
};

/* A `metadata_field_readwrite_view_t` is like a `metadata_field_read_view_t`
except that it also supports joining. */

template<class outer_t, class inner_t>
class metadata_field_readwrite_view_t : public metadata_readwrite_view_t<inner_t> {

public:
    metadata_field_readwrite_view_t(inner_t outer_t::*f, boost::shared_ptr<metadata_readwrite_view_t<outer_t> > o) :
        field(f), outer(o) { }

    inner_t get() {
        return ((outer->get()).*field);
    }

    void join(const inner_t &new_inner) {
        outer_t value = outer->get();
        semilattice_join(&(value.*field), new_inner);
        outer->join(value);
    }

    publisher_t<boost::function<void()> > *get_publisher() {
        return outer->get_publisher();
    }

private:
    inner_t outer_t::*field;
    boost::shared_ptr<metadata_readwrite_view_t<outer_t> > outer;
};

/* Convenient wrappers around `metadata_field_read[write]_view_t` */

template<class outer_t, class inner_t>
boost::shared_ptr<metadata_read_view_t<inner_t> > metadata_field(
        inner_t outer_t::*field,
        boost::shared_ptr<metadata_read_view_t<outer_t> > outer)
{
    return boost::make_shared<metadata_field_read_view_t<outer_t, inner_t> >(
        field, outer);
}

template<class outer_t, class inner_t>
boost::shared_ptr<metadata_readwrite_view_t<inner_t> > metadata_field(
        inner_t outer_t::*field,
        boost::shared_ptr<metadata_readwrite_view_t<outer_t> > outer)
{
    return boost::make_shared<metadata_field_readwrite_view_t<outer_t, inner_t> >(
        field, outer);
}
