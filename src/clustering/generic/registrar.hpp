#ifndef CLUSTERING_GENERIC_REGISTRAR_HPP_
#define CLUSTERING_GENERIC_REGISTRAR_HPP_

#include <algorithm>
#include <map>
#include <utility>

#include "clustering/generic/registration_metadata.hpp"
#include "clustering/generic/resource.hpp"
#include "rpc/mailbox/typed.hpp"
#include "concurrency/wait_any.hpp"
#include "concurrency/promise.hpp"

template<class business_card_t, class user_data_type, class registrant_type>
class registrar_t {

public:
    registrar_t(mailbox_manager_t *mm, user_data_type co) :
        mailbox_manager(mm), controller(co),
        create_mailbox(mailbox_manager, boost::bind(&registrar_t::on_create, this, _1, _2, _3, auto_drainer_t::lock_t(&drainer))),
        delete_mailbox(mailbox_manager, boost::bind(&registrar_t::on_delete, this, _1, auto_drainer_t::lock_t(&drainer)))
        { }

    registrar_business_card_t<business_card_t> get_business_card() {
        return registrar_business_card_t<business_card_t>(
            create_mailbox.get_address(),
            delete_mailbox.get_address());
    }

private:
    typedef typename registrar_business_card_t<business_card_t>::registration_id_t registration_id_t;

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

        /* Construct a `registrant_t` to tell the controller that something has
        now registered. */
        registrant_type registrant(controller, business_card);

        /* `registration` is the interface that we expose to the `on_update()`
        and `on_delete()` handlers. */
        cond_t deletion_cond;

        /* Expose `deletion_cond` so that `on_delete()` can find it. */
        map_insertion_sentry_t<registration_id_t, cond_t *> registration_map_sentry(
            &registrations, rid, &deletion_cond);

        /* Begin monitoring the peer so we can disconnect when necessary. */
        disconnect_watcher_t peer_monitor(mailbox_manager->get_connectivity_service(), peer);

        /* Release the mutex, since we're done with our initial setup phase */
        {
            mutex_t::acq_t doomed;
            swap(mutex_acq, doomed);
        }

        /* Wait till it's time to shut down */
        wait_any_t waiter(&deletion_cond, &peer_monitor, keepalive.get_drain_signal());
        waiter.wait_lazily_unordered();

        /* Reacquire the mutex, to avoid race conditions when we're
        deregistering from `deleters`. I'm not sure if there re any such race
        conditions, but better safe than sorry. */
        {
            mutex_t::acq_t reacquisition(&mutex);
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

    void on_delete(registration_id_t rid, UNUSED auto_drainer_t::lock_t keepalive) {

        /* Acquire the mutex so we don't race with `on_create()`. */
        mutex_t::acq_t mutex_acq(&mutex);

        /* Deliver our notification */
        typename std::map<registration_id_t, cond_t *>::iterator it =
            registrations.find(rid);
        if (it != registrations.end()) {
            cond_t *deleter = (*it).second;
            guarantee(!deleter->is_pulsed());
            deleter->pulse();
        } else {
            /* We got here because the registrant was deleted right after being
            created and the deregistration message arrived before the
            registration message. Insert a NULL into the map so that
            `on_create()` realizes it. */
            std::pair<registration_id_t, cond_t *> value(rid, NULL);
            registrations.insert(value);
        }
    }

    mailbox_manager_t *mailbox_manager;
    user_data_type controller;

    mutex_t mutex;
    std::map<registration_id_t, cond_t *> registrations;

    auto_drainer_t drainer;

    typename registrar_business_card_t<business_card_t>::create_mailbox_t create_mailbox;
    typename registrar_business_card_t<business_card_t>::delete_mailbox_t delete_mailbox;

    DISABLE_COPYING(registrar_t);
};

#endif /* CLUSTERING_GENERIC_REGISTRAR_HPP_ */
