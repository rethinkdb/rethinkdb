// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "clustering/reactor/reactor.hpp"

#include "clustering/immediate_consistency/branch/broadcaster.hpp"
#include "clustering/immediate_consistency/branch/replier.hpp"
#include "clustering/immediate_consistency/query/master.hpp"
#include "clustering/immediate_consistency/branch/multistore.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "concurrency/cross_thread_watchable.hpp"
#include "config/args.hpp"
#include "stl_utils.hpp"

template<class key_t, class value_t>
bool collapse_optionals_in_map(const change_tracking_map_t<key_t, boost::optional<value_t> > &map, change_tracking_map_t<key_t, value_t> *current_out) {
    guarantee(current_out != NULL);
    bool anything_changed = false;
    const bool do_init = current_out->get_current_version() == 0;
    std::set<key_t> keys_to_update;
    current_out->begin_version();
    if (do_init) {
        for (auto it = map.get_inner().begin(); it != map.get_inner().end(); ++it) {
            keys_to_update.insert(it->first);
        }
        anything_changed = true;
    } else {
        keys_to_update = map.get_changed_keys();
    }
    for (auto it = keys_to_update.begin(); it != keys_to_update.end(); it++) {
        auto existing_it = current_out->get_inner().find(*it);
        auto jt = map.get_inner().find(*it);
        if (jt != map.get_inner().end() && jt->second) {
            // Check if the new value is actually different from the old one
            bool has_changed = existing_it == current_out->get_inner().end()
                || !(existing_it->second == jt->second.get());
            if (has_changed) {
                current_out->set_value(*it, jt->second.get());
                anything_changed = true;
            }
        } else if (existing_it != current_out->get_inner().end()) {
            current_out->delete_value(*it);
            anything_changed = true;
        }
    }
    return anything_changed;
}

template<class protocol_t>
reactor_t<protocol_t>::reactor_t(
        const base_path_t& _base_path,
        io_backender_t *_io_backender,
        mailbox_manager_t *mm,
        ack_checker_t *ack_checker_,
        clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, boost::optional<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t<protocol_t> > > > > > > rd,
        branch_history_manager_t<protocol_t> *bhm,
        clone_ptr_t<watchable_t<blueprint_t<protocol_t> > > b,
        multistore_ptr_t<protocol_t> *_underlying_svs,
        perfmon_collection_t *_parent_perfmon_collection,
        typename protocol_t::context_t *_ctx) THROWS_NOTHING :
    base_path(_base_path),
    parent_perfmon_collection(_parent_perfmon_collection),
    regions_perfmon_collection(),
    regions_perfmon_membership(parent_perfmon_collection, &regions_perfmon_collection, "regions"),
    io_backender(_io_backender),
    mailbox_manager(mm),
    ack_checker(ack_checker_),
    directory_echo_writer(mailbox_manager, cow_ptr_t<reactor_business_card_t<protocol_t> >()),
    directory_echo_mirror(mailbox_manager, rd->template incremental_subview<
        change_tracking_map_t<peer_id_t, directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t<protocol_t> > > > > (
            &collapse_optionals_in_map<peer_id_t, directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t<protocol_t> > > >)),
    branch_history_manager(bhm),
    blueprint_watchable(b),
    underlying_svs(_underlying_svs),
    blueprint_subscription(boost::bind(&reactor_t<protocol_t>::on_blueprint_changed, this)),
    ctx(_ctx)
{
    with_priority_t p(CORO_PRIORITY_REACTOR);
    {
        typename watchable_t<blueprint_t<protocol_t> >::freeze_t freeze(blueprint_watchable);
        blueprint_watchable->get().guarantee_valid();
        try_spawn_roles();
        blueprint_subscription.reset(blueprint_watchable, &freeze);
    }
}

template <class protocol_t>
clone_ptr_t<watchable_t<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t<protocol_t> > > > > reactor_t<protocol_t>::get_reactor_directory() {
    return directory_echo_writer.get_watchable();
}

template <class protocol_t>
reactor_t<protocol_t>::directory_entry_t::directory_entry_t(reactor_t<protocol_t> *_parent, typename protocol_t::region_t _region)
    : parent(_parent), region(_region), reactor_activity_id(nil_uuid())
{ }

template <class protocol_t>
directory_echo_version_t reactor_t<protocol_t>::directory_entry_t::set(typename reactor_business_card_t<protocol_t>::activity_t activity) {
    typename directory_echo_writer_t<cow_ptr_t<reactor_business_card_t<protocol_t> > >::our_value_change_t our_value_change(&parent->directory_echo_writer);
    {
        typename cow_ptr_t<reactor_business_card_t<protocol_t> >::change_t cow_ptr_change(&our_value_change.buffer);
        if (!reactor_activity_id.is_nil()) {
            cow_ptr_change.get()->activities.erase(reactor_activity_id);
        }
        reactor_activity_id = generate_uuid();
        cow_ptr_change.get()->activities.insert(std::make_pair(reactor_activity_id, typename reactor_business_card_t<protocol_t>::activity_entry_t(region, activity)));
    }
    return our_value_change.commit();
}

template <class protocol_t>
directory_echo_version_t reactor_t<protocol_t>::directory_entry_t::update_without_changing_id(typename reactor_business_card_t<protocol_t>::activity_t activity) {
    guarantee(!reactor_activity_id.is_nil(), "This method should only be called when an activity has already been set\n");
    typename directory_echo_writer_t<cow_ptr_t<reactor_business_card_t<protocol_t> > >::our_value_change_t our_value_change(&parent->directory_echo_writer);
    {
        typename cow_ptr_t<reactor_business_card_t<protocol_t> >::change_t cow_ptr_change(&our_value_change.buffer);
        cow_ptr_change.get()->activities[reactor_activity_id].activity = activity;
    }
    return our_value_change.commit();
}

template <class protocol_t>
reactor_activity_id_t reactor_t<protocol_t>::directory_entry_t::get_reactor_activity_id() const {
    return reactor_activity_id;
}

template <class protocol_t>
reactor_t<protocol_t>::current_role_t::current_role_t(blueprint_role_t r, const blueprint_t<protocol_t> &b)
    : role(r), blueprint(b) { }

template <class protocol_t>
peer_id_t reactor_t<protocol_t>::get_me() THROWS_NOTHING {
    return mailbox_manager->get_connectivity_service()->get_me();
}

template <class protocol_t>
reactor_t<protocol_t>::directory_entry_t::~directory_entry_t() {
    if (!reactor_activity_id.is_nil()) {
        typename directory_echo_writer_t<cow_ptr_t<reactor_business_card_t<protocol_t> > >::our_value_change_t our_value_change(&parent->directory_echo_writer);
        {
            typename cow_ptr_t<reactor_business_card_t<protocol_t> >::change_t cow_ptr_change(&our_value_change.buffer);
            cow_ptr_change.get()->activities.erase(reactor_activity_id);
        }
        our_value_change.commit();
    }
}

template<class protocol_t>
void reactor_t<protocol_t>::on_blueprint_changed() THROWS_NOTHING {
    blueprint_t<protocol_t> blueprint = blueprint_watchable->get();
    blueprint.guarantee_valid();

    typename std::map<peer_id_t, std::map<typename protocol_t::region_t, blueprint_role_t> >::const_iterator role_it = blueprint.peers_roles.find(get_me());
    guarantee(role_it != blueprint.peers_roles.end(), "reactor_t assumes that it is mentioned in the blueprint it's given.");

    std::map<typename protocol_t::region_t, blueprint_role_t> blueprint_roles = role_it->second;
    for (typename std::map<typename protocol_t::region_t, current_role_t *>::iterator it = current_roles.begin();
         it != current_roles.end(); ++it) {
        typename std::map<typename protocol_t::region_t, blueprint_role_t>::iterator it2 =
            blueprint_roles.find(it->first);
        if (it2 == blueprint_roles.end()) {
            /* The shard boundaries have changed, and the shard that the running
            coroutine was for no longer exists; interrupt it */
            it->second->abort_roles.pulse_if_not_already_pulsed();
        } else {
            if (it->second->role != it2->second) {
                /* Our role for the shard has changed; interrupt the running
                coroutine */
                it->second->abort_roles.pulse_if_not_already_pulsed();
            } else {
                /* Notify the running coroutine of the new blueprint */
                it->second->blueprint.set_value(blueprint);
            }
        }
    }
}

template<class protocol_t>
void reactor_t<protocol_t>::try_spawn_roles() THROWS_NOTHING {
    blueprint_t<protocol_t> blueprint = blueprint_watchable->get();

    typename std::map<peer_id_t, std::map<typename protocol_t::region_t, blueprint_role_t> >::const_iterator role_it = blueprint.peers_roles.find(get_me());
    guarantee(role_it != blueprint.peers_roles.end(), "reactor_t assumes that it is mentioned in the blueprint it's given.");

    std::map<typename protocol_t::region_t, blueprint_role_t> blueprint_roles = role_it->second;
    for (typename std::map<typename protocol_t::region_t, blueprint_role_t>::iterator it = blueprint_roles.begin();
         it != blueprint_roles.end(); ++it) {
        bool none_overlap = true;
        for (typename std::map<typename protocol_t::region_t, current_role_t *>::iterator it2 = current_roles.begin();
             it2 != current_roles.end(); ++it2) {
            if (region_overlaps(it->first, it2->first)) {
                none_overlap = false;
                break;
            }
        }

        if (none_overlap) {
            //This state will be cleaned up in run_role
            current_role_t *role = new current_role_t(it->second, blueprint);
            current_roles.insert(std::make_pair(it->first, role));
            coro_t::spawn_sometime(boost::bind(&reactor_t<protocol_t>::run_role, this, it->first,
                                               role, auto_drainer_t::lock_t(&drainer)));
        }
    }
}

template<class protocol_t>
void reactor_t<protocol_t>::run_cpu_sharded_role(
        int cpu_shard_number,
        current_role_t *role,
        const typename protocol_t::region_t& region,
        multistore_ptr_t<protocol_t> *svs_subview,
        signal_t *interruptor,
        cond_t *abort_roles) THROWS_NOTHING {
    store_view_t<protocol_t> *store_view = svs_subview->get_store(cpu_shard_number);
    typename protocol_t::region_t cpu_sharded_region = region_intersection(region, protocol_t::cpu_sharding_subspace(cpu_shard_number, svs_subview->num_stores()));

    switch (role->role) {
    case blueprint_role_primary:
        be_primary(cpu_sharded_region, store_view, role->blueprint.get_watchable(), interruptor);
        break;
    case blueprint_role_secondary:
        be_secondary(cpu_sharded_region, store_view, role->blueprint.get_watchable(), interruptor);
        break;
    case blueprint_role_nothing:
        be_nothing(cpu_sharded_region, store_view, role->blueprint.get_watchable(), interruptor);
        break;
    default:
        unreachable();
        break;
    }

    // When one role returns, make sure all others are aborted
    if (!abort_roles->is_pulsed()) {
        abort_roles->pulse();
    }
}

template<class protocol_t>
bool we_see_our_bcard(const change_tracking_map_t<peer_id_t, cow_ptr_t<reactor_business_card_t<protocol_t> > > &bcards, peer_id_t me) {
    return std_contains(bcards.get_inner(), me);
}

template<class protocol_t>
void reactor_t<protocol_t>::run_role(
        typename protocol_t::region_t region,
        current_role_t *role,
        auto_drainer_t::lock_t keepalive) THROWS_NOTHING {

    //A store_view_t derived object that acts as a store for the specified region
    multistore_ptr_t<protocol_t> svs_subview(underlying_svs, region);

    {
        //All of the be_{role} functions respond identically to blueprint changes
        //and interruptions... so we just unify those signals
        wait_any_t wait_any(&role->abort_roles, keepalive.get_drain_signal());

        try {
            /* Notice this is necessary because the reactor first creates the
             * reactor and then inserts its business card in to the map However
             * we had an issue (#1132) in which reactor_be_primary was making
             * the assumption that the bcard would be set while it was run. We
             * decided that it was a good idea to have this actually be a
             * correct assumption. The below line waits until the bcard shows
             * up in the directory thus make sure that the bcard is in the
             * directory before the be_role functions get called. */
            directory_echo_mirror.get_internal()->run_until_satisfied(
                boost::bind(&we_see_our_bcard<protocol_t>, _1, get_me()), &wait_any,
                REACTOR_RUN_UNTIL_SATISFIED_NAP);
            // guarantee(CLUSTER_CPU_SHARDING_FACTOR == svs_subview.num_stores());

            pmap(svs_subview.num_stores(), boost::bind(&reactor_t<protocol_t>::run_cpu_sharded_role, this, _1, role, region, &svs_subview, &wait_any, &role->abort_roles));
        } catch (const interrupted_exc_t &) {
        }
    }

    //As promised, clean up the state from try_spawn_roles
    current_roles.erase(region);
    delete role;

    if (!keepalive.get_drain_signal()->is_pulsed()) {
        try_spawn_roles();
    }
}

template <class protocol_t>
reactor_t<protocol_t>::backfill_candidate_t::backfill_location_t::backfill_location_t(const backfiller_bcard_view_t &b, peer_id_t p, reactor_activity_id_t i)
    : backfiller(b), peer_id(p), activity_id(i) { }

template<class protocol_t>
boost::optional<boost::optional<broadcaster_business_card_t<protocol_t> > > reactor_t<protocol_t>::extract_broadcaster_from_reactor_business_card_primary(const boost::optional<boost::optional<typename reactor_business_card_t<protocol_t>::primary_t> > &bcard) {
    if (!bcard) {
        return boost::optional<boost::optional<broadcaster_business_card_t<protocol_t> > >();
    }
    if (!bcard.get()) {
        return boost::optional<boost::optional<broadcaster_business_card_t<protocol_t> > >(
            boost::optional<broadcaster_business_card_t<protocol_t> >());
    }
    return boost::optional<boost::optional<broadcaster_business_card_t<protocol_t> > >(
        boost::optional<broadcaster_business_card_t<protocol_t> >(bcard.get().get().broadcaster));
}

template <class protocol_t>
void reactor_t<protocol_t>::wait_for_directory_acks(directory_echo_version_t version_to_wait_on, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    while (true) {
        /* This function waits for acks from all the peers mentioned in the
        blueprint. If the blueprint changes while we're waiting for acks, we
        restart from the top. This is important because otherwise we might
        deadlock. For example, if we were waiting for a machine to come back up
        and then it was declared dead, our interruptor might not be pulsed but
        the `ack_waiter_t` would never be pulsed so we would get stuck. */
        cond_t blueprint_changed;
        blueprint_t<protocol_t> bp;
        typename watchable_t<blueprint_t<protocol_t> >::subscription_t subscription(
            boost::bind(&cond_t::pulse_if_not_already_pulsed, &blueprint_changed));
        {
            typename watchable_t<blueprint_t<protocol_t> >::freeze_t freeze(blueprint_watchable);
            bp = blueprint_watchable->get();
            subscription.reset(blueprint_watchable, &freeze);
        }
        typename std::map<peer_id_t, std::map<typename protocol_t::region_t, blueprint_role_t> >::iterator it = bp.peers_roles.begin();
        for (it = bp.peers_roles.begin(); it != bp.peers_roles.end(); it++) {
            typename directory_echo_writer_t<cow_ptr_t<reactor_business_card_t<protocol_t> > >::ack_waiter_t ack_waiter(&directory_echo_writer, it->first, version_to_wait_on);
            wait_any_t waiter(&ack_waiter, &blueprint_changed);
            wait_interruptible(&waiter, interruptor);
            if (blueprint_changed.is_pulsed()) {
                break;
            }
        }
        if (!blueprint_changed.is_pulsed()) {
            break;
        }
    }
}






#include "mock/dummy_protocol.hpp"
#include "memcached/protocol.hpp"
#include "rdb_protocol/protocol.hpp"

template class reactor_t<mock::dummy_protocol_t>;
template class reactor_t<memcached_protocol_t>;
template class reactor_t<rdb_protocol_t>;
