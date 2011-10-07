#ifndef __RPC_METADATA_VIEW_HPP__
#define __RPC_METADATA_VIEW_HPP__

#include "concurrency/pubsub.hpp"

class sync_failed_exc_t : public std::exception {
public:
    const char *what() const throw () {
        return "Could not sync with other node, because the connection was "
            "lost before the sync was over.";
    }
};

/* A `metadata_read_view_t` represents some sub-region of metadata. You can read
it and monitor it for changes. The purpose is to make it easier to make metadata
composable; because of `metadata_read_view_t`, each component that interacts
with the metadata only needs to know about its own little part of the metadata.
*/

template<class metadata_t>
class metadata_read_view_t {

public:
    virtual metadata_t get() = 0;

    virtual void sync_from(peer_id_t peer, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, sync_failed_exc_t) = 0;

    class subscription_t {
    public:
        subscription_t(boost::function<void()> cb) : subs(cb) { }
        subscription_t(boost::function<void()> cb, boost::shared_ptr<metadata_read_view_t> v) :
            view(v), subs(cb, view->get_publisher()) { }
        void resubscribe(boost::shared_ptr<metadata_read_view_t> v) {
            view = v;
            subs.resubscribe(view->get_publisher());
        }
        void unsubscribe() {
            subs.unsubscribe();
            view.reset();
        }
    private:
        /* Hold a pointer to the `metadata_read_view_t` so it doesn't die while
        we are subscribed to it */
        boost::shared_ptr<metadata_read_view_t> view;
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

    virtual void sync_to(peer_id_t peer, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, sync_failed_exc_t) = 0;
};

#endif /* __RPC_METADATA_VIEW_HPP__ */
