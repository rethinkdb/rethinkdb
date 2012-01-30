#ifndef __CLUSTERING_RESOURCE_HPP__
#define __CLUSTERING_RESOURCE_HPP__

#include "concurrency/wait_any.hpp"
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
    resource is inaccessible. */
    explicit resource_access_t(
            clone_ptr_t<directory_single_rview_t<boost::optional<business_card_t> > > v)
            THROWS_ONLY(resource_lost_exc_t) :
        metadata_view(v),
        connectivity_subscription(NULL, boost::bind(&resource_access_t::on_disconnect, this, _1)),
        change_subscription(boost::bind(&resource_access_t::on_change, this))
    {
        connectivity_service_t::peers_list_freeze_t connectivity_freeze(
            metadata_view->get_directory_service()->get_connectivity_service());
        if (metadata_view->get_directory_service()->get_connectivity_service()->get_peer_connected(
                metadata_view->get_peer())) {
            directory_read_service_t::peer_value_freeze_t change_freeze(
                metadata_view->get_directory_service(),
                metadata_view->get_peer());
            if (metadata_view->get_value().get()) {
                connectivity_subscription.reset(
                    metadata_view->get_directory_service()->get_connectivity_service(),
                    &connectivity_freeze);
                change_subscription.reset(
                    metadata_view->get_directory_service(),
                    metadata_view->get_peer(),
                    &change_freeze);
            } else {
                throw resource_lost_exc_t();
            }
        } else {
            throw resource_lost_exc_t();
        }
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
        boost::optional<boost::optional<business_card_t> > maybe_maybe_value = metadata_view->get_value();
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
    void on_disconnect(peer_id_t peer) {

        if (peer != metadata_view->get_peer()) {
            return;
        }

        /* We are obligated to deregister the change watcher as soon as we
        receive notification of the disconnection, because we can't have a
        change watcher for a disconnected peer. */
        change_subscription.reset();

        if (!failed_signal.is_pulsed()) {
            failed_reason = "We lost contact with the machine hosting the "
                "resource.";
            failed_signal.pulse();
        }
    }

    void on_change() {
        boost::optional<boost::optional<business_card_t> > maybe_maybe_value = metadata_view->get_value();
        rassert(maybe_maybe_value);
        boost::optional<business_card_t> maybe_value = maybe_maybe_value.get();
        if (!maybe_value) {
            if (!failed_signal.is_pulsed()) {
                failed_reason = "The machine hosting the resource took the "
                    "resource down.";
                failed_signal.pulse();
            }
        }
    }

    clone_ptr_t<directory_single_rview_t<boost::optional<business_card_t> > > metadata_view;
    connectivity_service_t::peers_list_subscription_t connectivity_subscription;
    directory_read_service_t::peer_value_subscription_t change_subscription;
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

#endif /* __CLUSTERING_RESOURCE_HPP__ */
