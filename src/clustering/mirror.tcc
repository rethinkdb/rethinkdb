





template<class protocol_t>
void mirror(
    typename protocol_t::store_t *store,
    mailbox_cluster_t *cluster,
    metadata_view_t<branch_metadata_t<protocol_t> > *branch_metadata,
    mirror_id_t backfiller_mirror_id
    signal_t *interruptor) {

    if (interruptor->is_pulsed())
        throw interrupted_exc_t();

    /* Start watching to see if the master will stop being master or will die
    at some point. */

    

    /* Pick our mirror ID */

    mirror_id_t mirror_id = generate_uuid();

    /* Set up our write mailbox. Any writes we receive will be queued; later we
    will start reading off the queue, but not yet. */

    unlimited_fifo_queue_t<boost::function<void()> > operation_queue;

    branch_metadata_t<protocol_t>::mirror_write_mailbox_t write_mailbox(
        cluster, boost::bind(&push_write, &operation_queue, store, _1, _2, _3));

    /* Send a join-message to the master and wait for a reply */

    {
        cond_t master_has_noticed_us;
        branch_metadata_t<protocol_t>::mirror_greet_mailbox_t greet_mailbox(
            cluster,
            boost::bind(&cond_t::pulse, &master_has_noticed_us));

        /* Check before we send because if the master is offline, `join_mailbox`
        will have been set to nil and we'll crash. */
        if (master_is_offline.is_pulsed())
            throw resource_disrupted_exc_t("branch we're joining is defunct");

        send(cluster, branch_metadata->get_metadata().join_mailbox,
            mirror_id, &greet_mailbox, &write_mailbox);

        cond_t c;
        cond_link_t continue_when_master_notices_us(&master_has_noticed_us, &c);
        cond_link_t continue_when_master_dies(&master_has_died, &c);
        cond_link_t continue_when_master_offline(&master_is_offline, &c);
        cond_link_t continue_when_interrupted(interruptor, &c);
        c.wait_lazily_unordered();

        if (master_has_died.is_pulsed() ||
            master_is_offline.is_pulsed() ||
            interruptor->is_pulsed()) return;

        rassert(master_has_noticed_us.is_pulsed());
    }

    /* Now that we've told the master we exist, we have an obligation to tell
    the master if we stop existing. This object's destructor informs the master
    that we've stopped existing. It won't get run if we crash, but that's OK; if
    the master loses sight of the node we're hosted on, it will assume that we
    no longer exist. */

    /* Check before we read `leave_mailbox` because if the master is offline,
    `leave_mailbox` will be nil and any attempt to send to it will crash. */
    if (master_is_offline.is_pulsed()) return;

    death_runner_t master_notifier;
    master_notifier.fun = boost::bind(
        &send, cluster, branch_metadata->get_metadata().leave_mailbox, mirror_id);

    /* Request a backfill from the specified node */

    {
        
    }

    /* Now that we've received our full backfill, start running the queued
    writes that we got from the master. */

    /* TODO: Make `100` a `#define`d constant. */
    coro_pool_t operation_runner(100, &operation_queue);

    /* Prepare to accept read queries */

    branch_metadata_t<protocol_t>::mirror_read_mailbox_t read_mailbox(
        cluster, boost::bind(&push_read, &operation_queue, store, _1, _2));

    /* If the master has gone offline, we can't tell it we're ready to accept
    reads because `ready_mailbox` will be nil. But we don't want to shut down,
    because we can still meaningfully serve outdated reads. So we just don't
    bother notifying the master. */
    if (!master_is_offline.is_pulsed()) {
        send(cluster,
            branch_metadata->get_metadata().ready_mailbox,
            mirror_id,
            &read_mailbox);
    }

    /* Make mailboxes to handle read-outdated queries and backfill requests */

    mirror_metadata_t<protocol_t>::read_mailbox_t read_outdated_mailbox(
        cluster, boost::bind(&perform_read_outdated, store, _1, _2));

    backfill_provider_t backfill_provider(store);

    /* Send out metadata saying that we're ready to accept read-outdated queries
    and backfill requests. The `metadata_presence_t` type registers us in its
    constructor and automatically deregisters us in its destructor. */

    struct metadata_presence_t {

        metadata_view_t<branch_metadata_t<protocol_t> > *branch_metadata;
        mirror_id_t mirror_id;

        metadata_presence_t(
                metadata_view_t<branch_metadata_t<protocol_t> > *branch_metadata,
                mirror_id_t mirror_id,
                mirror_metadata_t<protocol_t>::read_mailbox_t::address_t rmb,
                mirror_metadata_t<protocol_t>::backfill_mailbox_t::address_t bmb,
                mirror_metadata_t<protocol_t>::cancel_backfill_mailbox_t::address_t cbmb) :
            branch_metadata(branch_metadata), mirror_id(mirror_id)
        {
            mirror_metadata_t<protocol_t> our_metadata;
            our_metadata.alive = true;
            our_metadata.read_mailbox = rmb;
            our_metadata.backfill_mailbox = bmb;
            our_metadata.cancel_backfill_mailbox = cmb;

            branch_metadata_t<protocol_t> branch_md = branch_metadata->get_metadata();
            rassert(branch_md.mirrors.count(mirror_id) == 0);
            branch_md.mirrors[mirror_id] = our_metadata;
            branch_metadata->join_metadata(branch_md);
        }

        ~metadata_presence_t() {

            mirror_metadata_t<protocol_t> dead_metadata;
            dead_metadata.alive = false;

            branch_metadata_t<protocol_t> branch_md = branch_metadata->get_metadata();
            rassert(branch_md.mirrors[mirror_id].alive);
            branch_md.mirrors[mirror_id] = dead_metadata;
            branch_metadata->join_metadata(branch_md);
        }
    };

    metadata_presence_t metadata_presence(
        branch_metadata,
        mirror_id,
        &read_outdated_mailbox,
        &backfill_provider.backfill_mailbox,
        &backfill_provider.cancel_backfill_mailbox);

    /* Chill out and wait for an order to shut down. Even if we lose contact
    with the master, we'll keep serving read-outdated requests and backfill
    requests until the administrative assistant shuts us down. */

    interruptor->wait_lazily_unordered();

    /* The destructor for `metadata_presence` is run here. It will inform other
    nodes that we are no longer serving read-outdated queries or backfill
    requests. If anything is currently backfilling from us, it will notice our
    metadata broadcast and give up on the backfill. */

    /* The destructor for `backfill_provider_t` is run here. Any running
    backfills will be unceremoniously terminated. No explicit "cancel backfill"
    message will be sent to the backfillee; they will be expected to find out
    via the metadata. */

    /* The destructor for `operation_runner` is run here. It will stop handling
    reads and writes; any remaining reads and writes will be left on the
    queue.

    TODO: What if there's a read left on the queue? The client will get an
    error message back. Can we do better than that? We could tell the master
    that we're no longer accepting queries, then run any queries we've gotten,
    then shut down. Is that worth it? */

    /* The destructor for `master_notifier` is run here. It will tell the master
    that we are no longer accepting queries. */
}