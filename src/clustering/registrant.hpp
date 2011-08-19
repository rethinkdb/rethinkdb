#ifndef __CLUSTERING_REGISTRANT_HPP__
#define __CLUSTERING_REGISTRANT_HPP__

#include "clustering/registration.hpp"
#include "rpc/metadata/metadata.hpp"
#include "rpc/mailbox/typed.hpp"

/* `registrant_t` represents registering to receive write operations from a
master.

Construct a `registrant_t` to start receiving write operations from a master.
The constructor will block while it tries to contact the master. If you
interrupt it, it will throw an `interrupted_exc_t`; otherwise, it will return
normally even if it failed to contact the master.

To find out if you are currently receiving write operations from the master,
check the status of the `signal_t*` returned by `get_failed_signal()`. This is
implemented as a `signal_t*` rather than an exception thrown by the constructor
because something may go wrong after the constructor is done.

The write operations that are received will be passed to the function that you
pass to the `registrant_t` constructor.

Call `upgrade_to_reads()` to inform the master that you're ready to handle reads
as well as writes. It will always return immediately and will not throw an
exception.

To stop receiving write operations, call `~registrant_t()`. This will send a
message to the master to deregister and then return immediately. */

template<class protocol_t>
struct registrant_t {

    registrant_t(

            mailbox_cluster_t *cl,

            /* Which branch to register with */
            metadata_read_view_t<resource_metadata_t<registrar_metadata_t<protocol_t> > > *registar_md,

            /* Whenever a write operation is received, this function will be
            called in a separate coroutine. Calling the final argument will send
            a response back to the master. */
            boost::function<void(
                typename protocol_t::write_t,
                repli_timestamp_t,
                order_token_t,
                boost::function<void()>
                )> write_cb,

            /* If this is pulsed, cancel the registration process */
            signal_t *interruptor) :

        cluster(cl),
        registrar(cluster, registar_md),

        registration_id(generate_uuid()),

        is_upgraded(false),

        write_mailbox(cluster, boost::bind(&registration_t::on_write, this, _1, _2, _3, _4)),
        writeread_mailbox(cluster, boost::bind(&registration_t::on_writeread, this, _1, _2, _3, _4)),
        read_mailbox(cluster, boost::bind(&registration_t::on_read, this, _1, _2, _3))

    {
        if (interruptor->is_pulsed()) throw interrupted_exc_t();

        try {
            write_callback = write_cb;

            /* Set up a mailbox to receive the master's acknowledgement */
            cond_t master_has_noticed_us;
            realtime_registration_t<protocol_t>::greet_mailbox_t greet_mailbox(
                cluster,
                boost::bind(&cond_t::pulse, &master_has_noticed_us));

            /* Send a message to the master to register us */
            send(cluster, registrar.access().write_register_mailbox,
                registration_id, &greet_mailbox, &write_mailbox);

            /* Wait for the master to acknowledge us, or for something to go
            wrong */
            wait_any_lazily_unordered(&master_has_noticed_us,
                registrar.get_failed_signal(), interruptor);

            /* This will throw an exception if the registrar is down */
            registrar.access();

            if (interruptor->is_pulsed()) throw interrupted_exc_t();

            rassert(master_has_noticed_us.is_pulsed());

        } catch (resource_lost_exc_t) {
            /* We get here if we lost contact with the registrar before the
            registration process completed. We're not supposed to throw an
            exception from the constructor, even if we didn't register
            properly, so we just return. */
            return;
        }
    }

    signal_t *get_failed_signal() {
        return registrar.get_failed_signal();
    }

    void upgrade_to_reads(

            /* Whenever a write operation is received that the master wants us
            to compute the reply to, this function will be called in a separate
            coroutine. Calling the final argument will send a response back to
            the master. */
            boost::function<void(
                typename protocol_t::write_t,
                repli_timestamp_t, order_token_t,
                boost::function<void(typename protocol_t::write_response_t)>
                )> writeread_cb,

            /* Whenever a read operation is received, this function will be
            called in a separate coroutine. Calling the final argument will send
            a response back to the master. */
            boost::function<void(
                typename protocol_t::read_t,
                order_token_t,
                boost::function<void(typename protocol_t::read_response_t)>
                )> read_cb
        )
    {
        rassert(!is_upgraded);
        is_upgraded = true;

        try {
            writeread_callback = writeread_cb;
            read_callback = read_cb;
            /* Send the master a message to sign us up for writereads and reads.
            */
            send(cluster,
                branch_metadata->get_metadata().upgrade_mailbox,
                registration_id,
                &writeread_mailbox,
                &read_mailbox);
        } catch (resource_lost_exc_t) {
            /* The registrar is dead. Ignore it, since that's what we're
            supposed to do. The user will find out by checking
            `get_failed_signal()`. */
        }
    }

    ~registrant_t() {

        try {
            /* Send the master a message saying we no longer want writes. */
            send(cluster,
                registrar.access().deregister_mailbox,
                registration_id);
        } catch (resource_lost_exc_t) {
            /* There's no need to deregister since the registrar is dead
            anyway */
            return;
        }
    }

private:
    void on_write(
        typename protocol_t::write_t write,
        repli_timestamp_t timestamp,
        order_token_t otok,
        async_mailbox_t<void()>::address_t cont
        )
    {
        write_callback(write, timestamp, otok,
            boost::bind(&send, cluster, cont));
    }

    void on_writeread(
        typename protocol_t::write_t write,
        repli_timestamp_t timestamp,
        order_token_t otok,
        async_mailbox_t<void(typename protocol_t::write_response_t)>::address_t cont
        )
    {
        writeread_callback(write, timestamp, otok,
            boost::bind(&send, cluster, cont, _1));
    }

    void on_read(
        typename protocol_t::read_t read,
        order_token_t otok,
        async_mailbox_t<void(typename protocol_t::read_response_t)>::address_t cont
        )
    {
        read_callback(read, otok,
            boost::bind(&send, cluster, cont, _1));
    }

    mailbox_cluster_t *cluster;

    resource_access_t<registrar_metadata_t<protocol_t> > registrar;

    registrar_metadata_t<protocol_t>::registration_id_t registration_id;

    bool is_upgraded;

    registrar_metadata_t<protocol_t>::write_mailbox_t write_mailbox;
    registrar_metadata_t<protocol_t>::writeread_mailbox_t writeread_mailbox;
    registrar_metadata_t<protocol_t>::read_mailbox_t read_mailbox;

    boost::function<void(
        typename protocol_t::write_t,
        repli_timestamp_t,
        order_token_t,
        boost::function<void()>
        )> write_callback;
    boost::function<void(
        typename protocol_t::write_t,
        repli_timestamp_t,
        order_token_t,
        boost::function<void(typename protocol_t::write_response_t)>
        )> writeread_callback;
    boost::function<void(
        typename protocol_t::read_t,
        order_token_t,
        boost::function<void(typename protocol_t::read_response_t)>
        )> read_callback;
};

#endif /* __CLUSTERING_REGISTRANT_HPP__ */
