#ifndef __CLUSTERING_REGISTRAR_HPP__
#define __CLUSTERING_REGISTRAR_HPP__

#include "clustering/registration_metadata.hpp"
#include "clustering/resource.hpp"
#include "rpc/mailbox/typed.hpp"
#include "concurrency/wait_any.hpp"
#include "concurrency/promise.hpp"

template<class data_t, class controller_t, class registrant_t>
class registrar_t {

private:
    typedef typename registrar_metadata_t<data_t>::registration_id_t registration_id_t;

public:
    registrar_t(
            mailbox_cluster_t *cl,
            controller_t co,
            metadata_readwrite_view_t<resource_metadata_t<registrar_metadata_t<data_t> > > *metadata_view
            ) :
        cluster(cl), controller(co)
    {
        drain_semaphore_t::lock_t keepalive(&drain_semaphore);
        create_mailbox.reset(new typename registrar_metadata_t<data_t>::create_mailbox_t(
            cluster, boost::bind(&registrar_t::on_create, this, _1, _2, _3, _4, keepalive)));
        update_mailbox.reset(new typename registrar_metadata_t<data_t>::update_mailbox_t(
            cluster, boost::bind(&registrar_t::on_update, this, _1, _2, _3, keepalive)));
        delete_mailbox.reset(new typename registrar_metadata_t<data_t>::delete_mailbox_t(
            cluster, boost::bind(&registrar_t::on_delete, this, _1, _2, keepalive)));

        registrar_metadata_t<data_t> business_card;
        business_card.create_mailbox = create_mailbox->get_address();
        business_card.update_mailbox = update_mailbox->get_address();
        business_card.delete_mailbox = delete_mailbox->get_address();
        advertisement.reset(new resource_advertisement_t<registrar_metadata_t<data_t> >(
            cluster, metadata_view, business_card));
    }

    ~registrar_t() {

        /* Stop advertising so people know we're shutting down */
        advertisement.reset();

        /* Don't allow further registration/deregistration operations to begin
        */
        create_mailbox.reset();
        update_mailbox.reset();
        delete_mailbox.reset();

        /* Start stopping existing registrations */
        shutdown_cond.pulse();

        /* Wait for existing registrations to stop completely */
        drain_semaphore.drain();

        rassert(registrations.empty());
    }

private:
    void on_create(registration_id_t rid, async_mailbox_t<void()>::address_t ack_addr, peer_id_t peer, data_t data, UNUSED drain_semaphore_t::lock_t keepalive) {

        /* Grab the mutex to avoid race conditions if a message arrives at the
        update mailbox or the delete mailbox while we're working. We must not
        block between when `on_create()` begins and when `mutex_acq` is
        constructed. */
        mutex_acquisition_t mutex_acq(&mutex);

        /* `registration` is the interface that we expose to the `on_update()`
        and `on_delete()` handlers. */
        registration_t registration;
        cond_t deletion_cond;
        registration.deleter = &deletion_cond;

        {
            /* Construct a `registrant_t` to tell the controller that something has
            now registered. */
            registrant_t registrant(controller, data);
            registration.updater = boost::bind(&registrant_t::update, &registrant, _1);

            /* Expose `registrant` so that `on_update()` and `on_delete()` can
            find it. */
            map_insertion_sentry_t<registration_id_t, registration_t *> registration_map_sentry(
                &registrations, rid, &registration);

            /* Begin monitoring the peer so we can disconnect when necessary. */
            disconnect_watcher_t peer_monitor(cluster, peer);

            /* Release the mutex, since we're done with our initial setup phase
            */
            {
                mutex_acquisition_t doomed;
                swap(mutex_acq, doomed);
            }

            /* Send an ack if one was requested */
            if (!ack_addr.is_nil()) {
                send(cluster, ack_addr);
            }

            /* Wait till it's time to shut down */
            wait_any_t waiter(&deletion_cond, &peer_monitor, &shutdown_cond);
            waiter.wait_lazily_unordered();

            /* Reacquire the mutex, to avoid race conditions when we're
            deregistering from `updaters` and `deleters`. I'm not sure if there
            are any such race conditions, but better safe than sorry. */
            {
                mutex_acquisition_t reacquisition(&mutex);
                swap(mutex_acq, reacquisition);
            }

            /* `registration_map_sentry` destructor run here; `deletion_cond`
            cannot be pulsed after this, and `updater` cannot be called. */

            /* `registrant` destructor run here; this will tell the controller
            that the registration is dead and gone. */
        }

        /* If the thing that deregistered us requested an ack, it will have set
        `registration.on_deleted_callback` to a non-nil function. We must wait
        until after `registrant` has been destroyed to do this. */
        if (registration.on_deleted_callback) {
            rassert(deletion_cond.is_pulsed(), "Something set the "
                "`on_deleted_callback` but didn't tell us to delete ourself");
            registration.on_deleted_callback();
        }

        /* `mutex_acq` destructor run here; it's safe to release the mutex
        because we're no longer touching `updaters` or `deleters`. */
    }

    void on_update(registration_id_t rid, async_mailbox_t<void()>::address_t ack_addr, data_t new_value, UNUSED drain_semaphore_t::lock_t keepalive) {

        {
            /* Acquire the mutex so we don't race with `on_create()`. */
            mutex_acquisition_t mutex_acq(&mutex);

            /* Deliver our notification */
            typename std::map<registration_id_t, registration_t *>::iterator it =
                registrations.find(rid);
            if (it != registrations.end()) {
                registration_t *reg = (*it).second;
                reg->updater(new_value);
            }

            /* Mutex is released here */
        }

        /* Deliver an ack if one was requested */
        if (!ack_addr.is_nil()) {
            send(cluster, ack_addr);
        }
    }

    void on_delete(registration_id_t rid, async_mailbox_t<void()>::address_t ack_addr, UNUSED drain_semaphore_t::lock_t keepalive) {

        if (ack_addr.is_nil()) {

            /* No acknowledgement has been requested */

            /* Acquire the mutex so we don't race with `on_create()`. */
            mutex_acquisition_t mutex_acq(&mutex);

            /* Deliver our notification */
            typename std::map<registration_id_t, registration_t *>::iterator it =
                registrations.find(rid);
            if (it != registrations.end()) {
                registration_t *reg = (*it).second;
                rassert(!reg->deleter->is_pulsed());
                reg->deleter->pulse();
            }

        } else {

            /* An acknowledgement has been requested; we need some extra
            machinery to wait until the registrant has been shut down completely
            before sending the ack. */

            cond_t deletion_complete_cond;

            {
                /* Acquire the mutex so we don't race with `on_create()`. */
                mutex_acquisition_t mutex_acq(&mutex);

                /* Deliver our notification */
                typename std::map<registration_id_t, registration_t *>::iterator it =
                    registrations.find(rid);
                if (it != registrations.end()) {
                    registration_t *reg = (*it).second;
                    reg->on_deleted_callback = boost::bind(&cond_t::pulse, &deletion_complete_cond);
                    rassert(!reg->deleter->is_pulsed());
                    reg->deleter->pulse();
                }
            }

            /* Wait until after the `registrant_t` destructor has actually been
            run */
            deletion_complete_cond.wait_lazily_unordered();

            send(cluster, ack_addr);
        }
    }

    mailbox_cluster_t *cluster;
    controller_t controller;

    boost::scoped_ptr<typename registrar_metadata_t<data_t>::create_mailbox_t> create_mailbox;
    boost::scoped_ptr<typename registrar_metadata_t<data_t>::update_mailbox_t> update_mailbox;
    boost::scoped_ptr<typename registrar_metadata_t<data_t>::delete_mailbox_t> delete_mailbox;

    boost::scoped_ptr<resource_advertisement_t<registrar_metadata_t<data_t> > > advertisement;

    cond_t shutdown_cond;
    drain_semaphore_t drain_semaphore;

    mutex_t mutex;
    class registration_t {
    public:
        boost::function<void(data_t)> updater;
        cond_t *deleter;
        boost::function<void()> on_deleted_callback;
    };
    std::map<registration_id_t, registration_t *> registrations;
};

#endif /* __CLUSTERING_REGISTRAR_HPP__ */
