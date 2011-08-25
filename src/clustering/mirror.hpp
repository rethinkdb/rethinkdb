#ifndef __CLUSTERING_MIRROR_HPP__
#define __CLUSTERING_MIRROR_HPP__

template<class protocol_t>
class mirror_t {

private:
    typedef branch_metadata_t<protocol_t>::mirror_data_t mirror_data_t;

public:
    mirror_t(
            typename protocol_t::store_t *store,
            mailbox_cluster_t *cluster,
            metadata_readwrite_view_t<branch_metadata_t<protocol_t> > *branch_metadata,
            mirror_id_t backfill_provider,
            signal_t *interruptor) :

        write_mailbox(cluster, boost::bind(&mirror_t::on_write, this, _1, _2, _3, _4)),
        writeread_mailbox(cluster, boost::bind(&mirror_t::on_writeread, this, _1, _2, _3, _4)),
        read_mailbox(cluster, boost::bind(&mirror_t::on_read, this, _1, _2, _3)),

        registrar_view(&branch_metadata_t<protocol_t>::registrar, branch_metadata),
        mirror_map_view(&branch_metadata_t<protocol_t>::mirror_map, branch_metadata),
    {
        if (interruptor->is_pulsed()) throw interrupted_exc_t();

        /* Make sure that `shutdown_cond` gets pulsed if `interruptor` gets
        pulsed. This is  */
        signal_t::subscription_t shutdown_cond_pulser(
            boost::bind(&cond_t::pulse, &shutdown_cond),
            interruptor);

        /* Every write operation enters, then exits, `operation_order_manager`
        before it touches the store. By creating and holding a
        `coro_fifo_acq_t`, we can stop write operations from happening. We put
        it in a `boost::scoped_ptr` so we can release it at any time. */
        boost::scoped_ptr<coro_fifo_acq_t> backfill_coro_fifo_acq(
            new coro_fifo_acq_t(&operation_order_manager_t));

        /* Register for writes */
        try {
            registrant.reset(new registrant_t<mirror_data_t>(
                cluster,
                &registrar_view,
                mirror_data_t(write_mailbox.get_address()),
                interruptor));
        } catch (resource_lost_exc_t) {
            /* Ignore the error; we can still backfill from another node and
            meaningfully serve read-outdated queries */
        }

        /* Get a backfill */
        {
            metadata_member_read_view_t<
                mirror_id_t,
                resource_metadata_t<backfiller_metadata_t<protocol_t> >
                > backfiller_view(backfill_provider, &mirror_map_view);
            backfill(store, cluster, &backfiller_view, interruptor);
        }

        /* Allow write operations to proceed */
        backfill_coro_fifo_acq.reset();

        /* Register for writereads and reads */
        if (registrant) {
            try {
                registrant->update(mirror_data_t(
                    write_mailbox.get_address(),
                    writeread_mailbox.get_address(),
                    read_mailbox.get_address()));
            } catch (resource_lost_exc_t) {
                /* The master is down. Ignore the error because there's still value
                in responding to backfill requests and read-outdated queries. */
            }
        }

        /* Start serving backfills */
        mirror_id = generate_uuid();
        {
            /* Make a space for us in the mirror map */
            std::map<mirror_id_t, resource_metadata_t<backfiller_metadata_t<protocol_t> > > new_mirror;
            new_mirror[mirror_id] = resource_metadata_t<backfiller_metadata_t<protocol_t> >();
            mirror_map_view.join(new_mirror);
        }
        /* Set up a view pointing at our space in the mirror map */
        backfiller_view.reset(new metadata_member_readwrite_view_t<
            mirror_id_t,
            resource_metadata_t<backfiller_metadata_t<protocol_t> >
            >(mirror_id, &mirror_map_view));
        /* Set up the actual backfiller */
        backfiller.reset(new backfiller_t<protocol_t>(
            cluster, store, backfiller_view.get()));
    }

private:
    void on_write(
            typename protocol_t::write_t write,
            repli_timestamp_t ts,
            order_token_t tok,
            async_mailbox_t<void()>::address_t ack_addr) {
        
    }

    void on_writeread(
            typename protocol_t::write_t write,
            repli_timestamp_t ts,
            order_token_t tok,
            async_mailbox_t<void(typename protocol_t::write_response_t)>::address_t resp_addr) {
        
    }

    void on_read(
            typename protocol_t::read_t read,
            order_token_t tok,
            async_mailbox_t<void(typename protocol_t::read_response_t)>::address_t resp_addr) {
        
    }

    cond_t shutdown_cond;
    drain_semaphore_t drain_semaphore;

    /* This `coro_fifo_t` is responsible for making sure that operations are
    run in the same order that they arrive. We also use it to make the
    writes wait until the backfill is done. */
    coro_fifo_t operation_order_manager;

    typename mirror_data_t::write_mailbox_t write_mailbox;
    typename mirror_data_t::writeread_mailbox_t writeread_mailbox;
    typename mirror_data_t::read_mailbox_t read_mailbox;

    metadata_field_read_view_t<
        branch_metadata_t<protocol_t>,
        resource_metadata_t<registrar_metadata_t<mirror_data_t> >
        > registrar_view;
    metadata_field_readwrite_view_t<
        branch_metadata_t<protocol_t>,
        std::map<mirror_id_t, resource_metadata_t<backfiller_metadata_t<protocol_t> > >
        > mirror_map_view;

    boost::scoped_ptr<registrant_t<mirror_data_t> > registrant;

    typename branch_metadata_t<protocol_t>::mirror_id_t mirror_id;
    boost::scoped_ptr<metadata_member_readwrite_view_t<
        mirror_id_t,
        resource_metadata_t<backfiller_metadata_t<protocol_t> >
        > > backfiller_view;
    boost::scoped_ptr<backfiller_t<protocol_t> > backfiller;
};

#endif /* __CLUSTERING_MIRROR_HPP__ */
