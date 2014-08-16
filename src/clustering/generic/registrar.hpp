// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_GENERIC_REGISTRAR_HPP_
#define CLUSTERING_GENERIC_REGISTRAR_HPP_

#include <algorithm>
#include <functional>
#include <map>
#include <utility>

#include "clustering/generic/registration_metadata.hpp"
#include "clustering/generic/resource.hpp"
#include "rpc/mailbox/typed.hpp"
#include "concurrency/coro_pool.hpp"
#include "concurrency/promise.hpp"
#include "concurrency/queue/unlimited_fifo.hpp"
#include "concurrency/wait_any.hpp"

template<class business_card_t, class user_data_type, class registrant_type>
class registrar_t {

public:
    registrar_t(mailbox_manager_t *mm, user_data_type co) :
        mailbox_manager(mm), controller(co),
        registration_destruction_pool(1, &registration_destruction_queue,
                              &registration_destruction_callback),
        create_mailbox(mailbox_manager, std::bind(&registrar_t::on_create,
                                                  this, ph::_1, ph::_2, ph::_3,
                                                  auto_drainer_t::lock_t(&drainer))),
        delete_mailbox(mailbox_manager, std::bind(&registrar_t::on_delete,
                                                  this, ph::_1,
                                                  auto_drainer_t::lock_t(&drainer)))
    { }

    registrar_business_card_t<business_card_t> get_business_card() {
        return registrar_business_card_t<business_card_t>(
            create_mailbox.get_address(),
            delete_mailbox.get_address());
    }

private:
    typedef typename registrar_business_card_t<business_card_t>::registration_id_t
        registration_id_t;

    class active_registration_t : public signal_t::subscription_t {
    public:
        active_registration_t(
                registrar_t *_registrar,
                mutex_t::acq_t &&_mutex_acq,
                registration_id_t rid,
                peer_id_t peer,
                business_card_t business_card,
                auto_drainer_t::lock_t &&_keepalive) :
            keepalive(std::move(_keepalive)),
            registrar(_registrar),
            mutex_acq(std::move(_mutex_acq)),
            /* Construct a `registrant_t` to tell the controller that something has
            now registered. */
            registrant(registrar->controller, business_card),
            /* Expose `deletion_cond` so that `on_delete()` can find it. */
            registration_map_sentry(&registrar->registrations, rid, &deletion_cond),
            /* Begin monitoring the peer so we can disconnect when necessary. */
            peer_monitor(registrar->mailbox_manager, peer),
            waiter(&deletion_cond, &peer_monitor, keepalive.get_drain_signal()) {

            /* Release the mutex, since we're done with our initial setup phase */
            {
                mutex_t::acq_t doomed;
                swap(mutex_acq, doomed);
            }

            /* Wait till it's time to shut down */
            signal_t::subscription_t::reset(&waiter);
        }

        virtual void run() {
            /* Perform the destruction in a coroutine. */
            registrar->registration_destruction_queue.push([this]() { delete this; } );
        }

    private:
        ~active_registration_t() {
            /* The only thing that calls us should be waiter_subscription_t::run(). */
            rassert(waiter.is_pulsed());

            /* Unsubscribe outselves from `waiter`, before we destruct it. */
            signal_t::subscription_t::reset();

            /* Reacquire the mutex, to avoid race conditions when we're
            deregistering from `deleters`. I'm not sure if there re any such race
            conditions, but better safe than sorry. */
            {
                mutex_t::acq_t reacquisition(&registrar->mutex);
                swap(mutex_acq, reacquisition);
            }

            /* `registration_map_sentry` destructor run here; `deletion_cond` cannot
            be pulsed after this. */

            /* `deletion_cond` destructor run here. */

            /* `registrant` destructor run here; this will tell the controller that
            the registration is dead and gone. */

            /* `mutex_acq` destructor run here; it's safe to release the mutex
            because we're no longer touching `updaters` or `deleters`. */
        }

        auto_drainer_t::lock_t keepalive;
        registrar_t *registrar;
        mutex_t::acq_t mutex_acq;
        registrant_type registrant;
        cond_t deletion_cond;
        map_insertion_sentry_t<registration_id_t, cond_t *> registration_map_sentry;
        disconnect_watcher_t peer_monitor;
        wait_any_t waiter;

        DISABLE_COPYING(active_registration_t);
    };

    void on_create(registration_id_t rid, peer_id_t peer, business_card_t business_card, auto_drainer_t::lock_t keepalive) {
        /* Grab the mutex to avoid race conditions if a message arrives at the
        update mailbox or the delete mailbox while we're working. We must not
        block between when `on_create()` begins and when `mutex_acq` is
        constructed. */
        mutex_t::acq_t mutex_acq(&mutex);

        /* If the registrant has already deregistered but the deregistration
        message arrived ahead of the registration message, it will have left a
        NULL in the `registrations` map. */
        typename std::map<registration_id_t, cond_t *>::iterator it = registrations.find(rid);
        if (it != registrations.end()) {
            guarantee(it->second == NULL);
            registrations.erase(it);
            return;
        }

        /* Hand processing over to an active_registration_t. We use a heap
        allocated object to save memory compared to keeping this coroutine
        running for doing the remaining work. active_registration_t takes care
        of its own destruction once the keepalive drain signal is pulsed (or one
        of the other signals it waits on). */
        new active_registration_t(this, std::move(mutex_acq), rid, peer,
                                  business_card, std::move(keepalive));
    }

    void on_delete(registration_id_t rid, UNUSED auto_drainer_t::lock_t keepalive) {

        /* Acquire the mutex so we don't race with `on_create()`. */
        mutex_t::acq_t mutex_acq(&mutex);

        /* Deliver our notification */
        typename std::map<registration_id_t, cond_t *>::iterator it =
            registrations.find(rid);
        if (it != registrations.end()) {
            cond_t *deleter = it->second;
            guarantee(!deleter->is_pulsed());
            deleter->pulse();
        } else {
            /* We got here because the registrant was deleted right after being
            created and the deregistration message arrived before the
            registration message. Insert a NULL into the map so that
            `on_create()` realizes it. */
            cond_t *const zero = 0;
            std::pair<registration_id_t, cond_t *> value(rid, zero);
            registrations.insert(value);
        }
    }

    mailbox_manager_t *mailbox_manager;
    user_data_type controller;

    mutex_t mutex;
    std::map<registration_id_t, cond_t *> registrations;

    unlimited_fifo_queue_t<std::function<void()> > registration_destruction_queue;
    calling_callback_t registration_destruction_callback;
    coro_pool_t<std::function<void()> > registration_destruction_pool;

    auto_drainer_t drainer;

    typename registrar_business_card_t<business_card_t>::create_mailbox_t create_mailbox;
    typename registrar_business_card_t<business_card_t>::delete_mailbox_t delete_mailbox;

    DISABLE_COPYING(registrar_t);
};

#endif /* CLUSTERING_GENERIC_REGISTRAR_HPP_ */
