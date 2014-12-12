// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "clustering/reactor/reactor.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "clustering/immediate_consistency/branch/broadcaster.hpp"
#include "clustering/immediate_consistency/branch/replier.hpp"
#include "clustering/immediate_consistency/query/master.hpp"
#include "clustering/immediate_consistency/branch/multistore.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "concurrency/cross_thread_watchable.hpp"
#include "config/args.hpp"
#include "containers/scoped.hpp"
#include "stl_utils.hpp"

/* The nap time of 200 ms was determined experimentally as follows: In the specific test,
a 200 ms nap provided a high speed up when resharding a table in a cluster of 64 nodes.
Higher values improved the efficiency of the operation only marginally, while
unnecessarily slowing down directory changes in smaller clusters. */
constexpr int64_t directory_buffer_nap = 200;

reactor_t::reactor_t(
        const base_path_t& _base_path,
        io_backender_t *_io_backender,
        mailbox_manager_t *mm,
        const server_id_t &sid,
        backfill_throttler_t *backfill_throttler_,
        ack_checker_t *ack_checker_,
        watchable_map_t<
            peer_id_t, 
            directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t> > > *rd,
        branch_history_manager_t *bhm,
        clone_ptr_t<watchable_t<blueprint_t> > b,
        multistore_ptr_t *_underlying_svs,
        perfmon_collection_t *_parent_perfmon_collection,
        rdb_context_t *_ctx) THROWS_NOTHING :
    base_path(_base_path),
    parent_perfmon_collection(_parent_perfmon_collection),
    regions_perfmon_collection(),
    regions_perfmon_membership(parent_perfmon_collection, &regions_perfmon_collection, "regions"),
    io_backender(_io_backender),
    mailbox_manager(mm),
    server_id(sid),
    backfill_throttler(backfill_throttler_),
    ack_checker(ack_checker_),
    directory_echo_writer(mailbox_manager, cow_ptr_t<reactor_business_card_t>()),
    directory_buffer(directory_echo_writer.get_watchable(), directory_buffer_nap),
    directory_echo_mirror(mailbox_manager, rd),
    branch_history_manager(bhm),
    blueprint_watchable(b),
    underlying_svs(_underlying_svs),
    blueprint_subscription(boost::bind(&reactor_t::on_blueprint_changed, this)),
    ctx(_ctx)
{
    with_priority_t p(CORO_PRIORITY_REACTOR);
    {
        watchable_t<blueprint_t>::freeze_t freeze(blueprint_watchable);
        blueprint_watchable->get().guarantee_valid();
        try_spawn_roles();
        blueprint_subscription.reset(blueprint_watchable, &freeze);
    }
}

clone_ptr_t<watchable_t<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t> > > >
reactor_t::get_reactor_directory() {
    return directory_buffer.get_output();
}

reactor_t::directory_entry_t::directory_entry_t(reactor_t *_parent, region_t _region)
    : parent(_parent), region(_region), reactor_activity_id(nil_uuid())
{ }

directory_echo_version_t reactor_t::directory_entry_t::set(reactor_business_card_t::activity_t activity) {
    directory_echo_writer_t<cow_ptr_t<reactor_business_card_t> >::our_value_change_t our_value_change(&parent->directory_echo_writer);
    {
        cow_ptr_t<reactor_business_card_t>::change_t cow_ptr_change(&our_value_change.buffer);
        if (!reactor_activity_id.is_nil()) {
            cow_ptr_change.get()->activities.erase(reactor_activity_id);
        }
        reactor_activity_id = generate_uuid();
        cow_ptr_change.get()->activities.insert(std::make_pair(reactor_activity_id, reactor_business_card_t::activity_entry_t(region, activity)));
    }
    return our_value_change.commit();
}

directory_echo_version_t reactor_t::directory_entry_t::update_without_changing_id(reactor_business_card_t::activity_t activity) {
    guarantee(!reactor_activity_id.is_nil(), "This method should only be called when an activity has already been set\n");
    directory_echo_writer_t<cow_ptr_t<reactor_business_card_t> >::our_value_change_t our_value_change(&parent->directory_echo_writer);
    {
        cow_ptr_t<reactor_business_card_t>::change_t cow_ptr_change(&our_value_change.buffer);
        cow_ptr_change.get()->activities[reactor_activity_id].activity = activity;
    }
    return our_value_change.commit();
}

reactor_activity_id_t reactor_t::directory_entry_t::get_reactor_activity_id() const {
    return reactor_activity_id;
}

reactor_t::current_role_t::current_role_t(blueprint_role_t r, const blueprint_t &b)
    : role(r), blueprint(b) { }

peer_id_t reactor_t::get_me() THROWS_NOTHING {
    return mailbox_manager->get_connectivity_cluster()->get_me();
}

reactor_t::directory_entry_t::~directory_entry_t() {
    if (!reactor_activity_id.is_nil()) {
        directory_echo_writer_t<cow_ptr_t<reactor_business_card_t> >::our_value_change_t our_value_change(&parent->directory_echo_writer);
        {
            cow_ptr_t<reactor_business_card_t>::change_t cow_ptr_change(&our_value_change.buffer);
            cow_ptr_change.get()->activities.erase(reactor_activity_id);
        }
        our_value_change.commit();
    }
}

void reactor_t::on_blueprint_changed() THROWS_NOTHING {
    blueprint_t blueprint = blueprint_watchable->get();
    blueprint.guarantee_valid();

    std::map<peer_id_t, std::map<region_t, blueprint_role_t> >::const_iterator role_it = blueprint.peers_roles.find(get_me());
    guarantee(role_it != blueprint.peers_roles.end(), "reactor_t assumes that it is mentioned in the blueprint it's given.");

    std::map<region_t, blueprint_role_t> blueprint_roles = role_it->second;
    for (std::map<region_t, current_role_t *>::iterator it = current_roles.begin();
         it != current_roles.end(); ++it) {
        std::map<region_t, blueprint_role_t>::iterator it2 =
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

void reactor_t::try_spawn_roles() THROWS_NOTHING {
    blueprint_t blueprint = blueprint_watchable->get();

    std::map<peer_id_t, std::map<region_t, blueprint_role_t> >::const_iterator role_it = blueprint.peers_roles.find(get_me());
    guarantee(role_it != blueprint.peers_roles.end(), "reactor_t assumes that it is mentioned in the blueprint it's given.");

    std::map<region_t, blueprint_role_t> blueprint_roles = role_it->second;
    for (std::map<region_t, blueprint_role_t>::iterator it = blueprint_roles.begin();
         it != blueprint_roles.end(); ++it) {
        bool none_overlap = true;
        for (std::map<region_t, current_role_t *>::iterator it2 = current_roles.begin();
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
            coro_t::spawn_sometime(boost::bind(&reactor_t::run_role, this, it->first,
                                               role, auto_drainer_t::lock_t(&drainer)));
        }
    }
}

void reactor_t::run_cpu_sharded_role(
        int cpu_shard_number,
        current_role_t *role,
        const region_t& region,
        multistore_ptr_t *svs_subview,
        signal_t *interruptor,
        cond_t *abort_roles) THROWS_NOTHING {
    store_view_t *store_view = svs_subview->get_store(cpu_shard_number);
    region_t cpu_sharded_region = region_intersection(region, rdb_protocol::cpu_sharding_subspace(cpu_shard_number, svs_subview->num_stores()));

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

void reactor_t::run_role(
        region_t region,
        current_role_t *role,
        auto_drainer_t::lock_t keepalive) THROWS_NOTHING {

    //A store_view_t derived object that acts as a store for the specified region
    multistore_ptr_t svs_subview(underlying_svs, region);

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
            directory_echo_mirror.get_internal()->run_key_until_satisfied(
                get_me(),
                [](const cow_ptr_t<reactor_business_card_t> *maybe_bcard) {
                    return (maybe_bcard != nullptr);
                },
                &wait_any);
            // guarantee(CLUSTER_CPU_SHARDING_FACTOR == svs_subview.num_stores());

            pmap(svs_subview.num_stores(), boost::bind(&reactor_t::run_cpu_sharded_role, this, _1, role, region, &svs_subview, &wait_any, &role->abort_roles));
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

reactor_t::backfill_candidate_t::backfill_location_t::backfill_location_t(const backfiller_bcard_view_t &b, peer_id_t p, reactor_activity_id_t i)
    : backfiller(b), peer_id(p), activity_id(i) { }

boost::optional<boost::optional<broadcaster_business_card_t> > reactor_t::extract_broadcaster_from_reactor_business_card_primary(const boost::optional<boost::optional<reactor_business_card_t::primary_t> > &bcard) {
    if (!bcard) {
        return boost::optional<boost::optional<broadcaster_business_card_t> >();
    }
    if (!bcard.get()) {
        return boost::optional<boost::optional<broadcaster_business_card_t> >(
            boost::optional<broadcaster_business_card_t>());
    }
    return boost::optional<boost::optional<broadcaster_business_card_t> >(
        boost::optional<broadcaster_business_card_t>(bcard.get().get().broadcaster));
}

void reactor_t::wait_for_directory_acks(directory_echo_version_t version_to_wait_on, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    while (true) {
        /* This function waits for acks from all the peers mentioned in the
        blueprint. If the blueprint changes while we're waiting for acks, we
        restart from the top. This is important because otherwise we might
        deadlock. For example, if we were waiting for a server to come back up
        and then it was declared dead, our interruptor might not be pulsed but
        the `ack_waiter_t` would never be pulsed so we would get stuck. */
        cond_t blueprint_changed;
        blueprint_t bp;
        watchable_t<blueprint_t>::subscription_t subscription(
            boost::bind(&cond_t::pulse_if_not_already_pulsed, &blueprint_changed));
        {
            watchable_t<blueprint_t>::freeze_t freeze(blueprint_watchable);
            bp = blueprint_watchable->get();
            subscription.reset(blueprint_watchable, &freeze);
        }
        std::map<peer_id_t, std::map<region_t, blueprint_role_t> >::iterator it = bp.peers_roles.begin();
        for (it = bp.peers_roles.begin(); it != bp.peers_roles.end(); it++) {
            directory_echo_writer_t<cow_ptr_t<reactor_business_card_t> >::ack_waiter_t ack_waiter(&directory_echo_writer, it->first, version_to_wait_on);
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

