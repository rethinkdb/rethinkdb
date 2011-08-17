#ifndef __RPC_METADATA_VIEW_HPP__
#define __RPC_METADATA_VIEW_HPP__

#include "concurrency/pubsub.hpp"

/* A `metadata_read_view_t` represents some sub-region of metadata. You can read
it and monitor it for changes. The purpose is to make it easier to make metadata
composable; because of `metadata_read_view_t`, each component that interacts
with the metadata only needs to know about its own little part of the metadata.

Note that `metadata_read_view_t` doesn't necessarily correspond to an actual
location in the metadata of a real cluster. It's also sometimes used in the
other direction: some components create and expose their own
`metadata_read_view_t`, and expect their users to link their
`metadata_read_view_t` to some location in the cluster's metadata. */

template<class metadata_t>
class metadata_read_view_t {

public:
    virtual metadata_t get() = 0;

    class subscription_t {
    public:
        subscription_t(boost::function<void()> cb) : subs(cb) { }
        subscription_t(boost::function<void()> cb, metadata_read_view_t *v) :
            subs(cb, v->get_publisher()) { }
        void resubscribe(metadata_read_view_t *v) {
            subs.resubscribe(v->get_publisher());
        }
        void unsubscribe() {
            subs.unsubscribe();
        }
    private:
        publisher_t<boost::function<void()> >::subscription_t subs;
    };

    virtual publisher_t<boost::function<void()> > *get_publisher() = 0;

protected:
    virtual ~metadata_read_view_t() { }
};

/* A `metadata_readwrite_view_t` is like a `metadata_read_view_t` except that
you can join new metadata with it as well as read from it. */

template<class metadata_t>
class metadata_readwrite_view_t : public metadata_read_view_t<metadata_t> {

public:
    virtual void join(const metadata_t &) = 0;
};

#endif /* __RPC_METADATA_VIEW_HPP__ */
