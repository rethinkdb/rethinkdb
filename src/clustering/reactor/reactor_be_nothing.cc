#include "clustering/reactor/reactor.hpp"

#include "clustering/immediate_consistency/branch/backfiller.hpp"
#include "clustering/immediate_consistency/branch/multistore.hpp"
#include "clustering/immediate_consistency/branch/replier.hpp"


/* Returns true if every peer listed as a primary for this shard in the
 * blueprint has activity primary_t and every peer listed as a secondary has
 * activity secondary_up_to_date_t. */
template <class protocol_t>
bool reactor_t<protocol_t>::is_safe_for_us_to_be_nothing(const std::map<peer_id_t, reactor_business_card_t<protocol_t> > &_reactor_directory, const blueprint_t<protocol_t> &blueprint,
                                                         const typename protocol_t::region_t &region)
{
    typedef reactor_business_card_t<protocol_t> rb_t;

    /* Iterator through the peers the blueprint claims we should be able to
     * see. */
    for (typename blueprint_t<protocol_t>::role_map_t::const_iterator p_it =  blueprint.peers_roles.begin();
                                                                      p_it != blueprint.peers_roles.end();
                                                                      p_it++) {
        typename std::map<peer_id_t, reactor_business_card_t<protocol_t> >::const_iterator bcard_it = _reactor_directory.find(p_it->first);
        if (bcard_it == _reactor_directory.end()) {
            //The peer is down or has no reactor
            return false;
        }

        typename blueprint_t<protocol_t>::region_to_role_map_t::const_iterator r_it = p_it->second.find(region);
        rassert(r_it != p_it->second.end(), "Invalid blueprint issued, different peers have different sharding schemes.\n");

        /* Whether or not we found a directory entry for this peer */
        bool found = false;
        for (typename rb_t::activity_map_t::const_iterator it =  bcard_it->second.activities.begin();
                                                           it != bcard_it->second.activities.end();
                                                           it++) {
            if (it->second.first == region) {
                if (r_it->second == blueprint_details::role_primary) {
                    if (!boost::get<typename rb_t::primary_t>(&it->second.second)) {
                        return false;
                    }
                } else if (r_it->second == blueprint_details::role_secondary) {
                    if (!boost::get<typename rb_t::secondary_up_to_date_t>(&it->second.second)) {
                        return false;
                    }
                }
                found = true;
                break;
            }
        }

        if (!found) {
            return false;
        }
    }

    return true;
}

template<class protocol_t>
void reactor_t<protocol_t>::be_nothing(typename protocol_t::region_t region,
        multistore_ptr_t<protocol_t> *svs, const clone_ptr_t<watchable_t<blueprint_t<protocol_t> > > &blueprint,
        signal_t *interruptor) THROWS_NOTHING {
    order_source_t order_source;  // TODO: order_token_t::ignore

    try {
        directory_entry_t directory_entry(this, region);

        {
            /* We offer backfills while waiting for it to be safe to shutdown
             * in case another peer needs a copy of the data */
            backfiller_t<protocol_t> backfiller(mailbox_manager, branch_history_manager, svs);

            /* Tell the other peers that we are looking to shutdown and
             * offering backfilling until we do. */
            scoped_ptr_t<fifo_enforcer_sink_t::exit_read_t> read_token;
            svs->new_read_token(&read_token);
            region_map_t<protocol_t, binary_blob_t> metainfo_blob;
            svs->do_get_metainfo(order_source.check_in("be_nothing").with_read_mode(), &read_token, interruptor, &metainfo_blob);
            typename reactor_business_card_t<protocol_t>::nothing_when_safe_t
                activity(to_version_range_map(metainfo_blob), backfiller.get_business_card());
            directory_echo_version_t version_to_wait_on = directory_entry.set(activity);

            /* Make sure everyone sees that we're trying to erase our data,
             * it's important to do this to avoid the following race condition:
             *
             * Peer 1 and Peer 2 both are secondaries.
             * Peer 1 gets a blueprint saying its role is nothing and peer 2's is secondary,
             * Peer 2 gets a blueprint saying its role is nothing and peer 1's is secondary,
             *
             * since each one sees the other is secondary they both think it's
             * safe to shutdown and thus destroy their data leading to data
             * loss.
             *
             * The below line makes sure that they will sync their new roles
             * with one another before they begin destroying data.
             *
             * This makes it possible for either to proceed with deleting the
             * data, but never both, it's also possible that neither proceeds
             * which is okay as well.
             */
            wait_for_directory_acks(version_to_wait_on, interruptor);

            /* Make sure we don't go down and delete the data on our machine
             * before every who needs a copy has it. */
            run_until_satisfied_2(
                directory_echo_mirror.get_internal(),
                blueprint,
                boost::bind(&reactor_t<protocol_t>::is_safe_for_us_to_be_nothing, this, _1, _2, region),
                interruptor);
        }

        /* We now know that it's safe to shutdown so we tell the other peers
         * that we are beginning the process of erasing data. */
        directory_entry.set(typename reactor_business_card_t<protocol_t>::nothing_when_done_erasing_t());

        /* This actually erases the data. */
        {
            scoped_ptr_t<fifo_enforcer_sink_t::exit_write_t> write_token;
            svs->new_write_token(&write_token);

            svs->reset_data(region, region_map_t<protocol_t, binary_blob_t>(region, binary_blob_t(version_range_t(version_t::zero()))), &write_token, interruptor);
        }

        /* Tell the other peers that we are officially nothing for this region,
         * end of story. */
        directory_entry.set(typename reactor_business_card_t<protocol_t>::nothing_t());

        interruptor->wait_lazily_unordered();
    } catch (interrupted_exc_t) {
        /* ignore */
    }
}


#include "mock/dummy_protocol.hpp"
#include "memcached/protocol.hpp"
#include "rdb_protocol/protocol.hpp"

template class reactor_t<mock::dummy_protocol_t>;
template class reactor_t<memcached_protocol_t>;
template class reactor_t<rdb_protocol_t>;
