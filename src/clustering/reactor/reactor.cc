#include "clustering/reactor/reactor.hpp"

#include "clustering/immediate_consistency/branch/broadcaster.hpp"
#include "clustering/immediate_consistency/branch/replier.hpp"
#include "clustering/immediate_consistency/query/master.hpp"
#include "clustering/immediate_consistency/branch/multistore.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "concurrency/cross_thread_watchable.hpp"

template<class key_t, class value_t>
std::map<key_t, value_t> collapse_optionals_in_map(const std::map<key_t, boost::optional<value_t> > &map) {
    std::map<key_t, value_t> res;
    for (typename std::map<key_t, boost::optional<value_t> >::const_iterator it = map.begin(); it != map.end(); it++) {
        if (it->second) {
            res.insert(std::make_pair(it->first, it->second.get()));
        }
    }
    return res;
}

template<class protocol_t>
reactor_t<protocol_t>::reactor_t(
        io_backender_t *_io_backender,
        mailbox_manager_t *mm,
        typename master_t<protocol_t>::ack_checker_t *ack_checker_,
        clone_ptr_t<watchable_t<std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t<protocol_t> > > > > > > rd,
        branch_history_manager_t<protocol_t> *bhm,
        clone_ptr_t<watchable_t<blueprint_t<protocol_t> > > b,
        multistore_ptr_t<protocol_t> *_underlying_svs,
        perfmon_collection_t *_parent_perfmon_collection,
        typename protocol_t::context_t *_ctx) THROWS_NOTHING :
    io_backender(_io_backender),
    mailbox_manager(mm),
    ack_checker(ack_checker_),
    directory_echo_writer(mailbox_manager, cow_ptr_t<reactor_business_card_t<protocol_t> >()),
    directory_echo_mirror(mailbox_manager, rd->subview(&collapse_optionals_in_map<peer_id_t, directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t<protocol_t> > > >)),
    branch_history_manager(bhm),
    blueprint_watchable(b),
    underlying_svs(_underlying_svs),
    blueprint_subscription(boost::bind(&reactor_t<protocol_t>::on_blueprint_changed, this)),
    parent_perfmon_collection(_parent_perfmon_collection),
    regions_perfmon_collection(),
    regions_perfmon_membership(parent_perfmon_collection, &regions_perfmon_collection, "regions"),
    ctx(_ctx)
{
    {
        typename watchable_t<blueprint_t<protocol_t> >::freeze_t freeze(blueprint_watchable);
        blueprint_watchable->get().assert_valid();
        try_spawn_roles();
        blueprint_subscription.reset(blueprint_watchable, &freeze);
    }
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
    rassertf(!reactor_activity_id.is_nil(), "This method should only be called when an activity has already been set\n");
    typename directory_echo_writer_t<cow_ptr_t<reactor_business_card_t<protocol_t> > >::our_value_change_t our_value_change(&parent->directory_echo_writer);
    {
        typename cow_ptr_t<reactor_business_card_t<protocol_t> >::change_t cow_ptr_change(&our_value_change.buffer);
        cow_ptr_change.get()->activities[reactor_activity_id].activity = activity;
    }
    return our_value_change.commit();
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
    blueprint.assert_valid();
    rassertf(std_contains(blueprint.peers_roles, get_me()), "reactor_t assumes that it is mentioned in the blueprint it's given.");

    std::map<typename protocol_t::region_t, blueprint_role_t> blueprint_roles =
        blueprint.peers_roles.find(get_me())->second;
    for (typename std::map<typename protocol_t::region_t, current_role_t *>::iterator it = current_roles.begin();
            it != current_roles.end(); it++) {
        typename std::map<typename protocol_t::region_t, blueprint_role_t>::iterator it2 =
            blueprint_roles.find((*it).first);
        if (it2 == blueprint_roles.end()) {
            /* The shard boundaries have changed, and the shard that the running
            coroutine was for no longer exists; interrupt it */
            it->second->abort.pulse_if_not_already_pulsed();
        } else {
            if (it->second->role != it2->second) {
                /* Our role for the shard has changed; interrupt the running
                coroutine */
                it->second->abort.pulse_if_not_already_pulsed();
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
    rassertf(std_contains(blueprint.peers_roles, get_me()), "reactor_t assumes that it is mentioned in the blueprint it's given.");

    std::map<typename protocol_t::region_t, blueprint_role_t> blueprint_roles =
        blueprint.peers_roles.find(get_me())->second;
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
        signal_t *interruptor) THROWS_NOTHING {
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
        wait_any_t wait_any(&role->abort, keepalive.get_drain_signal());

        // guarantee(CLUSTER_CPU_SHARDING_FACTOR == svs_subview.num_stores());

        pmap(svs_subview.num_stores(), boost::bind(&reactor_t<protocol_t>::run_cpu_sharded_role, this, _1, role, region, &svs_subview, &wait_any));
    }

    //As promised, clean up the state from try_spawn_roles
    current_roles.erase(region);
    delete role;

    if (!keepalive.get_drain_signal()->is_pulsed()) {
        try_spawn_roles();
    }
}

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
