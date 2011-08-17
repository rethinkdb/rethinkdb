#ifndef __RPC_METADATA_VIEW_CONTROLLER_HPP__
#define __RPC_METADATA_VIEW_CONTROLLER_HPP__

#include "rpc/metadata/view.hpp"

/* `metadata_read_view_controller_t` exposes a `metadata_read_view_t` (via the
`get_view()` method), the value of which can be changed by calling the `join()`
method on the controller. */

template<class metadata_t>
class metadata_read_view_controller_t {

public:
    metadata_read_view_controller_t(const metadata_t &m) :
        view(this),
        metadata(m),
        change_publisher(&change_lock) { }

    metadata_read_view_t<metadata_t> *get_view() {
        return &view;
    }

    void join(const metadata_t &new_metadata) {
        mutex_acquisition_t change_acq(&change_lock);
        semilattice_join(&metadata, new_metadata);
        change_publisher.publish(&call, &change_acq);
    }

private:
    class view_t : public metadata_read_view_t<metadata_t> {
    public:
        metadata_t get() {
            return controller->metadata;
        }
    private:
        friend class metadata_read_view_controller_t;
        publisher_t<boost::function<void()> > *get_publisher() {
            return controller->change_publisher.get_publisher();
        }
        view_t(metadata_read_view_controller_t *c) : controller(c) { }
        metadata_read_view_controller_t *controller;
    } view;

    metadata_t metadata;

    mutex_t change_lock;
    static void call(const boost::function<void()> &fun) {
        fun();
    }
    publisher_controller_t<boost::function<void()> > change_publisher;
};

#endif /* __RPC_METADATA_VIEW_CONTROLLER_HPP__ */