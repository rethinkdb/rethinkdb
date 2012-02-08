#include "clustering/immediate_consistency/branch/broadcaster.hpp"
#include "clustering/immediate_consistency/branch/replier.hpp"
#include "clustering/immediate_consistency/query/master.hpp"

template<class protocol_t>
reactor_t<protocol_t>::reactor_t(
        mailbox_manager_t *mm,
        clone_ptr_t<directory_rwview_t<boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > > > rd,
        clone_ptr_t<directory_wview_t<std::map<master_id_t, master_business_card_t<protocol_t> > > > _master_directory,
        boost::shared_ptr<semilattice_readwrite_view_t<branch_history_t<protocol_t> > > bh,
        watchable_t<blueprint_t<protocol_t> > *b,
        store_view_t<protocol_t> *_underlying_store) THROWS_NOTHING :
    mailbox_manager(mm), directory_echo_access(mailbox_manager, rd, reactor_business_card_t<protocol_t>()), 
    master_directory(_master_directory), 
    branch_history(bh), blueprint(b), underlying_store(_underlying_store),
    blueprint_subscription(boost::bind(&reactor_t<protocol_t>::on_blueprint_changed, this))
{
    {
        typename watchable_t<blueprint_t<protocol_t> >::freeze_t freeze(blueprint);
        blueprint->get().assert_valid();
        try_spawn_roles();
        blueprint_subscription.reset(blueprint, &freeze);
    }
}

template <class protocol_t>
reactor_t<protocol_t>::directory_entry_t::directory_entry_t(reactor_t<protocol_t> *_parent, typename protocol_t::region_t _region)
    : parent(_parent), region(_region), reactor_activity_id(boost::uuids::nil_generator()())
{ }

template <class protocol_t>
directory_echo_version_t reactor_t<protocol_t>::directory_entry_t::set(typename reactor_business_card_t<protocol_t>::activity_t activity) {
    typename directory_echo_access_t<reactor_business_card_t<protocol_t> >::our_value_change_t our_value_change(&parent->directory_echo_access);
    if (!reactor_activity_id.is_nil()) {
        our_value_change.buffer.activities.erase(reactor_activity_id);
    }
    reactor_activity_id = generate_uuid();
    our_value_change.buffer.activities.insert(std::make_pair(reactor_activity_id, std::make_pair(region, activity)));
    return our_value_change.commit();
}

template <class protocol_t>
directory_echo_version_t reactor_t<protocol_t>::directory_entry_t::update_without_changing_id(typename reactor_business_card_t<protocol_t>::activity_t activity) {
    typename directory_echo_access_t<reactor_business_card_t<protocol_t> >::our_value_change_t our_value_change(&parent->directory_echo_access);
    rassert(!reactor_activity_id.is_nil(), "This method should only be called when an activity has already been set\n");

    our_value_change.buffer.activities.insert(std::make_pair(reactor_activity_id, std::make_pair(region, activity)));
    return our_value_change.commit();
}

template <class protocol_t>
reactor_t<protocol_t>::directory_entry_t::~directory_entry_t() {
    if (!reactor_activity_id.is_nil()) {
        typename directory_echo_access_t<reactor_business_card_t<protocol_t> >::our_value_change_t our_value_change(&parent->directory_echo_access);
        our_value_change.buffer.activities.erase(reactor_activity_id);
        our_value_change.commit();
    }
}

template<class protocol_t>
void reactor_t<protocol_t>::on_blueprint_changed() THROWS_NOTHING {
    blueprint->get().assert_valid();
    std::map<typename protocol_t::region_t, typename blueprint_t<protocol_t>::role_t> blueprint_roles =
        (*blueprint->get().peers.find(get_me())).second;
    typename std::map<
            typename protocol_t::region_t,
            std::pair<typename blueprint_t<protocol_t>::role_t, cond_t *>
            >::iterator it;
    for (it = current_roles.begin(); it != current_roles.end(); it++) {
        typename std::map<typename protocol_t::region_t, typename blueprint_t<protocol_t>::role_t>::iterator it2 =
            blueprint_roles.find((*it).first);
        if (it2 == blueprint_roles.end() || (*it).second.first != (*it2).second) {
            if (!(*it).second.second->is_pulsed()) {
                (*it).second.second->pulse();
            }
        }
    }
}

template<class protocol_t>
void reactor_t<protocol_t>::try_spawn_roles() THROWS_NOTHING {
    std::map<typename protocol_t::region_t, typename blueprint_t<protocol_t>::role_t> blueprint_roles =
        (*blueprint->get().peers.find(get_me())).second;
    typename std::map<typename protocol_t::region_t, typename blueprint_t<protocol_t>::role_t>::iterator it;
    for (it = blueprint_roles.begin(); it != blueprint_roles.end(); it++) {
        bool none_overlap = true;
        typename std::map<
            typename protocol_t::region_t,
            std::pair<typename blueprint_t<protocol_t>::role_t, cond_t *>
            >::iterator it2;
        for (it2 = current_roles.begin(); it2 != current_roles.end(); it2++) {
            if (region_overlaps((*it).first, (*it2).first)) {
                none_overlap = false;
                break;
            }
        }

        if (none_overlap) {
            //This state will be cleaned up in run_role
            cond_t *blueprint_changed_cond = new cond_t;
            current_roles.insert(std::make_pair(it->first, std::make_pair(it->second, blueprint_changed_cond)));
            coro_t::spawn_sometime(boost::bind(&reactor_t<protocol_t>::run_role, this, it->first, 
                                               it->second, blueprint_changed_cond, auto_drainer_t::lock_t(&drainer)));
        }
    }
}

template <class protocol_t>
class store_subview_t : public store_view_t<protocol_t>
{
public:
    store_subview_t(store_view_t<protocol_t> *, typename protocol_t::region_t region)
        : store_view_t<protocol_t>(region)
    { }

    virtual boost::shared_ptr<typename store_view_t<protocol_t>::read_transaction_t> begin_read_transaction(
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) {
                crash("Not implemented\n");
            }

    virtual boost::shared_ptr<typename store_view_t<protocol_t>::write_transaction_t> begin_write_transaction(
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) {
                crash("Not implemented\n");
            }
};

template<class protocol_t>
void reactor_t<protocol_t>::run_role(
        typename protocol_t::region_t region,
        typename blueprint_t<protocol_t>::role_t role,
        cond_t *blueprint_changed_cond,
        auto_drainer_t::lock_t keepalive) THROWS_NOTHING {

    //A store_view_t derived object that acts as a store for the specified region
    store_subview_t<protocol_t> store_subview(underlying_store, region);

    //All of the be_{role} functions respond identically to blueprint changes
    //and interruptions... so we just unify those signals
    wait_any_t wait_any(blueprint_changed_cond, keepalive.get_drain_signal());

    switch (role) {
        case blueprint_t<protocol_t>::role_primary:
            be_primary(region, &store_subview, &wait_any);
            break;
        case blueprint_t<protocol_t>::role_secondary:
            be_secondary(region, &store_subview, &wait_any);
            break;
        case blueprint_t<protocol_t>::role_listener:
            be_listener(region, &store_subview, &wait_any);
            break;
        case blueprint_t<protocol_t>::role_nothing:
            be_nothing(region, &store_subview, &wait_any);
            break;
        default:
            unreachable();
            break;
    }

    //As promised, clean up the state from try_spawn_roles
    current_roles.erase(region);
    delete blueprint_changed_cond;

    if (!keepalive.get_drain_signal()->is_pulsed()) {
        try_spawn_roles();
    }
}

template<class protocol_t>
void reactor_t<protocol_t>::be_primary(typename protocol_t::region_t region, store_view_t<protocol_t> *store, signal_t *interruptor) THROWS_NOTHING {
    try {
        directory_entry_t directory_entry(this, region);
        directory_echo_version_t version_to_wait_on = directory_entry.set(typename reactor_business_card_t<protocol_t>::primary_when_safe_t());

        /* block until all peers have acked `directory_entry` */
        wait_for_directory_acks(version_to_wait_on, interruptor);

        directory_echo_access.get_internal_view()
        /* Block until foreach key in region: foreach peer in blueprint scope:
         * peer has SECONDAY_LOST, NOTHING_SOON, LISTENER, or NOTHING for this
         * region */

        /* foreach key in region: backfill from most up-to-date
           SECONDARY_LOST or NOTHING_SOON */
        broadcaster_t<protocol_t> broadcaster(mailbox_manager, branch_history, store, interruptor);

        directory_entry.set(typename reactor_business_card_t<protocol_t>::primary_t(broadcaster.get_business_card()));

        clone_ptr_t<directory_single_rview_t<boost::optional<broadcaster_business_card_t<protocol_t> > > > broadcaster_business_card =
            get_directory_entry_view<typename reactor_business_card_t<protocol_t>::primary_t>(get_me(), directory_entry.get_reactor_activity_id())->
                subview(optional_monad_lens<broadcaster_business_card_t<protocol_t>, typename reactor_business_card_t<protocol_t>::primary_t>(
                    field_lens(&reactor_business_card_t<protocol_t>::primary_t::broadcaster)));

        listener_t<protocol_t> listener(mailbox_manager, broadcaster_business_card, branch_history, &broadcaster, interruptor);
        replier_t<protocol_t> replier(&listener);
        master_t<protocol_t> master(mailbox_manager, master_directory, region, &broadcaster);

        directory_entry.update_without_changing_id(typename reactor_business_card_t<protocol_t>::primary_t(broadcaster.get_business_card(), replier.get_business_card()));

        interruptor->wait_lazily_unordered();
    } catch (interrupted_exc_t) {
        /* ignore */
    }
}

template<class protocol_t>
void reactor_t<protocol_t>::be_secondary(typename protocol_t::region_t region, store_view_t<protocol_t> *store, signal_t *interruptor) THROWS_NOTHING {
    try {
        /* Tell everyone that we're backfilling so that we can get up to
         * date. */
        directory_entry_t directory_entry(this, region);
        while (true) {
            boost::optional<clone_ptr_t<directory_single_rview_t<boost::optional<broadcaster_business_card_t<protocol_t> > > > > broadcaster =
                find_broadcaster_in_directory(region);
            if (!broadcaster) {
                /* TODO: Backfill from most up-to-date peer? */
                backfiller_t<protocol_t> backfiller(mailbox_manager, branch_history, store);
                typename reactor_business_card_t<protocol_t>::secondary_without_primary_t activity(store->get_metadata(interruptor), backfiller.get_business_card());
                directory_entry.set(activity);
                broadcaster = wait_for_broadcaster_to_appear_in_directory(region, interruptor);
            }

            /* We need to save this to a local variable because there may be a
             * race condition should the broadcaster go down. */
            boost::optional<broadcaster_business_card_t<protocol_t> > broadcaster_business_card = broadcaster.get()->get_value();
            if (!broadcaster_business_card) {
                /* The broadcaster went down immediately after we found it so
                 * we need to go through the loop again. */
                continue;
            }
            branch_id_t branch_id = broadcaster_business_card.get().branch_id;

            try {
                /* TODO we need to find a backfiller for listener_t */

                /* We have found a broadcaster (a master to track) so now we
                 * need to backfill to get up to date. */
                directory_entry.set(typename reactor_business_card_t<protocol_t>::secondary_backfilling_t());

                /* This causes backfilling to happen. Once this constructor returns we are up to date. */
                listener_t<protocol_t> listener(mailbox_manager, branch_history, broadcaster, store, interruptor);

                /* This gives others access to our services, in particular once
                 * this constructor returns people can send us queries and use
                 * us for backfills. */
                replier_t<protocol_t> replier(&listener);

                /* Make the directory reflect the new role that we are filling.
                 * (Being a secondary). */
                directory_entry.set(typename reactor_business_card_t<protocol_t>::secondary_up_to_date_t(branch_id, replier.get_business_card()));

                /* Wait for something to change. */
                wait_interruptible(listener.get_outdated_signal(), interruptor);
            } catch (typename listener_t<protocol_t>::backfiller_lost_exc_t) {
                /* We lost the backfiller which means we should retry, just
                 * going back to the top of the while loop accomplishes this.
                 * */
            }
        }
    } catch (interrupted_exc_t) {
        /* ignore */
    }
}

template<class protocol_t>
void reactor_t<protocol_t>::be_listener(typename protocol_t::region_t region, store_view_t<protocol_t> *store, signal_t *interruptor) THROWS_NOTHING {
    try {
        directory_entry_t directory_entry(this, region);
        while (true) {
            /* Tell everyone else that we're waiting for a broadcaster to
             * appear so that we can backfill. */
            directory_entry.set(typename reactor_business_card_t<protocal_t>::listener_without_primary_t());

            /* TODO we need to find a backfiller for listener_t */

            /* TODO: Backfill from most up-to-date peer if broadcaster is not
            found? */

            /* This actually finds the broadcaster that we will backfill from. */

            //TODO implement the function wait_for_broadcaster_to_appear_in_directory
            clone_ptr_t<directory_single_rview_t<boost::optional<broadcaster_business_card_t<protocol_t> > > > broadcaster =
                wait_for_broadcaster_to_appear_in_directory(interruptor);

            /* We've found a broadcaster tell everyone else that we're about to
             * begin backfilling. */
            directory_entry.set(typename reactor_business_card_t<protocal_t>::listener_backfilling_t());

            try {
                /* Construct a listener to receive the backfill. */
                listener_t<protocol_t> listener(mailbox_manager, branch_history, broadcaster, store, interruptor);

                /* We've completed backfilling, tell everyone that we're an up
                 * to date listener. (the architect will probably react to this
                 * by giving us a different role to fill .*/
                directory_entry.set(typename reactor_business_card_t<protocal_t>::listener_up_to_date_t());

                /* Wait for something to change (probably us getting a new role
                 * to fill). */
                wait_interruptible(listener.get_outdated_signal(), interruptor);
            } catch (typename listener_t<protocol_t>::backfiller_lost_exc_t) {
                /* We lost the backfiller which means we should retry, just
                 * going back to the top of the while loop accomplishes this.
                 * */
            }
        }
    } catch (interrupted_exc_t) {
        /* ignore */
    }
}

template<class protocol_t>
void reactor_t<protocol_t>::be_nothing(typename protocol_t::region_t region, store_view_t<protocol_t> *store, signal_t *interruptor) THROWS_NOTHING {
    try {
        directory_entry_t directory_entry(this, region);

        {
            /* We offer backfills while waiting for it to be safe to shutdown
             * in case another peer needs a copy of the data */
            backfiller_t<protocol_t> backfiller(mailbox_manager, branch_history, store);

            /* Tell the other peers that we are looking to shutdown and
             * offering backfilling until we do. */
            typename reactor_business_card_t<protocol_t>::nothing_when_safe_t activity(store_view->get_metadata(interruptor), backfiller.get_business_card());
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

            /* block until for each key: everything listed as a primary or secondary
            in the blueprint is listed as PRIMARY, SECONDARY, SECONDARY_LOST, or
            PRIMARY_SOON in the directory */
        }

        /* We now know that it's safe to shutdown so we tell the other peers
         * that we are beginning the process of erasing data. */
        directory_entry.set(typename reactor_business_card_t<protocol_t>::nothing_when_done_erasing_t());

        /* This actually erases the data. */
        store.reset_data(region, region_map_t<protocol_t>(region, version_range_t::zero()), store.new_write_token(), interruptor);

        /* Tell the other peers that we are officially nothing for this region,
         * end of story. */
        directory_entry.set(typename reactor_business_card_t<protocol_t>::nothing_t());

        interruptor->wait_lazily_unordered();
    } catch (interrupted_exc_t) {
        /* ignore */
    }
}

template <class protocol_t>
void reactor_t<protocol_t>::wait_for_directory_acks(directory_echo_version_t version_to_wait_on, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    blueprint_t<protocol_t> bp = blueprint->get();
    typename std::map<peer_id_t, std::map<typename protocol_t::region_t, typename blueprint_t<protocol_t>::role_t> >::iterator it = bp.peers.begin();
    for (it = bp.peers.begin(); it != bp.peers.end(); it++) {
        typename directory_echo_access_t<reactor_business_card_t<protocol_t> >::ack_waiter_t ack_waiter(&directory_echo_access, it->first, version_to_wait_on);
        wait_interruptible(&ack_waiter, interruptor);
    }

}

template <class protocol_t>
template <class activity_t>
clone_ptr_t<directory_single_rview_t<boost::optional<activity_t> > > reactor_t<protocol_t>::get_directory_entry_view(peer_id_t p_id, const reactor_activity_id_t &ra_id) {
    typedef read_lens_t<boost::optional<activity_t>, boost::optional<reactor_business_card_t<protocol_t> > > activity_read_lens_t;

    class activity_lens_t : public activity_read_lens_t {
    public:
        explicit activity_lens_t(const reactor_activity_id_t &_ra_id) 
            : ra_id(_ra_id)
        { }

        boost::optional<activity_t> get(const boost::optional<reactor_business_card_t<protocol_t> > &outer) const {
            if (!outer) {
                return boost::optional<activity_t>();
            } else if (outer.get().activities.find(ra_id) == outer.get().activities.end()) {
                return boost::optional<activity_t>();
            } else {
                try {
                    return boost::optional<activity_t>(boost::get<activity_t>(outer.get().activities.find(ra_id)->second.second));
                } catch (boost::bad_get) {
                    crash("Tried to get an activity of an unexpected type, it "
                            "is assumed the person calling this function knows "
                            "the type of the activity they will be getting "
                            "back.\n");
                }
            }
        }

        activity_lens_t *clone() const {
            return new activity_lens_t(ra_id);
        }

    private:
        reactor_activity_id_t ra_id;
    };

    return directory_echo_access.get_internal_view()->get_peer_view(p_id)->subview(clone_ptr_t<activity_read_lens_t>(new activity_lens_t(ra_id)));
}

