#ifndef __CLUSTERING_REGISTRANT_HPP__
#define __CLUSTERING_REGISTRANT_HPP__

#include "clustering/registration_metadata.hpp"
#include "rpc/metadata/metadata.hpp"
#include "rpc/mailbox/typed.hpp"

template<class data_t>
class registrant_t {

public:
    /* This constructor registers with the given registrant. If the registrant
    is inaccessible, it throws an exception. Otherwise, it returns immediately.
    */
    registrant_t(
            mailbox_cluster_t *cl,
            metadata_read_view_t<resource_metadata_t<registrar_metadata_t<data_t> > > *registrar_md,
            data_t initial_value) :
        cluster(cl),
        registrar(cluster, registrar_md),
        registration_id(generate_uuid()),
        manually_deregistered(false)
    {
        /* This will make it so that we get deregistered in our destructor. */
        deregisterer.fun = boost::bind(&registrant_t::send_deregister_message,
            cluster,
            registrar.access().deregister_mailbox,
            registration_id);

        /* Send a message to register us */
        send(cluster, registrar.access().create_mailbox,
            registration_id,
            async_mailbox_t<void()>::address_t(),
            cluster->get_me(),
            initial_value);
    }

    /* This constructor registers with the given registrant. If the registrant
    is inaccessible or something goes wrong in registration, it throws an
    exception. If `interruptor` is pulsed, it throws `interrupted_exc_t`. If all
    goes well, it blocks until the registrant acknowledges it and then returns
    normally. */
    registrant_t(
            mailbox_cluster_t *cl,
            metadata_read_view_t<resource_metadata_t<registrar_metadata_t<data_t> > > *registrar_md,
            data_t initial_value,
            signal_t *interruptor) :
        cluster(cl),
        registrar(cluster, registrar_md),
        registration_id(generate_uuid()),
        manually_deregistered(false)
    {
        /* Create a mailbox to listen for acks */
        cond_t ack_cond;
        async_mailbox_t<void()> mbox(cluster, boost::bind(&cond_t::pulse, &ack_cond));

        /* Set up deregisterer. The reason it's a separate object is so that if
        something goes wrong in the constructor, we'll still get deregistered.
        */
        deregisterer.fun = boost::bind(&registrant_t::send_deregister_message,
            cluster,
            registrar.access().delete_mailbox,
            registration_id);

        /* Send a message to register us */
        send(cluster, registrar.access().create_mailbox,
            registration_id,
            mbox.get_address(),
            cluster->get_me(),
            initial_value);

        /* Wait for reply or interruption */
        wait_any_t waiter(
            interruptor,
            &ack_cond,
            registrar.get_failed_signal());
        waiter.wait_lazily_unordered();

        /* If we got interrupted, throw an exception */
        if (interruptor->is_pulsed()) throw interrupted_exc_t();

        /* If the registrar went offline, throw an exception */
        registrar.access();

        /* It wasn't an interruption, so it must have been a reply */
        rassert(ack_cond.is_pulsed());
    }

    signal_t *get_failed_signal() {
        rassert(!manually_deregistered);
        return registrar.get_failed_signal();
    }

    /* `update()` changes the registration data. It throws an exception if the
    registration is bad. Otherwise, this version of `update()` returns
    immediately. */
    void update(data_t new_data) {

        rassert(!manually_deregistered);

        /* Send a message to change registration */
        send(cluster, registrar.access().update_mailbox,
            registration_id,
            async_mailbox_t<void()>::address_t(),
            new_data);
    }

    /* This version of `update()` is like the previous version except that it
    blocks until either the registrant acknowledges the update or the signal is
    pulsed. If the signal is pulsed, it throws `interrupted_exc_t`. */
    void update(data_t new_data, signal_t *interruptor) {

        rassert(!manually_deregistered);

        /* Set up a mailbox to listen for acks */
        cond_t ack_cond;
        async_mailbox_t<void()> mbox(cluster, boost::bind(&cond_t::pulse, &ack_cond));

        /* Send a message to register us */
        send(cluster, registrar.access().update_mailbox,
            registration_id,
            mbox.get_address(),
            new_data);

        /* Wait for reply or interruption */
        wait_any_t waiter(
            interruptor,
            &ack_cond,
            registrar.get_failed_signal());
        waiter.wait_lazily_unordered();

        /* If we got interrupted, throw an exception */
        if (interruptor->is_pulsed()) throw interrupted_exc_t();

        /* If the registrar went offline, throw an exception */
        registrar.access();

        /* It wasn't an interruption, so it must have been a reply */
        rassert(ack_cond.is_pulsed());
    }

    /* The destructor deregisters us and returns immediately. It never throws
    any exceptions. If we've already been deregistered via `deregister()`, then
    it does nothing. */
    ~registrant_t() {

        /* Most of the work is done by the destructor for `deregisterer` */
    }

    /* `deregister()` deregisters us. If `interruptor` is pulsed, it throws
    `interrupted_exc_t`; otherwise, it blocks until the deregistration is
    complete. It never throws any other exceptions. After you call
    `deregister()`, it's illegal to call any other method. The only reason it
    exists is that you can't overload C++ destructors... */
    void deregister(signal_t *interruptor) {

        /* Setting `manually_deregistered` to `true` makes it so that any other
        method calls will fail. */
        rassert(!manually_deregistered);
        manually_deregistered = true;

        /* Disable `deregisterer` so we don't send a redundant deregistration */
        deregisterer.fun.clear();

        try {
            /* Set up a mailbox to listen for the ack */
            cond_t ack_cond;
            async_mailbox_t<void()> mbox(cluster, boost::bind(&cond_t::pulse, &ack_cond));

            /* Send a message to deregister us */
            send(cluster, registrar.access().delete_mailbox,
                registration_id,
                mbox.get_address());

            /* Wait for a reply or interruption */
            wait_any_t waiter(
                interruptor,
                &ack_cond,
                registrar.get_failed_signal());
            waiter.wait_lazily_unordered();

            /* If we got interrupted, throw an exception */
            if (interruptor->is_pulsed()) throw interrupted_exc_t();

            /* If the registrar went offline, throw an exception. (This is just
            for consistency's sake; we'll catch the exception in just a moment.)
            */
            registrar.access();

            /* If there wasn't an exception, there must have been a reply. */
            rassert(ack_cond.is_pulsed());

        } catch (resource_lost_exc_t) {
            /* Ignore it. We're not supposed to throw any exceptions unless we
            were interrupted. Besides, we're not gonna get any more messages
            anyway. */
        }
    }

private:
    typedef typename registrar_metadata_t<data_t>::registration_id_t registration_id_t;

    /* We can't deregister in our destructor because then we wouldn't get
    deregistered if we died mid-constructor. Instead, the deregistration must be
    done by a separate subcomponent. `deregisterer` is that subcomponent. The
    constructor sets `deregisterer.fun` to a `boost::bind()` of
    `send_deregister_message()`, and that deregisters things as necessary. */
    static void send_deregister_message(
            mailbox_cluster_t *cluster,
            typename registrar_metadata_t<data_t>::delete_mailbox_t::address_t addr,
            registration_id_t rid) {
        send(cluster, addr, rid, async_mailbox_t<void()>::address_t());
    }
    death_runner_t deregisterer;

    mailbox_cluster_t *cluster;
    resource_access_t<registrar_metadata_t<data_t> > registrar;
    registration_id_t registration_id;
    bool manually_deregistered;
};

#endif /* __CLUSTERING_REGISTRANT_HPP__ */
