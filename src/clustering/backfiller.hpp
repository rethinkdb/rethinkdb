#ifndef __CLUSTERING_BACKFILLER_HPP__
#define __CLUSTERING_BACKFILLER_HPP__

#include <map>

#include "clustering/backfill_metadata.hpp"
#include "clustering/resource.hpp"
#include "concurrency/wait_any.hpp"
#include "rpc/metadata/view.hpp"
#include "timestamps.hpp"

template<class protocol_t>
struct backfiller_t :
    public home_thread_mixin_t
{
    backfiller_t(
            mailbox_cluster_t *c,
            typename protocol_t::store_t *s,
            metadata_readwrite_view_t<resource_metadata_t<backfiller_metadata_t<protocol_t> > > *md_view) :
        cluster(c), store(s)
    {
        /* Set up mailboxes, in scoped pointers so we can deconstruct them when
        it's convenient */
        backfill_mailbox.reset(new typename backfiller_metadata_t<protocol_t>::backfill_mailbox_t(
            cluster, boost::bind(&backfiller_t::on_backfill, this, _1, _2, _3, _4)));
        cancel_backfill_mailbox.reset(new typename backfiller_metadata_t<protocol_t>::cancel_backfill_mailbox_t(
            cluster, boost::bind(&backfiller_t::on_cancel_backfill, this, _1)));

        /* Advertise our existence */
        backfiller_metadata_t<protocol_t> business_card;
        business_card.backfill_mailbox = backfill_mailbox->get_address();
        business_card.cancel_backfill_mailbox = cancel_backfill_mailbox->get_address();
        advertisement.reset(new resource_advertisement_t<backfiller_metadata_t<protocol_t> >(cluster, md_view, business_card));
    }

    ~backfiller_t() {

        /* Stop advertising our existence */
        advertisement.reset();

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
    typedef typename backfiller_metadata_t<protocol_t>::backfill_session_id_t session_id_t;

    void on_backfill(
        session_id_t session_id,
        typename protocol_t::store_t::backfill_request_t request,
        typename async_mailbox_t<void(typename protocol_t::store_t::backfill_chunk_t)>::address_t chunk_cont,
        typename async_mailbox_t<void(state_timestamp_t)>::address_t end_cont) {

        assert_thread();

        /* Acquire the drain semaphore so that the backfiller doesn't shut down
        while we're still running. */
        drain_semaphore_t::lock_t lock(&drain_semaphore);

        /* Set up a local interruptor cond and put it in the map so that this
        session can be interrupted if the backfillee decides to abort */
        cond_t local_interruptor;
        local_interruptors[session_id] = &local_interruptor;

        /* Set up a cond that gets pulsed if we're interrupted either way */
        wait_any_t interrupted(&local_interruptor, &global_interruptor);

        /* Calling `send_chunk()` will send a chunk to the backfillee. We need
        to cast `send()` to the correct type before calling `boost::bind()` so
        that C++ will find the correct overload. */
        void (*send_cast_to_correct_type)(
            mailbox_cluster_t *,
            typename async_mailbox_t<void(typename protocol_t::store_t::backfill_chunk_t)>::address_t,
            const typename protocol_t::store_t::backfill_chunk_t &) = &send;
        boost::function<void(typename protocol_t::store_t::backfill_chunk_t)> send_fun =
            boost::bind(send_cast_to_correct_type, cluster, chunk_cont, _1);

        /* Perform the backfill */
        bool success;
        state_timestamp_t end;
        try {
            end = store->backfiller(
                request,
                send_fun,
                &interrupted);
            success = true;
        } catch (interrupted_exc_t) {
            rassert(interrupted.is_pulsed());
            success = false;
        }

        /* Send the confirmation */
        if (success) {
            send(cluster, end_cont, end);
        }

        /* Get rid of our local interruptor */
        local_interruptors.erase(session_id);
    }

    void on_cancel_backfill(session_id_t session_id) {

        assert_thread();

        typename std::map<session_id_t, cond_t *>::iterator it =
            local_interruptors.find(session_id);
        if (it != local_interruptors.end()) {
            (*it).second->pulse();
        }
    }

    mailbox_cluster_t *cluster;
    typename protocol_t::store_t *store;

    drain_semaphore_t drain_semaphore;
    cond_t global_interruptor;
    std::map<session_id_t, cond_t *> local_interruptors;

    boost::scoped_ptr<typename backfiller_metadata_t<protocol_t>::backfill_mailbox_t> backfill_mailbox;
    boost::scoped_ptr<typename backfiller_metadata_t<protocol_t>::cancel_backfill_mailbox_t> cancel_backfill_mailbox;

    boost::scoped_ptr<resource_advertisement_t<backfiller_metadata_t<protocol_t> > > advertisement;
};

#endif /* __CLUSTERING_BACKFILLER_HPP__ */
