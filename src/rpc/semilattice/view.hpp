#ifndef RPC_SEMILATTICE_VIEW_HPP_
#define RPC_SEMILATTICE_VIEW_HPP_

#include "concurrency/pubsub.hpp"
#include "rpc/connectivity/connectivity.hpp"
#include "utils.hpp"

class sync_failed_exc_t : public std::exception {
public:
    const char *what() const throw () {
        return "Could not sync with other node, because the connection was "
            "lost before the sync was over.";
    }
};

/* A `semilattice_read_view_t` represents some sub-region of metadata. You can
read it and monitor it for changes. The purpose is to make it easier to make
metadata composable; because of `semilattice_read_view_t`, each component that
interacts with the metadata only needs to know about its own little part of the
metadata. */

/* TODO: Make this more like `directory_*_view_t`, with `clone_ptr_t` and a
`subview()` method and all that jazz.

Also, use the same locking scheme as `directory_*_view_t`, where you have to
hold the lock in order to subscribe. */

template<class metadata_t>
class semilattice_read_view_t : public home_thread_mixin_t {

public:
    virtual metadata_t get() = 0;

    virtual void sync_from(peer_id_t peer, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, sync_failed_exc_t) = 0;

    class subscription_t {
    public:
        explicit subscription_t(boost::function<void()> cb) : subs(cb) { }
        subscription_t(boost::function<void()> cb, boost::shared_ptr<semilattice_read_view_t> v) : subs(cb) {
            reset(v);
        }
        void reset(boost::shared_ptr<semilattice_read_view_t> v) {
            subs.reset(v->get_publisher());
            view = v;
        }
        void reset() {
            subs.reset(NULL);
            view.reset();
        }
    private:
        /* Hold a pointer to the `semilattice_read_view_t` so it doesn't die while
        we are subscribed to it */
        boost::shared_ptr<semilattice_read_view_t> view;
        publisher_t<boost::function<void()> >::subscription_t subs;
    };

    virtual publisher_t<boost::function<void()> > *get_publisher() = 0;

protected:
    virtual ~semilattice_read_view_t() { }
};

/* A `semilattice_readwrite_view_t` is like a `semilattice_read_view_t` except that
you can join new metadata with it as well as read from it. */

template<class metadata_t>
class semilattice_readwrite_view_t : public semilattice_read_view_t<metadata_t> {

public:
    virtual void join(const metadata_t &) = 0;

    virtual void sync_to(peer_id_t peer, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, sync_failed_exc_t) = 0;
};

#endif /* RPC_SEMILATTICE_VIEW_HPP_ */
