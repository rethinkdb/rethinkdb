// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_GENERIC_RESOURCE_HPP_
#define CLUSTERING_GENERIC_RESOURCE_HPP_

#include <string>

#include "errors.hpp"
#include <boost/optional.hpp>

#include "concurrency/wait_any.hpp"
#include "concurrency/watchable.hpp"

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
    directory; the outer `boost::optional` is empty if the server is not
    connected, and the inner one is empty if the resource was destroyed. */
    explicit resource_access_t(
            clone_ptr_t<watchable_t<boost::optional<boost::optional<business_card_t> > > > v)
            THROWS_ONLY(resource_lost_exc_t) :
        metadata_view(v),
        change_subscription(std::bind(&resource_access_t::on_change, this))
    {
        watchable_freeze_t<boost::optional<boost::optional<business_card_t> > > freeze(metadata_view);
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
        guarantee(failed_signal.is_pulsed());
        return failed_reason;
    }

    /* Returns the resource contact info if the resource is accessible. Throws
    `resource_lost_exc_t` if it's not. */
    business_card_t access() THROWS_ONLY(resource_lost_exc_t) {
        /* This function was the site of #65. Previously it asserted
         * maybe_maybe_value && maybe_maybe_value.get() -> !failed_signal.is_pulsed()
         * which is incorrect because a peer can disconnect and reconnect which
         * leaves the signal pulsed but the resource as not none.
         * Now we do:
         * !maybe_maybe_value || !maybe_maybe_value.get() -> failed_signal.is_pulsed()
         * which is correct.
         * And throw resource_lost_exc_t even if the resource has come back
         * since this resource_access_t has a pulsed signal and thus can't be
         * used. (A new one needs to be constructed.)
         */
        if (failed_signal.is_pulsed()) {
            throw resource_lost_exc_t();
        } else {
            boost::optional<boost::optional<business_card_t> > maybe_maybe_value = metadata_view->get();
            guarantee(maybe_maybe_value && maybe_maybe_value.get(), "If either of these are None the signal should have been pulsed.");
            return maybe_maybe_value.get().get();
        }
    }

private:
    void on_change() {
        boost::optional<boost::optional<business_card_t> > maybe_maybe_value = metadata_view->get();
        if (!maybe_maybe_value) {
            if (!failed_signal.is_pulsed()) {
                failed_reason = "The server hosting the resource went down.";
                failed_signal.pulse();
            }
        } else {
            boost::optional<business_card_t> maybe_value = maybe_maybe_value.get();
            if (!maybe_value) {
                if (!failed_signal.is_pulsed()) {
                    failed_reason = "The server hosting the resource took the "
                        "resource down.";
                    failed_signal.pulse();
                }
            }
        }
    }

    clone_ptr_t<watchable_t<boost::optional<boost::optional<business_card_t> > > > metadata_view;
    watchable_subscription_t<boost::optional<boost::optional<business_card_t> > > change_subscription;
    cond_t failed_signal;
    std::string failed_reason;

    DISABLE_COPYING(resource_access_t);
};

#endif /* CLUSTERING_GENERIC_RESOURCE_HPP_ */
