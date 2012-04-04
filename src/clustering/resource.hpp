#ifndef CLUSTERING_RESOURCE_HPP_
#define CLUSTERING_RESOURCE_HPP_

#include "concurrency/wait_any.hpp"
#include "concurrency/watchable.hpp"
#include "rpc/directory/read_view.hpp"
#include "rpc/directory/write_view.hpp"
#include "rpc/mailbox/mailbox.hpp"
#include "rpc/mailbox/typed.hpp"

/* It's a common paradigm to have some resource that you find out about through
metadata, and need to monitor in case it goes offline. This type facilitates
that. */

template<class business_card_t>
class resource_access_t;

/* A resource-user constructs a `resource_access_t` when it wants to start using
the resource. The `resource_access_t` will give it access to the resource's
contact info, but also ensure safety by watching to see if the resource goes
down. */

class resource_lost_exc_t : public std::exception {
public:
    resource_lost_exc_t() { }
    const char *what() const throw () {
        return "access to resource interrupted";
    }
};

template<class business_card_t>
class resource_access_t {

public:
    /* Starts monitoring the resource. Throws `resource_lost_exc_t` if the
    resource is inaccessible. In general, `v` refers to something in the
    directory; the outer `boost::optional` is empty if the machine is not
    connected, and the inner one is empty if the resource was destroyed. */
    explicit resource_access_t(
            clone_ptr_t<watchable_t<boost::optional<boost::optional<business_card_t> > > > v)
            THROWS_ONLY(resource_lost_exc_t) :
        metadata_view(v),
        change_subscription(boost::bind(&resource_access_t::on_change, this))
    {
        typename watchable_t<boost::optional<boost::optional<business_card_t> > >::freeze_t freeze(metadata_view);
        if (!metadata_view->get() || !metadata_view->get().get()) {
            throw resource_lost_exc_t();
        }
        change_subscription.reset(metadata_view, &freeze);
    }

    /* Returns a `signal_t *` that will be pulsed when access to the resource is
    lost. */
    signal_t *get_failed_signal() {
        return &failed_signal;
    }

    /* Returns a string that describes why `get_failed_signal()` is pulsed. */
    std::string get_failed_reason() {
        rassert(failed_signal.is_pulsed());
        return failed_reason;
    }

    /* Returns the resource contact info if the resource is accessible. Throws
    `resource_lost_exc_t` if it's not. */
    business_card_t access() THROWS_ONLY(resource_lost_exc_t) {
        boost::optional<boost::optional<business_card_t> > maybe_maybe_value = metadata_view->get();
        if (!maybe_maybe_value) {
            rassert(failed_signal.is_pulsed());
            throw resource_lost_exc_t();
        }
        if (!maybe_maybe_value.get()) {
            rassert(failed_signal.is_pulsed());
            throw resource_lost_exc_t();
        }
        rassert(!failed_signal.is_pulsed());
        return maybe_maybe_value.get().get();
    }

private:
    void on_change() {
        boost::optional<boost::optional<business_card_t> > maybe_maybe_value = metadata_view->get();
        if (!maybe_maybe_value) {
            if (!failed_signal.is_pulsed()) {
                failed_reason = "The machine hosting the resource went down.";
                failed_signal.pulse();
            }
        } else {
            boost::optional<business_card_t> maybe_value = maybe_maybe_value.get();
            if (!maybe_value) {
                if (!failed_signal.is_pulsed()) {
                    failed_reason = "The machine hosting the resource took the "
                        "resource down.";
                    failed_signal.pulse();
                }
            }
        }
    }

    clone_ptr_t<watchable_t<boost::optional<boost::optional<business_card_t> > > > metadata_view;
    typename watchable_t<boost::optional<boost::optional<business_card_t> > >::subscription_t change_subscription;
    cond_t failed_signal;
    std::string failed_reason;
};

/* The most common type of resource is one that registers/deregisters itself by
making an entry in a `std::map`. Here's a type that automatically handles that
for you using RAII. */

template<class key_t, class business_card_t>
class resource_map_advertisement_t {
public:
    resource_map_advertisement_t(
            clone_ptr_t<directory_wview_t<std::map<key_t, business_card_t> > > mv,
            key_t k,
            business_card_t business_card) :
        map_view(mv),
        key(k)
    {
        directory_write_service_t::our_value_lock_acq_t lock(map_view->get_directory_service());
        std::map<key_t, business_card_t> map = map_view->get_our_value(&lock);
        rassert(map.count(key) == 0);
        map[key] = business_card;
        map_view->set_our_value(map, &lock);
    }

    ~resource_map_advertisement_t() {
        directory_write_service_t::our_value_lock_acq_t lock(map_view->get_directory_service());
        std::map<key_t, business_card_t> map = map_view->get_our_value(&lock);
        rassert(map.count(key) == 1);
        map.erase(key);
        map_view->set_our_value(map, &lock);
    }

private:
    clone_ptr_t<directory_wview_t<std::map<key_t, business_card_t> > > map_view;
    key_t key;
};

#endif /* CLUSTERING_RESOURCE_HPP_ */
