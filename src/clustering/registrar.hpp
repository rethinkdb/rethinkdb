#ifndef __CLUSTERING_REGISTRAR_HPP__
#define __CLUSTERING_REGISTRAR_HPP__

#include "clustering/registration_metadata.hpp"
#include "clustering/resource.hpp"
#include "rpc/mailbox/typed.hpp"
#include "concurrency/wait_any.hpp"
#include "concurrency/promise.hpp"

/* Each master constructs a `registrar_t` which is responsible for keeping track
of the mirrors associated with that branch. */

template<class protocol_t>
class registrar_t {

private:
    typedef typename registrar_metadata_t<protocol_t>::registration_id_t registration_id_t;

public:
    /* `registrant_t` represents a mirror that's registered with the branch. */
    class registrant_t {

    public:
        /* To send messages to the mirror, call `write()`, `writeread()`, or
        `read()`. It's illegal to call `writeread()` or `read()` if the mirror
        has not yet upgraded to reads. These operations all block until the
        mirror responds. If contact with the mirror is lost, or the mirror
        deregisters instead of responding, the operation will throw a
        `lost_exc_t`. */

        class lost_exc_t : public std::exception {
            const char *what() const throw () {
                return "registrant went offline before acking operation";
            }
        };

        void write(typename protocol_t::write_t write, repli_timestamp_t timestamp, order_token_t tok, signal_t *interruptor) {

            /* Lock the drain semaphore to avoid weird race conditions if we are
            deregistered mid-operation */
            drain_semaphore_t::lock_t dsem_lock(&drain_semaphore);

            /* Set up a mailbox to receive the mirror's acknowledgement */
            cond_t ack_cond;
            async_mailbox_t<void()> ack_mailbox(
                registrar->cluster, boost::bind(&cond_t::pulse, &ack_cond));

            /* Send the write to the mirror */
            send(registrar->cluster, write_mailbox,
                write, timestamp, tok, ack_mailbox.get_address());

            /* Wait for the mirror to reply or for us to be interrupted */
            wait_any_t waiter(&ack_cond, interruptor, &shutdown_cond);
            waiter.wait_lazily_unordered();

            if (interruptor->is_pulsed()) throw interrupted_exc_t();
            if (shutdown_cond.is_pulsed()) throw lost_exc_t();
        }

        typename protocol_t::write_response_t writeread(typename protocol_t::write_t write, repli_timestamp_t timestamp, order_token_t tok, signal_t *interruptor) {

            drain_semaphore_t::lock_t dsem_lock(&drain_semaphore);

            promise_t<typename protocol_t::write_response_t> response_cond;
            async_mailbox_t<void(typename protocol_t::write_response_t)> response_mailbox(
                registrar->cluster, boost::bind(&promise_t<typename protocol_t::write_response_t>::pulse, &response_cond, _1));

            send(registrar->cluster, writeread_mailbox,
                write, timestamp, tok, response_mailbox.get_address());

            wait_any_t waiter(response_cond.get_ready_signal(), interruptor, &shutdown_cond);
            waiter.wait_lazily_unordered();

            if (interruptor->is_pulsed()) throw interrupted_exc_t();
            if (shutdown_cond.is_pulsed()) throw lost_exc_t();

            return response_cond.get_value();
        }

        typename protocol_t::read_response_t read(typename protocol_t::read_t read, order_token_t tok, signal_t *interruptor) {

            drain_semaphore_t::lock_t dsem_lock(&drain_semaphore);

            promise_t<typename protocol_t::read_response_t> response_cond;
            async_mailbox_t<void(typename protocol_t::read_response_t)> response_mailbox(
                registrar->cluster, boost::bind(&promise_t<typename protocol_t::read_response_t>::pulse, &response_cond, _1));

            send(registrar->cluster, read_mailbox,
                read, tok, response_mailbox.get_address());

            wait_any_t waiter(response_cond.get_ready_signal(), interruptor, &shutdown_cond);
            waiter.wait_lazily_unordered();

            if (interruptor->is_pulsed()) throw interrupted_exc_t();
            if (shutdown_cond.is_pulsed()) throw lost_exc_t();

            return response_cond.get_value();
        }

    private:
        friend class registrar_t;

        registrant_t(registrar_t *r, registrar_t::registration_id_t id,
                typename registrar_metadata_t<protocol_t>::write_mailbox_t::address_t waddr) :
            registrar(r), rid(id),
            peer_monitor(registrar->cluster, waddr.get_peer()),
            peer_monitor_subs(boost::bind(&registrant_t::destroy, this)),
            parent_dead_subs(boost::bind(&registrant_t::destroy, this)),
            write_mailbox(waddr), handles_reads(false)
        {
            registrar->registrants[rid] = this;
            peer_monitor_subs.resubscribe(&peer_monitor);
            parent_dead_subs.resubscribe(&registrar->shutdown_cond),
            registrar->callback->on_register(this);
        }

        void upgrade(
            typename registrar_metadata_t<protocol_t>::writeread_mailbox_t::address_t wraddr,
            typename registrar_metadata_t<protocol_t>::read_mailbox_t::address_t raddr)
        {
            /* If we're shutting down, ignore the `upgrade()` because we don't
            want to send an `on_upgrade()` event if we've already sent an
            `on_deregister()` event. */
            if (shutdown_cond.is_pulsed()) return;

            rassert(!handles_reads);
            handles_reads = true;
            writeread_mailbox = wraddr;
            read_mailbox = raddr;

            registrar->callback->on_upgrade_to_reads(this);
        }

        void destroy() {
            /* There are several different reasons that `destroy()` might be
            called. To avoid race conditions, the first thing to call
            `destroy()` pulses `shutdown_cond` and every subsequent thing aborts
            if `shutdown_cond` is already pulsed. */
            if (!shutdown_cond.is_pulsed()) {
                shutdown_cond.pulse();
                delete this;
            }
        }

        ~registrant_t() {
            /* We should only be called from `destroy()`. */
            rassert(shutdown_cond.is_pulsed());

            /* Unregister anything that might try to call `destroy()` again or
            to start a new operation. */
            registrar->callback->on_deregister(this);
            parent_dead_subs.unsubscribe();
            peer_monitor_subs.unsubscribe();
            registrar->registrants.erase(rid);

            /* Wait for any existing read/write operations to notice that
            `shutdown_cond` has been pulsed and to stop */
            drain_semaphore.drain();
        }

        registrar_t *registrar;
        registrar_t::registration_id_t rid;

        /* Watch to see if we lose touch with the registrant */
        disconnect_watcher_t peer_monitor;
        signal_t::subscription_t peer_monitor_subs;

        /* Watch to see if the registrar starts shutting down */
        signal_t::subscription_t parent_dead_subs;

        typename registrar_metadata_t<protocol_t>::write_mailbox_t::address_t write_mailbox;

        bool handles_reads;
        typename registrar_metadata_t<protocol_t>::writeread_mailbox_t::address_t writeread_mailbox;
        typename registrar_metadata_t<protocol_t>::read_mailbox_t::address_t read_mailbox;

        cond_t shutdown_cond;   /* pulsed when we start shutting down */
        drain_semaphore_t drain_semaphore;
    };

    /* To find out what's registering or deregistering, create a `callback_t`
    and pass it to the `registrar_t` constructor. */

    class callback_t {
    public:
        virtual void on_register(registrant_t *) = 0;
        virtual void on_upgrade_to_reads(registrant_t *) = 0;
        virtual void on_deregister(registrant_t *) = 0;
    protected:
        virtual ~callback_t() { }
    };

    registrar_t(
            mailbox_cluster_t *c,
            callback_t *cb,
            metadata_readwrite_view_t<resource_metadata_t<registrar_metadata_t<protocol_t> > > *metadata_view
            ) :
        cluster(c),
        callback(cb),
        register_mailbox(new typename registrar_metadata_t<protocol_t>::register_mailbox_t(
            cluster, boost::bind(&registrar_t::on_register, this, _1, _2, _3))),
        upgrade_mailbox(new typename registrar_metadata_t<protocol_t>::upgrade_mailbox_t(
            cluster, boost::bind(&registrar_t::on_upgrade, this, _1, _2, _3))),
        deregister_mailbox(new typename registrar_metadata_t<protocol_t>::deregister_mailbox_t(
            cluster, boost::bind(&registrar_t::on_deregister, this, _1)))
    {
        registrar_metadata_t<protocol_t> business_card;
        business_card.register_mailbox = register_mailbox->get_address();
        business_card.upgrade_mailbox = upgrade_mailbox->get_address();
        business_card.deregister_mailbox = deregister_mailbox->get_address();
        advertisement.reset(new resource_advertisement_t<registrar_metadata_t<protocol_t> >(
            cluster, metadata_view, business_card));
    }

    ~registrar_t() {

        /* Stop advertising so people know we're shutting down */
        advertisement.reset();

        /* Don't allow further registration/deregistration operations to begin
        */
        register_mailbox.reset();
        upgrade_mailbox.reset();
        deregister_mailbox.reset();

        /* Start stopping existing registrations */
        shutdown_cond.pulse();

        /* Wait for existing registrations to stop completely */
        drain_semaphore.drain();
        rassert(registrants.empty());
    }

private:
    mailbox_cluster_t *cluster;

    callback_t *callback;

    cond_t shutdown_cond;
    drain_semaphore_t drain_semaphore;
    std::map<registration_id_t, registrant_t *> registrants;

    boost::scoped_ptr<typename registrar_metadata_t<protocol_t>::register_mailbox_t> register_mailbox;
    boost::scoped_ptr<typename registrar_metadata_t<protocol_t>::upgrade_mailbox_t> upgrade_mailbox;
    boost::scoped_ptr<typename registrar_metadata_t<protocol_t>::deregister_mailbox_t> deregister_mailbox;

    void on_register(registration_id_t rid, async_mailbox_t<void()>::address_t ack_addr, typename registrar_metadata_t<protocol_t>::write_mailbox_t::address_t waddr) {
        registrants[rid] = new registrant_t(this, rid, waddr);
        send(cluster, ack_addr);
    }

    void on_upgrade(registration_id_t rid, typename registrar_metadata_t<protocol_t>::writeread_mailbox_t::address_t wraddr, typename registrar_metadata_t<protocol_t>::read_mailbox_t::address_t raddr) {
        typename std::map<registration_id_t, registrant_t *>::iterator it = registrants.find(rid);
        if (it != registrants.end()) {
            (*it).second->upgrade(wraddr, raddr);
        }
    }

    void on_deregister(registration_id_t rid) {
        typename std::map<registration_id_t, registrant_t *>::iterator it = registrants.find(rid);
        if (it != registrants.end()) {
            (*it).second->destroy();
        }
    }

    boost::scoped_ptr<resource_advertisement_t<registrar_metadata_t<protocol_t> > > advertisement;
};

#endif /* __CLUSTERING_REGISTRAR_HPP__ */
