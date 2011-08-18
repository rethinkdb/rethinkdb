#ifndef __CLUSTERING_REGISTRAR_HPP__
#define __CLUSTERING_REGISTRAR_HPP__

/* Each master constructs a `registrar_t` which is responsible for keeping track
of the mirrors associated with that branch. */

template<class protocol_t>
class registrar_t {

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

            cond_t c;
            cond_link_t continue_if_acked(&ack_cond, &c);
            cond_link_t continue_if_interrupted(interruptor, &c);
            cond_link_t continue_if_lost(&lost_cond, &c);
            c.wait_lazily_unordered();

            if (interruptor->is_pulsed()) {
                throw interrupted_exc_t();
            }

            if (lost_cond.is_pulsed()) {
                throw lost_exc_t();
            }
        }

        typename protocol_t::write_response_t writeread(typename protocol_t::write_t write, repli_timestamp_t timestamp, order_token_t tok, signal_t *interruptor) {

            drain_semaphore_t::lock_t dsem_lock(&drain_semaphore);

            promise_t<typename protocol_t::write_response_t> response_cond;
            async_mailbox_t<boost::function<void(typename protocol_t::write_response_t)> > response_mailbox(
                registrar->cluster, boost::bind(&promise_t<typename protocol_t::write_response_t>::pulse, &response_cond));

            send(registrar->cluster, writeread_mailbox,
                write, timestamp, tok, &response_mailbox);

            cond_t c;
            cond_link_t continue_if_acked(response_cond.get_signal(), c);
            cond_link_t continue_if_interrupted
        }

        typename protocol_t::read_response_t read(typename protocol_t::read_t read, order_token_t tok, signal_t *interruptor) {
        }

    private:
        registrar_t *registrar;
        registrar_metadata_t<protocol_t>::write_mailbox_t::address_t write_mailbox;
        registrar_metadata_t<protocol_t>::writeread_mailbox_t::address_t writeread_mailbox;
        registrar_metadata_t<protocol_t>::read_mailbox_t::address_t read_mailbox;
        drain_semaphore_t drain_semaphore;
    };

    class callback_t {
    public:
        virtual void on_register(registrant_t *) = 0;
        virtual void on_upgrade_to_reads(registrant_t *) = 0;
        virtual void on_deregister(registrant_t *) = 0;
    };

    registrar_t(mailbox_cluster_t *cluster, callback_t *cb) :
        register_mailbox(cluster, boost::bind(&registrar_t::on_register, _1, _2)),
        upgrade_mailbox(cluster, boost::bind(&registrar_t::on_upgrade, _1, _2, _3)),
        deregister_mailbox(cluster, boost::bind(&registrar_t::on_deregister, _1)),
        callback(cb)
        { }

    registrar_metadata_t<protocol_t> get_business_card() {
        registrar_metadata_t<protocol_t> business_card;
        business_card.register_mailbox = &register_mailbox;
        business_card.upgrade_mailbox = &upgrade_mailbox;
        business_card.deregister_mailbox = &deregister_mailbox;
        return business_card;
    }

private:
    registrar_metadata_t<protocol_t>::register_mailbox_t register_mailbox;
    registrar_metadata_t<protocol_t>::upgrade_mailbox_t upgrade_mailbox;
    registrar_metadata_t<protocol_t>::deregister_mailbox_t deregister_mailbox;

    callback_t *callback;
};

#endif /* __CLUSTERING_REGISTRAR_HPP__ */
