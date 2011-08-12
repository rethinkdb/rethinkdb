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
as well as writes.

To stop receiving write operations, call `~registrant_t()`. This will send a
message to the master to deregister and then return immediately. */

template<class protocol_t>
struct registrant_t {

    registrant_t(

            mailbox_cluster_t *cl,

            /* Which branch to register with */
            metadata_view_t<registration_metadata_t<protocol_t> > *bmd,

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
        branch_metadata(bmd),

        registration_id(generate_uuid()),

        is_upgraded(false),

        write_mailbox(cluster, boost::bind(&registration_t::on_write, this, _1, _2, _3, _4)),
        writeread_mailbox(cluster, boost::bind(&registration_t::on_writeread, this, _1, _2, _3, _4)),
        read_mailbox(cluster, boost::bind(&registration_t::on_read, this, _1, _2, _3))

    {
        write_callback = write_cb;

        if (interruptor->is_pulsed()) throw interrupted_exc_t();

        /* Start monitoring the master we're joining. */

        master_is_offline.reset(new branch_death_watcher_t<protocol_t>(branch_metadata));
        fail_if_master_is_offline.reset(new cond_link_t(master_is_offline.get(), &failed_cond));

        if (failed_cond.is_pulsed()) return;

        master_is_dead.reset(new disconnect_watcher_t<protocol_t>(
            branch_metadata->get_metadata().read_mailbox.get_peer()));
        fail_if_master_is_dead.reset(new cond_link_t(master_is_dead.get(), &failed_cond));

        if (failed_cond.is_pulsed()) return;

        /* Set up a mailbox to receive the master's acknowledgement */
        cond_t master_has_noticed_us;
        realtime_registration_t<protocol_t>::greet_mailbox_t greet_mailbox(
            cluster,
            boost::bind(&cond_t::pulse, &master_has_noticed_us));

        /* Check before we send because if the master is offline, `join_mailbox`
        will have been set to nil and we'll crash. */
        if (failed_cond.is_pulsed()) return;

        /* Send a message to the master to register us */
        send(cluster, branch_metadata->get_metadata().write_register_mailbox,
            registration_id, &greet_mailbox, &write_mailbox);

        /* Wait for the master to acknowledge us, or for something to go wrong
        */

        cond_t c;
        cond_link_t continue_when_master_notices_us(&master_has_noticed_us, &c);
        cond_link_t continue_when_master_fails(&failed_cond, &c);
        cond_link_t continue_when_interrupted(interruptor, &c);
        c.wait_lazily_unordered();

        if (interruptor->is_pulsed()) throw interrupted_exc_t();

        if (failed_cond.is_pulsed()) return;
        rassert(master_has_noticed_us.is_pulsed());
    }

    signal_t *get_failed_signal() {
        return &failed_cond;
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

        /* If the master went offline or we lost contact, there's nothing we can
        do. */
        if (failed_cond.is_pulsed()) return;

        writeread_callback = writeread_cb;
        read_callback = read_cb;

        /* Send the master a message to sign us up for writereads and reads. */
        send(cluster,
            branch_metadata->get_metadata().upgrade_mailbox,
            registration_id,
            &writeread_mailbox,
            &read_mailbox);
    }

    ~registrant_t() {

        /* If the master went offline or we lost contact, then the master won't
        be sending us any more writes anyway. */
        if (failed_cond.is_pulsed()) return;

        /* Send the master a message saying we no longer want writes. */
        send(cluster,
            branch_metadata->get_metadata().deregister_mailbox,
            registration_id);
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

    metadata_view_t<branch_metadata_t<protocol_t> > *branch_metadata;

    cond_t failed_cond;

    boost::scoped_ptr<branch_death_watcher_t<protocol_t> > master_is_offline;
    boost::scoped_ptr<cond_link_t> fail_if_master_is_offline;
    boost::scoped_ptr<disconnect_watcher_t> master_is_dead;
    boost::scoped_ptr<cond_link_t> fail_if_master_is_dead;

    realtime_registration_id_t registration_id;

    bool is_upgraded;

    registration_metadata_t<protocol_t>::write_mailbox_t write_mailbox;
    registration_metadata_t<protocol_t>::writeread_mailbox_t writeread_mailbox;
    registration_metadata_t<protocol_t>::read_mailbox_t read_mailbox;

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
