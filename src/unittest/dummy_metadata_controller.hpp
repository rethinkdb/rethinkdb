#ifndef __UNITTEST_DUMMY_METADATA_CONTROLLER_HPP__
#define __UNITTEST_DUMMY_METADATA_CONTROLLER_HPP__

#include "errors.hpp"
#include <boost/make_shared.hpp>

#include "rpc/metadata/view.hpp"

/* `dummy_metadata_controller_t` exposes a `metadata_readwrite_view_t` (via the
`get_view()` method) which isn't hooked up to any other nodes in the cluster.
It's mostly useful for testing purposes. */

template<class metadata_t>
class dummy_metadata_controller_t {

public:
    dummy_metadata_controller_t(const metadata_t &m) :
        view(boost::make_shared<view_t>(this)),
        metadata(m),
        change_publisher(&change_lock) { }

    ~dummy_metadata_controller_t() {
        view->controller = NULL;
    }

    boost::shared_ptr<metadata_readwrite_view_t<metadata_t> > get_view() {
        return view;
    }

private:
    class view_t : public metadata_readwrite_view_t<metadata_t> {
    public:
        view_t(dummy_metadata_controller_t *c) : controller(c) { }
        metadata_t get() {
            rassert(controller, "accessing a `dummy_metadata_controller_t`'s "
                "view after the controller was destroyed.");
            return controller->metadata;
        }
        void join(const metadata_t &new_metadata) {
            rassert(controller, "accessing a `dummy_metadata_controller_t`'s "
                "view after the controller was destroyed.");
            {
                mutex_acquisition_t change_acq(&controller->change_lock);
                semilattice_join(&controller->metadata, new_metadata);
                controller->change_publisher.publish(&dummy_metadata_controller_t::call, &change_acq);
            }
            if (rng.randint(2) == 0) nap(rng.randint(10));
        }
        void sync_from(peer_id_t, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, sync_failed_exc_t) {
            if (rng.randint(2) == 0) nap(rng.randint(10), interruptor);
        }
        void sync_to(peer_id_t, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, sync_failed_exc_t) {
            if (rng.randint(2) == 0) nap(rng.randint(10), interruptor);
        }
        publisher_t<boost::function<void()> > *get_publisher() {
            rassert(controller, "accessing a `dummy_metadata_controller_t`'s "
                "view after the controller was destroyed.");
            return controller->change_publisher.get_publisher();
        }
        dummy_metadata_controller_t *controller;
        rng_t rng;
    };
    boost::shared_ptr<view_t> view;

    metadata_t metadata;

    mutex_t change_lock;
    static void call(const boost::function<void()> &fun) {
        fun();
    }
    publisher_controller_t<boost::function<void()> > change_publisher;
};

#endif /* __UNITTEST_DUMMY_METADATA_CONTROLLER_HPP__ */