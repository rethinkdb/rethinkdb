// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef UNITTEST_DUMMY_METADATA_CONTROLLER_HPP_
#define UNITTEST_DUMMY_METADATA_CONTROLLER_HPP_

#include "errors.hpp"
#include <boost/make_shared.hpp>

#include "arch/timing.hpp"
#include "rpc/semilattice/view.hpp"
#include "concurrency/mutex.hpp"

/* `dummy_semilattice_controller_t` exposes a `semilattice_readwrite_view_t`
(via the `get_view()` method) which isn't hooked up to any other nodes in the
cluster. It's mostly useful for testing purposes. It only works on one thread.
*/

template<class metadata_t>
class dummy_semilattice_controller_t {

public:
    dummy_semilattice_controller_t() :
        view(boost::make_shared<view_t>(this)) { }
    explicit dummy_semilattice_controller_t(const metadata_t &m) :
        view(boost::make_shared<view_t>(this)),
        metadata(m) { }

    ~dummy_semilattice_controller_t() {
        view->controller = NULL;
    }

    boost::shared_ptr<semilattice_readwrite_view_t<metadata_t> > get_view() {
        return view;
    }

private:
    class view_t : public semilattice_readwrite_view_t<metadata_t> {
    public:
        explicit view_t(dummy_semilattice_controller_t *c) : controller(c) { }
        metadata_t get() {
            rassert(controller, "accessing a `dummy_semilattice_controller_t`'s "
                "view after the controller was destroyed.");
            return controller->metadata;
        }
        void join(const metadata_t &new_metadata) {
            rassert(controller, "accessing a `dummy_semilattice_controller_t`'s "
                "view after the controller was destroyed.");
            {
                mutex_t::acq_t change_acq(&controller->change_lock);
                semilattice_join(&controller->metadata, new_metadata);
                controller->change_publisher.publish(&dummy_semilattice_controller_t::call);
            }
            if (rng.randint(2) == 0) nap(rng.randint(10));
        }
        void sync_from(peer_id_t, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, sync_failed_exc_t) {
            if (rng.randint(2) == 0) nap(rng.randint(10), interruptor);
        }
        void sync_to(peer_id_t, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, sync_failed_exc_t) {
            if (rng.randint(2) == 0) nap(rng.randint(10), interruptor);
        }
        publisher_t<std::function<void()> > *get_publisher() {
            rassert(controller, "accessing a `dummy_semilattice_controller_t`'s "
                "view after the controller was destroyed.");
            return controller->change_publisher.get_publisher();
        }
        dummy_semilattice_controller_t *controller;
        rng_t rng;
    };
    boost::shared_ptr<view_t> view;

    metadata_t metadata;

    mutex_t change_lock;
    static void call(const std::function<void()> &fun) {
        fun();
    }
    publisher_controller_t<std::function<void()> > change_publisher;
};

#endif /* UNITTEST_DUMMY_METADATA_CONTROLLER_HPP_ */
