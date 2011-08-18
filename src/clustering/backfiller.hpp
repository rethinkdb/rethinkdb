#ifndef __CLUSTERING_BACKFILLER_HPP__
#define __CLUSTERING_BACKFILLER_HPP__

#include "clustering/backfill_metadata.hpp"

template<class protocol_t>
struct backfiller_t :
    public home_thread_mixin_t
{

    backfiller_t(mailbox_cluster_t *c, typename protocol_t::store_t *s) :
        cluster(c), store(s)
    {
        /* Set up mailboxes, in scoped pointers so we can deconstruct them when
        it's convenient */
        backfill_mailbox.reset(new backfiller_metadata_t<protocol_t>::backfill_mailbox_t(
            cluster, boost::bind(&backfiller_t::on_backfill, this, _1, _2, _3, _4)));
        cancel_backfill_mailbox.reset(new backfiller_metadata_t<protocol_t>::cancel_backfill_mailbox_t(
            cluster, boost::bind(&backfiller_t::on_cancel_backfill, this, _1)));
    }

    backfiller_metadata_t<protocol_t> get_business_card() {
        assert_thread();
        backfiller_metadata_t<protocol_t> business_card;
        business_card.backfill_mailbox = &backfill_mailbox;
        business_card.cancel_backfill_mailbox = &cancel_backfill_mailbox;
        return business_card;
    }

    ~backfiller_t() {

        /* Tear down mailboxes so no new backfills can begin. It doesn't matter
        if we receive further messages in our `cancel_backfill_mailbox` at this
        point, but we tear it down anyway for no reason. */
        backfill_mailbox.reset();
        cancel_backfill_mailbox.reset();

        /* Cancel all existing backfills */
        global_interruptor.pulse();

        /* Wait for cancellation to take effect */
        drain_semaphore.drain();
    }

private:
    typedef backfiller_metadata_t<protocol_t>::backfill_session_id_t session_id_t;

    void on_backfill(
        session_id_t session_id,
        typename protocol_t::backfill_request_t request,
        async_mailbox_t<void(typename protocol_t::backfill_chunk_t)>::address_t chunk_cont,
        async_mailbox_t<void()>::address_t end_cont) {

        assert_thread();

        /* Acquire the drain semaphore so that the backfiller doesn't shut down
        while we're still running. */
        drain_semaphore_t::lock_t lock(&drain_semaphore);

        /* Set up a local interruptor cond and put it in the map so that this
        session can be interrupted if the backfillee decides to abort */
        cond_t local_interruptor;
        local_interruptors[session_id] = &local_interruptor;

        /* Set up a cond that gets pulsed if we're interrupted either way */
        cond_t any_interruption;
        cond_link_t listen_for_local_interrupts(&local_interruptor, &any_interruption);
        cond_link_t listen_for_global_interrupts(&global_interruptor, &any_interruption);

        /* Perform the backfill */
        bool success;
        try {
            store->send_backfill(
                request,
                boost::bind(&send, cluster, chunk_cont, _1),
                &any_interruption
                );
            success = true;
        } catch (interrupted_exc_t) {
            rassert(any_interruption.is_pulsed());
            success = false;
        }

        /* Send the confirmation */
        if (success) {
            send(cluster, end_cont);
        }

        /* Get rid of our local interruptor */
        local_interruptors.erase(session_id);
    }

    void on_cancel_backfill(session_id_t session_id) {

        assert_thread();

        std::map<session_id_t, cond_t *>::iterator it =
            local_interruptors.find(session_id);
        if (it != local_interruptors.end()) {
            (*it).second->pulse();
        }
    }

    mailbox_cluster_t *cluster;
    typename protocol_t::store_t *store;
    metadata_view_t<boost::optional<backfiller_metadata_t<protocol_t> > > *metadata;

    boost::scoped_ptr<backfiller_metadata_t<protocol_t>::backfill_mailbox_t> backfill_mailbox;
    boost::scoped_ptr<backfiller_metadata_t<protocol_t>::cancel_backfill_mailbox_t> cancel_backfill_mailbox;

    drain_semaphore_t drain_semaphore;
    cond_t global_interruptor;
    std::map<session_id_t, cond_t *> local_interruptors;
};

#endif /* __CLUSTERING_BACKFILLER_HPP__ */
