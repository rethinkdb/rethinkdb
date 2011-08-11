/* `mirror_write_registration_t` represents registering to receive write
operations from a master.

Construct a `mirror_write_registration_t` to start receiving write operations
from a master. The constructor will block while it tries to contact the master.
If you interrupt it, it will throw an `interrupted_exc_t`; otherwise, it will
return normally even if it failed to contact the master.

To find out if you are currently receiving write operations from the master,
check the status of the `signal_t*` returned by `get_failed_signal()`. This is
implemented as a `signal_t*` rather than an exception thrown by the constructor
because something may go wrong after the constructor is done.

The write operations that are received will be passed to the function that you
pass to the `mirror_write_registration_t` constructor.

To stop receiving write operations, call `~mirror_write_registration_t()`. This
will send a message to the master to deregister and then return immediately. */

template<class protocol_t>
struct mirror_write_registration_t {

    mirror_write_registration_t(

            mailbox_cluster_t *cl,

            /* Which branch to register with */
            metadata_view_t<branch_metadata_t<protocol_t> > *bmd,

            /* Whenever a write operation is received, this function will be
            called in a separate coroutine. Calling the final argument will send
            a response back to the master. */
            boost::function<void(
                typename protocol_t::write_t,
                repli_timestamp_t,
                order_token_t,
                boost::function<void(boost::optional<typename protocol_t::write_response_t>)>
                )> fun,

            /* If this is pulsed, cancel the registration process */
            signal_t *interruptor) :

        cluster(cl),
        branch_metadata(bmd),

        write_mailbox(cluster, boost::bind(&mirror_write_registration_t::on_write, this, _1, _2, _3, _4)),

        callback(fun)
    {

        if (interruptor->is_pulsed()) throw interrupted_exc_t();

        /* Start monitoring the master we're joining. */

        master_is_offline.reset(new branch_death_watcher_t<protocol_t>(branch_metadata));
        fail_if_master_is_offline.reset(new cond_link_t(master_is_offline.get(), &failed_cond));

        if (failed_cond.is_pulsed()) return;

        master_is_dead.reset(new disconnect_watcher_t<protocol_t>(
            branch_metadata->get_metadata().read_mailbox.get_peer()));
        fail_if_master_is_dead.reset(new cond_link_t(master_is_dead.get(), &failed_cond));

        if (failed_cond.is_pulsed()) return;

        /* Generate a registration ID to identify our registration with the
        master */
        registration_id = generate_uuid();

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

    ~mirror_write_registration_t() {

        /* If the master went offline or we lost contact, then the master won't
        be sending us any more writes anyway. */
        if (failed_cond.is_pulsed()) return;

        /* Send the master a message saying we no longer want writes. */
        send(cluster,
            branch_metadata->get_metadata().write_unregister_mailbox,
            registration_id);
    }

private:
    void on_write(
        typename protocol_t::write_t write,
        repli_timestamp_t timestamp,
        order_token_t otok,
        async_mailbox_t<void(boost::optional<typename protocol_t::write_response_t>)>::address_t cont
        )
    {
        callback(write, timestamp, otok,
            boost::bind(send, cluster, cont, _1));
    }

    mailbox_cluster_t *cluster;

    metadata_view_t<branch_metadata_t<protocol_t> > *branch_metadata;

    cond_t failed_cond;

    boost::scoped_ptr<branch_death_watcher_t<protocol_t> > master_is_offline;
    boost::scoped_ptr<cond_link_t> fail_if_master_is_offline;
    boost::scoped_ptr<disconnect_watcher_t> master_is_dead;
    boost::scoped_ptr<cond_link_t> fail_if_master_is_dead;

    write_registration_id_t registration_id;

    realtime_registration_t<protocol_t>::write_mailbox_t write_mailbox;

    boost::function<void(
        typename protocol_t::write_t,
        repli_timestamp_t,
        order_token_t,
        boost::function<void(boost::optional<typename protocol_t::write_response_t>)>
        )> callback;
};

/* `mirror_read_registration_t` represents registering to receive realtime read
operations from the master. It's only legal to register for realtime read
operations if you're already registered for write operations, because realtime
read operations are supposed to guarantee ordering with respect to writes.

Construct a `mirror_read_registration_t` and give it a pointer to your already-
constructed `mirror_write_registration_t`. When the constructor returns, you
will be registered for realtime read operations unless something went wrong. The
realtime read registration will be valid iff the write registration is valid, so
just check `mirror_write_registration_t::get_failed_signal()` to see if you are
registered properly.

Destroy the `mirror_read_registration_t` to inform the master you're no longer
ready to process realtime reads. This is useful if you want to do a "warm
shutdown" where you process any operations the master has already sent you but
