#ifndef __CLUSTERING_BACKFILLER_HPP__
#define __CLUSTERING_BACKFILLER_HPP__

#include <map>

#include "clustering/backfill_metadata.hpp"
#include "clustering/resource.hpp"
#include "protocol_api.hpp"

/* Used internally by `backfiller_t`. Grr I want lambdas grr. */
inline state_timestamp_t get_earliest_timestamp_of_version_range(const version_range_t &vr) {
    return vr.earliest.timestamp;
}

/* If you construct a `backfiller_t` for a given store, then it will advertise
its existence in the metadata and serve backfills over the network. */
template<class protocol_t>
struct backfiller_t :
    public home_thread_mixin_t
{
    backfiller_t(
            mailbox_cluster_t *c,
            branch_history_database_t<protocol_t> *bh,
            store_view_t<protocol_t> *s,
            boost::shared_ptr<metadata_readwrite_view_t<resource_metadata_t<backfiller_metadata_t<protocol_t> > > > md_view) :
        cluster(c), branch_history(bh),
        store(s),
        backfill_mailbox(cluster, boost::bind(&backfiller_t::on_backfill, this, _1, _2, _3, _4, _5, auto_drainer_t::lock_t(&drainer))),
        cancel_backfill_mailbox(cluster, boost::bind(&backfiller_t::on_cancel_backfill, this, _1, auto_drainer_t::lock_t(&drainer))),
        advertisement(cluster, md_view, backfiller_metadata_t<protocol_t>(backfill_mailbox.get_address(), cancel_backfill_mailbox.get_address()))
        { }

private:
    typedef typename backfiller_metadata_t<protocol_t>::backfill_session_id_t session_id_t;

    void on_backfill(
            session_id_t session_id,
            region_map_t<protocol_t, version_range_t> start_point,
            typename async_mailbox_t<void(region_map_t<protocol_t, version_range_t>)>::address_t end_point_cont,
            typename async_mailbox_t<void(typename protocol_t::backfill_chunk_t)>::address_t chunk_cont,
            async_mailbox_t<void()>::address_t done_cont,
            auto_drainer_t::lock_t keepalive) {

        assert_thread();
        rassert(region_is_superset(store->get_region(), start_point.get_domain()));

        /* Set up a local interruptor cond and put it in the map so that this
        session can be interrupted if the backfillee decides to abort */
        cond_t local_interruptor;
        map_insertion_sentry_t<session_id_t, cond_t *> be_interruptible(&local_interruptors, session_id, &local_interruptor);

        /* Set up a cond that gets pulsed if we're interrupted by either the
        backfillee stopping or the backfiller destructor being called, but don't
        wait on that cond yet. */
        wait_any_t interrupted(&local_interruptor, keepalive.get_drain_signal());

        try {
            boost::shared_ptr<typename store_view_t<protocol_t>::read_transaction_t> transaction =
                store->begin_read_transaction(&interrupted);
            region_map_t<protocol_t, version_range_t> end_point =
                region_map_transform<protocol_t, binary_blob_t, version_range_t>(
                    transaction->get_metadata(&interrupted),
                    &binary_blob_t::get<version_range_t>
                    );

#ifndef NDEBUG
            /* Confirm that `start_point` is a point in our past */
            std::vector<std::pair<typename protocol_t::region_t, version_range_t> > start_point_pairs =
                start_point.get_as_pairs();
            std::vector<std::pair<typename protocol_t::region_t, version_range_t> > end_point_pairs =
                end_point.get_as_pairs();
            for (int i = 0; i < (int)start_point_pairs.size(); i++) {
                for (int j = 0; j < (int)end_point_pairs.size(); j++) {
                    typename protocol_t::region_t ixn = region_intersection(start_point_pairs[i].first, end_point_pairs[i].first);
                    if (!region_is_empty(ixn)) {
                        version_t start = start_point_pairs[i].second.latest;
                        version_t end = end_point_pairs[i].second.earliest;
                        rassert(start.timestamp <= end.timestamp);
                        rassert(branch_history->version_is_ancestor(start, end, ixn));
                    }
                }
            }
#endif

            /* Transmit `end_point` to the backfillee */
            send(cluster, end_point_cont, end_point);

            /* Calling `send_chunk()` will send a chunk to the backfillee. We
            need to cast `send()` to the correct type before calling
            `boost::bind()` so that C++ will find the correct overload. */
            void (*send_cast_to_correct_type)(
                mailbox_cluster_t *,
                typename async_mailbox_t<void(typename protocol_t::backfill_chunk_t)>::address_t,
                const typename protocol_t::backfill_chunk_t &
                ) = &send;
            boost::function<void(typename protocol_t::backfill_chunk_t)> send_fun =
                boost::bind(send_cast_to_correct_type, cluster, chunk_cont, _1);

            /* Actually perform the backfill */
            transaction->send_backfill(
                region_map_transform<protocol_t, version_range_t, state_timestamp_t>(
                    start_point,
                    &get_earliest_timestamp_of_version_range
                    ),
                send_fun,
                &interrupted
                );

            /* Send a confirmation */
            send(cluster, done_cont);

        } catch (interrupted_exc_t) {
            /* Ignore. If we were interrupted by the backfillee, then it already
            knows the backfill is cancelled. If we were interrupted by the
            backfiller shutting down, it will know when it sees we deconstructed
            our `resource_advertisement_t`. */
        }
    }

    void on_cancel_backfill(session_id_t session_id, UNUSED auto_drainer_t::lock_t) {

        assert_thread();

        typename std::map<session_id_t, cond_t *>::iterator it =
            local_interruptors.find(session_id);
        if (it != local_interruptors.end()) {
            (*it).second->pulse();
        }
    }

    mailbox_cluster_t *cluster;
    branch_history_database_t<protocol_t> *branch_history;

    store_view_t<protocol_t> *store;

    auto_drainer_t drainer;
    std::map<session_id_t, cond_t *> local_interruptors;

    typename backfiller_metadata_t<protocol_t>::backfill_mailbox_t backfill_mailbox;
    typename backfiller_metadata_t<protocol_t>::cancel_backfill_mailbox_t cancel_backfill_mailbox;

    resource_advertisement_t<backfiller_metadata_t<protocol_t> > advertisement;
};

#endif /* __CLUSTERING_BACKFILLER_HPP__ */
