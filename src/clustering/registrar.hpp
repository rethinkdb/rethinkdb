#ifndef __CLUSTERING_REGISTRAR_HPP__
#define __CLUSTERING_REGISTRAR_HPP__

/* Each master constructs a `registrar_t` which is responsible for keeping track
of the mirrors associated with that branch. */

template<class protocol_t>
class registrar_t {

private:
    typedef registrar_metadata_t<protocol_t>::registration_id_t registration_id_t;

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

            cond_t ack_cond;
            async_mailbox_t<boost::function<void()> > ack_mailbox(
                registrar->cluster, boost::bind(&cond_t::pulse, &ack_cond));

            send(registrar->cluster, write_mailbox,
                write, timestamp, tok, &ack_mailbox);

            wait_any_lazily_unordered(&acked_cond, interruptor, &lost_cond);

            if (interruptor->is_pulsed()) throw interrupted_exc_t();
            if (lost_cond.is_pulsed()) throw lost_exc_t();
        }

        typename protocol_t::write_response_t writeread(typename protocol_t::write_t write, repli_timestamp_t timestamp, order_token_t tok, signal_t *interruptor) {

            drain_semaphore_t::lock_t dsem_lock(&drain_semaphore);

            promise_t<typename protocol_t::write_response_t> response_cond;
            async_mailbox_t<boost::function<void(typename protocol_t::write_response_t)> > response_mailbox(
                registrar->cluster, boost::bind(&promise_t<typename protocol_t::write_response_t>::pulse, &response_cond));

            send(registrar->cluster, writeread_mailbox,
                write, timestamp, tok, &response_mailbox);

            wait_any_lazily_unordered(response_cond.get_signal(), interruptor, &lost_cond);

            if (interruptor->is_pulsed()) throw interrupted_exc_t();
            if (lost_cond.is_pulsed()) throw lost_exc_t();

            return response_cond.get_value();
        }

        typename protocol_t::read_response_t read(typename protocol_t::read_t read, order_token_t tok, signal_t *interruptor) {

            drain_semaphore_t::lock_t dsem_lock(&drain_semaphore);

            promise_t<typename protocol_t::read_response_t> response_cond;
            async_mailbox_t<boost::function<void(typename protocol_t::read_response_t)> > response_mailbox(
                registrar->cluster, boost::bind(&promise_t<typename protocol_t::read_response_t>::pulse, &response_cond));

            send(registrar->cluster, read_mailbox,
                read, tok, &response_mailbox);

            wait_any_lazily_unordered(response_cond.get_signal(), interruptor, &lost_cond);

            if (interruptor->is_pulsed()) throw interrupted_exc_t();
            if (lost_cond.is_pulsed()) throw lost_exc_t();

            return response_cond.get_value();
        }

    private:
        friend class registrar_t;

        registrant_t(registrar_t *r, registrar_metadata_t<protocol_t>::write_mailbox_t::address_t waddr) :
            registrar(r), peer_monitor(registrar->cluster, waddr.get_peer()),
            peer_monitor_subs(boost::bind(&registrant_t::on_disconnect, this), &peer_monitor),
            write_mailbox(waddr), handles_reads(false) { }

        void upgrade(registrar_metadata_t<protocol_t>::writeread_mailbox_t::address_t wraddr,
            registrar_metadata_t<protocol_t>::read_mailbox_t::address_t raddr)
        {
            rassert(!handles_reads);
            handles_reads = true;
            writeread_mailbox = wraddr;
            read_mailbox = raddr;
        }

        void on_disconnect() {
            drain_semaphore_t::lock_t keepalive(&drain_semaphore);
            coro_t::spawn(boost::bind(&registrar_t::on_deregister, this, keepalive));
        }

        ~registrant_t() {
            lost_cond.pulse();
            drain_semaphore.drain();
        }

        registrar_t *registrar;
        registration_id_t registration_id;

        disconnect_watcher_t peer_monitor;
        signal_t::subscription_t peer_monitor_subs;

        registrar_metadata_t<protocol_t>::write_mailbox_t::address_t write_mailbox;

        bool handles_reads;
        registrar_metadata_t<protocol_t>::writeread_mailbox_t::address_t writeread_mailbox;
        registrar_metadata_t<protocol_t>::read_mailbox_t::address_t read_mailbox;

        cond_t lost_cond;   /* pulsed when we start shutting down */
        drain_semaphore_t drain_semaphore;
    };

    /* To find out what's registering or deregistering, create a `callback_t`
    and pass it to the `registrar_t` constructor. */

    class callback_t {
    public:
        virtual void on_register(registrant_t *) = 0;
        virtual void on_upgrade_to_reads(registrant_t *) = 0;
        virtual void on_deregister(registrant_t *) = 0;
    };

    registrar_t(mailbox_cluster_t *c, callback_t *cb) :
        cluster(c),
        callback(cb),
        register_mailbox(cluster, boost::bind(&registrar_t::on_register, _1, _2)),
        upgrade_mailbox(cluster, boost::bind(&registrar_t::on_upgrade, _1, _2, _3)),
        deregister_mailbox(cluster, boost::bind(&registrar_t::on_deregister, _1))
        { }

    registrar_metadata_t<protocol_t> get_business_card() {
        registrar_metadata_t<protocol_t> business_card;
        business_card.register_mailbox = &register_mailbox;
        business_card.upgrade_mailbox = &upgrade_mailbox;
        business_card.deregister_mailbox = &deregister_mailbox;
        return business_card;
    }

private:
    mailbox_cluster_t *cluster;

    callback_t *callback;

    mutex_t registrants_lock;

    /* `registrants` must be deconstructed after the mailboxes are
    deconstructed, or else a message might try to access a registrant as
    `registrants` was being destroyed. */
    boost::ptr_map<registration_id_t, registrant_t> registrants;

    registrar_metadata_t<protocol_t>::register_mailbox_t register_mailbox;
    registrar_metadata_t<protocol_t>::upgrade_mailbox_t upgrade_mailbox;
    registrar_metadata_t<protocol_t>::deregister_mailbox_t deregister_mailbox;

    void on_register(registration_id_t rid, registrar_metadata_t<protocol_t>::write_mailbox_t::address_t waddr) {
        mutex_acquisition_t acq(&registrants_lock);
        registrant_t *registrant = new registrant_t(this, rid, waddr);
        rassert(registrants.count(rid) == 0);
        registrants[rid] = registrant;
        callback->on_register(registrant);
    }

    void on_upgrade(registrant_id_t rid, registrar_metadata_t<protocol_t>::writeread_mailbox_t::address_t wraddr, registrar_metadata_t<protocol_t>::read_mailbox_t::address_t raddr) {
        mutex_acquisition_t acq(&registrants_lock);
        rassert(registrants.count(rid) != 0);
        rassert(it != registrants.end());
        registrant_t *registrant = registrants[rid];
        registrant->upgrade(wraddr, raddr);
        callback->on_upgrade(registrant);
    }

    void on_deregister(registrant_id_t rid) {
        mutex_acquisition_t acq(&registrants_lock);
        boost::ptr_map<registration_id_t, registrant_t>::iterator it = registrants.find(rid);
        if (it != registrants.end()) {
            callback->on_deregister((*it).second);
            registrants.erase(it);
        }
    }
};

#endif /* __CLUSTERING_REGISTRAR_HPP__ */
