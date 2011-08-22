#ifndef __RPC_METADATA_VIEW_CONTROLLER_HPP__
#define __RPC_METADATA_VIEW_CONTROLLER_HPP__

#include "rpc/metadata/view.hpp"

/* `metadata_view_controller_t` exposes a `metadata_readwrite_view_t` (via the
`get_view()` method) which isn't hooked up to any other nodes in the cluster.
It's mostly useful for testing purposes. */

template<class metadata_t>
class metadata_view_controller_t {

public:
    metadata_view_controller_t(const metadata_t &m) :
        view(this),
        metadata(m),
        change_publisher(&change_lock) { }

    metadata_readwrite_view_t<metadata_t> *get_view() {
        return &view;
    }

private:
    class view_t : public metadata_readwrite_view_t<metadata_t> {
    public:
        metadata_t get() {
            return controller->metadata;
        }
        void join(const metadata_t &new_metadata) {
            mutex_acquisition_t change_acq(&controller->change_lock);
            semilattice_join(&controller->metadata, new_metadata);
            controller->change_publisher.publish(&metadata_view_controller_t::call, &change_acq);
        }
    private:
        friend class metadata_view_controller_t;
        publisher_t<boost::function<void()> > *get_publisher() {
            return controller->change_publisher.get_publisher();
        }
        view_t(metadata_view_controller_t *c) : controller(c) { }
        metadata_view_controller_t *controller;
    } view;

    metadata_t metadata;

    mutex_t change_lock;
    static void call(const boost::function<void()> &fun) {
        fun();
    }
    publisher_controller_t<boost::function<void()> > change_publisher;
};

#endif /* __RPC_METADATA_VIEW_CONTROLLER_HPP__ */