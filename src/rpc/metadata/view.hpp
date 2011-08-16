#ifndef __RPC_METADATA_VIEW_HPP__
#define __RPC_METADATA_VIEW_HPP__

#include "concurrency/pubsub.hpp"
#include "concurrency/watchable.hpp"

/* A `metadata_view_t` represents some sub-region of metadata. You can read it,
join new values with it, and monitor it for changes. The purpose is to make it
easier to write composable components that interact with metadata; because of
`metadata_view_t`, each component that uses metadata only needs to know about
the part that it interacts with directly. */

template<class metadata_t>
class metadata_view_t {

public:
    virtual metadata_t get() = 0;
    virtual void join(const metadata_t &) = 0;

    class subscription_t {
    public:
        subscription_t(boost::function<void()> cb) : subs(cb) { }
        subscription_t(boost::function<void()> cb, metadata_view_t *v) :
            subs(cb, v->get_publisher()) { }
        void resubscribe(metadata_view_t *v) {
            subs.resubscribe(v->get_publisher());
        }
        void unsubscribe() {
            subs.unsubscribe();
        }
    private:
        publisher_t<boost::function<void()> >::subscription_t subs;
    };

protected:
    virtual ~metadata_view_t() { }

private:
    virtual publisher_t<boost::function<void()> > *get_publisher() = 0;
};

/* `metadata_field_view_t` is a `metadata_view_t` that corresponds to some
sub-field of another `metadata_view_t`. Pass the field to the constructor as a
member pointer. */

template<class outer_t, class inner_t>
class metadata_field_view_t : public metadata_view_t<inner_t> {

public:
    metadata_field_view_t(inner_t outer_t::*f, metadata_view_t<outer_t> *o) :
        field(f), outer(o) { }

    inner_t get() {
        return ((outer->get()).*field);
    }

    void join(const inner_t &new_inner) {
        outer_t value = outer->get();
        semilattice_join(&(value.*field), new_inner);
        outer->join(value);
    }

private:
    publisher_t<boost::function<void()> > *get_publisher() {
        return outer->get_publisher();
    }

    inner_t outer_t::*field;
    metadata_view_t<outer_t> *outer;
};

#endif /* __RPC_METADATA_VIEW_HPP__ */
