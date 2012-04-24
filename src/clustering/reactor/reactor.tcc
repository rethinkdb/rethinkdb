#ifndef CLUSTERING_REACTOR_REACTOR_TCC_
#define CLUSTERING_REACTOR_REACTOR_TCC_

#include "clustering/immediate_consistency/branch/broadcaster.hpp"
#include "clustering/immediate_consistency/branch/replier.hpp"
#include "clustering/immediate_consistency/query/master.hpp"

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
        mailbox_manager_t *mm,
        typename master_t<protocol_t>::ack_checker_t *ack_checker_,
        clone_ptr_t<watchable_t<std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > > > > rd,
        boost::shared_ptr<semilattice_readwrite_view_t<branch_history_t<protocol_t> > > bh,
        clone_ptr_t<watchable_t<blueprint_t<protocol_t> > > b,
        store_view_t<protocol_t> *_underlying_store) THROWS_NOTHING :
    mailbox_manager(mm),
    ack_checker(ack_checker_),
    reactor_directory(rd),
    directory_echo_writer(mailbox_manager, reactor_business_card_t<protocol_t>()),
    directory_echo_mirror(mailbox_manager, rd->subview(&collapse_optionals_in_map<peer_id_t, directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > >)),
    branch_history(bh),
    master_directory(std::map<master_id_t, master_business_card_t<protocol_t> >()),
    blueprint_watchable(b), underlying_store(_underlying_store),
    blueprint_subscription(boost::bind(&reactor_t<protocol_t>::on_blueprint_changed, this))
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
    : parent(_parent), region(_region), reactor_activity_id(boost::uuids::nil_generator()())
{ }

template <class protocol_t>
directory_echo_version_t reactor_t<protocol_t>::directory_entry_t::set(typename reactor_business_card_t<protocol_t>::activity_t activity) {
    typename directory_echo_writer_t<reactor_business_card_t<protocol_t> >::our_value_change_t our_value_change(&parent->directory_echo_writer);
    if (!reactor_activity_id.is_nil()) {
        our_value_change.buffer.activities.erase(reactor_activity_id);
    }
    reactor_activity_id = generate_uuid();
    our_value_change.buffer.activities.insert(std::make_pair(reactor_activity_id, std::make_pair(region, activity)));
    return our_value_change.commit();
}

template <class protocol_t>
directory_echo_version_t reactor_t<protocol_t>::directory_entry_t::update_without_changing_id(typename reactor_business_card_t<protocol_t>::activity_t activity) {
    typename directory_echo_writer_t<reactor_business_card_t<protocol_t> >::our_value_change_t our_value_change(&parent->directory_echo_writer);
    rassert(!reactor_activity_id.is_nil(), "This method should only be called when an activity has already been set\n");

    our_value_change.buffer.activities[reactor_activity_id].second = activity;
    return our_value_change.commit();
}

template <class protocol_t>
reactor_t<protocol_t>::directory_entry_t::~directory_entry_t() {
    if (!reactor_activity_id.is_nil()) {
        typename directory_echo_writer_t<reactor_business_card_t<protocol_t> >::our_value_change_t our_value_change(&parent->directory_echo_writer);
        our_value_change.buffer.activities.erase(reactor_activity_id);
        our_value_change.commit();
    }
}

template<class protocol_t>
void reactor_t<protocol_t>::on_blueprint_changed() THROWS_NOTHING {
    blueprint_watchable->get().assert_valid();
    rassert(std_contains(blueprint_watchable->get().peers_roles, get_me()), "reactor_t assumes that it is mentioned in the blueprint it's given.");

    std::map<typename protocol_t::region_t, typename blueprint_details::role_t> blueprint_roles =
        (*blueprint_watchable->get().peers_roles.find(get_me())).second;
    typename std::map<
            typename protocol_t::region_t,
            std::pair<typename blueprint_details::role_t, cond_t *>
            >::iterator it;
    for (it = current_roles.begin(); it != current_roles.end(); it++) {
        typename std::map<typename protocol_t::region_t, blueprint_details::role_t>::iterator it2 =
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
    blueprint_t<protocol_t> blueprint = blueprint_watchable->get();
    rassert(std_contains(blueprint.peers_roles, get_me()), "reactor_t assumes that it is mentioned in the blueprint it's given.");

    std::map<typename protocol_t::region_t, typename blueprint_details::role_t> blueprint_roles =
        (*blueprint.peers_roles.find(get_me())).second;
    typename std::map<typename protocol_t::region_t, blueprint_details::role_t>::iterator it;
    for (it = blueprint_roles.begin(); it != blueprint_roles.end(); it++) {
        bool none_overlap = true;
        typename std::map<
            typename protocol_t::region_t,
            std::pair<typename blueprint_details::role_t, cond_t *>
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
                                               it->second, blueprint_changed_cond, blueprint, auto_drainer_t::lock_t(&drainer)));
        }
    }
}

template<class protocol_t>
void reactor_t<protocol_t>::run_role(
        typename protocol_t::region_t region,
        typename blueprint_details::role_t role,
        cond_t *blueprint_changed_cond,
        const blueprint_t<protocol_t> &blueprint,
        auto_drainer_t::lock_t keepalive) THROWS_NOTHING {

    //A store_view_t derived object that acts as a store for the specified region
    store_subview_t<protocol_t> store_subview(underlying_store, region);

    {
        //All of the be_{role} functions respond identically to blueprint changes
        //and interruptions... so we just unify those signals
        wait_any_t wait_any(blueprint_changed_cond, keepalive.get_drain_signal());

        switch (role) {
            case blueprint_details::role_primary:
                be_primary(region, &store_subview, blueprint, &wait_any);
                break;
            case blueprint_details::role_secondary:
                be_secondary(region, &store_subview, blueprint, &wait_any);
                break;
            case blueprint_details::role_nothing:
                be_nothing(region, &store_subview, blueprint, &wait_any);
                break;
            default:
                unreachable();
                break;
        }
    }

    //As promised, clean up the state from try_spawn_roles
    current_roles.erase(region);
    delete blueprint_changed_cond;

    if (!keepalive.get_drain_signal()->is_pulsed()) {
        try_spawn_roles();
    }
}

template<class protocol_t>
boost::optional<boost::optional<broadcaster_business_card_t<protocol_t> > > reactor_t<protocol_t>::extract_broadcaster_from_reactor_business_card_primary(
        const boost::optional<boost::optional<typename reactor_business_card_t<protocol_t>::primary_t> > &bcard) {
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
    blueprint_t<protocol_t> bp = blueprint_watchable->get();
    typename std::map<peer_id_t, std::map<typename protocol_t::region_t, typename blueprint_details::role_t> >::iterator it = bp.peers_roles.begin();
    for (it = bp.peers_roles.begin(); it != bp.peers_roles.end(); it++) {
        typename directory_echo_writer_t<reactor_business_card_t<protocol_t> >::ack_waiter_t ack_waiter(&directory_echo_writer, it->first, version_to_wait_on);
        wait_interruptible(&ack_waiter, interruptor);
    }
}

template<class protocol_t, class activity_t>
boost::optional<boost::optional<activity_t> > extract_activity_from_reactor_bcard(const std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > > &bcards, peer_id_t p_id, const reactor_activity_id_t &ra_id) {
    typename std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > >::const_iterator it = bcards.find(p_id);
    if (it == bcards.end()) {
        return boost::optional<boost::optional<activity_t> >();
    }
    if (!it->second) {
        return boost::optional<boost::optional<activity_t> >(boost::optional<activity_t>());
    }
    typename reactor_business_card_t<protocol_t>::activity_map_t::const_iterator jt = it->second->internal.activities.find(ra_id);
    if (jt == it->second->internal.activities.end()) {
        return boost::optional<boost::optional<activity_t> >(boost::optional<activity_t>());
    }
    try {
        return boost::optional<boost::optional<activity_t> >(boost::optional<activity_t>(boost::get<activity_t>(jt->second.second)));
    } catch (boost::bad_get) {
        crash("Tried to get an activity of an unexpected type, it is assumed "
            "the person calling this function knows the type of the activity "
            "they will be getting back.\n");
    }
}

template <class protocol_t>
template <class activity_t>
clone_ptr_t<watchable_t<boost::optional<boost::optional<activity_t> > > > reactor_t<protocol_t>::get_directory_entry_view(peer_id_t p_id, const reactor_activity_id_t &ra_id) {
    return reactor_directory->subview(boost::bind(&extract_activity_from_reactor_bcard<protocol_t, activity_t>, _1, p_id, ra_id));
}

#endif
