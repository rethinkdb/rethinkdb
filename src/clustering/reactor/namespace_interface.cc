// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/reactor/namespace_interface.hpp"

#include <functional>

#include "clustering/immediate_consistency/query/master_access.hpp"
#include "concurrency/fifo_enforcer.hpp"
#include "concurrency/watchable.hpp"
#include "rdb_protocol/env.hpp"

cluster_namespace_interface_t::cluster_namespace_interface_t(
        mailbox_manager_t *mm,
        const std::map<namespace_id_t, std::map<key_range_t, server_id_t> >
            *region_to_primary_maps_,
        watchable_map_t<peer_id_t, namespace_directory_metadata_t> *dv,
        const namespace_id_t &namespace_id_,
        rdb_context_t *_ctx)
    : mailbox_manager(mm),
      region_to_primary_maps(region_to_primary_maps_),
      directory_view(dv),
      namespace_id(namespace_id_),
      ctx(_ctx),
      start_count(0),
      starting_up(true),
      subs(directory_view,
        [this](const peer_id_t &peer, const namespace_directory_metadata_t *bcard) {
            update_registrant(peer, bcard);
        }, true) {
    rassert(ctx != NULL);
    starting_up = false;
    if (start_count == 0) {
        start_cond.pulse();
    }
}

bool cluster_namespace_interface_t::check_readiness(table_readiness_t readiness,
                                                    signal_t *interruptor) {
    rassert(readiness != table_readiness_t::finished,
            "Cannot check for the 'finished' state with namespace_interface_t.");
    try {
        switch (readiness) {
        case table_readiness_t::outdated_reads:
            {
                read_response_t res;
                read_t r(dummy_read_t(), profile_bool_t::DONT_PROFILE);
                read_outdated(r, &res, interruptor);
            }
            break;
        case table_readiness_t::reads:
            {
                read_response_t res;
                read_t r(dummy_read_t(), profile_bool_t::DONT_PROFILE);
                read(r, &res, order_token_t::ignore, interruptor);
            }
            break;
        case table_readiness_t::finished: // Fallthrough in release mode, better than a crash
        case table_readiness_t::writes:
            {
                write_response_t res;
                write_t w(dummy_write_t(), profile_bool_t::DONT_PROFILE,
                          ql::configured_limits_t::unlimited);
                write(w, &res, order_token_t::ignore, interruptor);
            }
            break;
        case table_readiness_t::unavailable:
            // Do nothing - all tables are always >= unavailable
            break;
        default:
            unreachable();
        }
    } catch (const cannot_perform_query_exc_t &) {
        return false;
    }
    return true;
}

void cluster_namespace_interface_t::read(
    const read_t &r,
    read_response_t *response,
    order_token_t order_token,
    signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t) {
    order_token.assert_read_mode();
    dispatch_immediate_op<read_t, fifo_enforcer_sink_t::exit_read_t, read_response_t>(
            &master_access_t::new_read_token,
            &master_access_t::read,
            r, response, order_token, interruptor);
}

void cluster_namespace_interface_t::read_outdated(const read_t &r, read_response_t *response, signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t) {
    /* This seems kind of silly. We do it this way because
       `dispatch_outdated_read` needs to be able to see `outdated_read_info_t`,
       which is defined in the `private` section. */
    dispatch_outdated_read(r, response, interruptor);
}

void cluster_namespace_interface_t::write(const write_t &w,
                                          write_response_t *response,
                                          order_token_t order_token,
                                          signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t) {
    order_token.assert_write_mode();
    dispatch_immediate_op<write_t, fifo_enforcer_sink_t::exit_write_t, write_response_t>(&master_access_t::new_write_token, &master_access_t::write,
                                                                                                                                   w, response, order_token, interruptor);
}

std::set<region_t>
cluster_namespace_interface_t::get_sharding_scheme()
    THROWS_ONLY(cannot_perform_query_exc_t) {
    std::vector<region_t> s;
    for (region_map_t<std::set<relationship_t *> >::iterator it = relationships.begin();
         it != relationships.end();
         it++) {
        for (std::set<relationship_t *>::iterator jt = it->second.begin();
             jt != it->second.end();
             jt++) {
            s.push_back((*jt)->region);
        }
    }
    region_t whole;
    region_join_result_t res = region_join(s, &whole);
    if (res != REGION_JOIN_OK || whole != region_t::universe()) {
        throw cannot_perform_query_exc_t(
            "cannot compute sharding scheme because primary replicas are "
            "unavailable or duplicated",
            query_state_t::FAILED);
    }
    return std::set<region_t>(s.begin(), s.end());
}

template<class op_type, class fifo_enforcer_token_type, class op_response_type>
void cluster_namespace_interface_t::dispatch_immediate_op(
    /* `how_to_make_token` and `how_to_run_query` have type pointer-to-
       member-function. */
    void (master_access_t::*how_to_make_token)(
        fifo_enforcer_token_type *),
    void (master_access_t::*how_to_run_query)(
        const op_type &,
        op_response_type *,
        order_token_t,
        fifo_enforcer_token_type *,
        signal_t *) THROWS_ONLY(interrupted_exc_t, resource_lost_exc_t,
                                cannot_perform_query_exc_t),
    const op_type &op,
    op_response_type *response,
    order_token_t order_token,
    signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t) {

    if (interruptor->is_pulsed()) throw interrupted_exc_t();

    std::vector<scoped_ptr_t<immediate_op_info_t<op_type, fifo_enforcer_token_type> > >
        masters_to_contact;
    scoped_ptr_t<immediate_op_info_t<op_type, fifo_enforcer_token_type> >
        new_op_info(new immediate_op_info_t<op_type, fifo_enforcer_token_type>());
    for (auto it = relationships.begin(); it != relationships.end(); ++it) {
        if (op.shard(it->first, &new_op_info->sharded_op)) {
            relationship_t *chosen_relationship = NULL;
            const std::set<relationship_t *> *relationship_map = &it->second;
            for (auto jt = relationship_map->begin();
                 jt != relationship_map->end();
                 ++jt) {
                if ((*jt)->master_access) {
                    if (chosen_relationship) {
                        throw cannot_perform_query_exc_t(
                            "Too many primary replicas available",
                            query_state_t::FAILED);
                    }
                    chosen_relationship = *jt;
                }
            }
            if (!chosen_relationship) {
                auto region_to_primary = region_to_primary_maps->find(namespace_id);
                if (region_to_primary != region_to_primary_maps->end()) {
                    auto primary = region_to_primary->second.find(it->first.inner);
                    if (primary != region_to_primary->second.end()) {
                        std::string mid = uuid_to_str(primary->second);
                        // Throw a more specific error if possible
                        throw cannot_perform_query_exc_t(
                            strprintf("Primary replica for shard %s not available "
                                      "(server %s is not ready)",
                                      key_range_to_string(it->first.inner).c_str(),
                                      mid.c_str()),
                            query_state_t::FAILED);
                    }
                }
                throw cannot_perform_query_exc_t(
                    strprintf("Primary replica for shard %s not available",
                              key_range_to_string(it->first.inner).c_str()),
                    query_state_t::FAILED);
            }
            new_op_info->master_access = chosen_relationship->master_access;
            (new_op_info->master_access->*how_to_make_token)(
                &new_op_info->enforcement_token);
            new_op_info->keepalive = auto_drainer_t::lock_t(
                &chosen_relationship->drainer);
            masters_to_contact.push_back(std::move(new_op_info));
            new_op_info.init(
                new immediate_op_info_t<op_type, fifo_enforcer_token_type>());
        }
    }

    std::vector<op_response_type> results(masters_to_contact.size());
    std::vector<boost::optional<cannot_perform_query_exc_t> >
        failures(masters_to_contact.size());
    pmap(masters_to_contact.size(), std::bind(
             &cluster_namespace_interface_t::template perform_immediate_op<
                 op_type, fifo_enforcer_token_type, op_response_type>,
             this,
             how_to_run_query,
             &masters_to_contact,
             &results,
             &failures,
             order_token,
             ph::_1,
             interruptor));

    if (interruptor->is_pulsed()) throw interrupted_exc_t();

    bool seen_valid = false;
    boost::optional<cannot_perform_query_exc_t> last_failed_exc;
    for (size_t i = 0; i < masters_to_contact.size(); ++i) {
        if (!failures[i]) {
            seen_valid = true;
            if (last_failed_exc) break;
        } else {
            switch (failures[i]->get_query_state()) {
            case query_state_t::FAILED:
                last_failed_exc = failures[i];
                break;
            case query_state_t::INDETERMINATE:
                // We can throw right away here because we already know whether
                // or not we're indeterminate.
                throw *failures[i];
                break;
            default: unreachable();
            }
            if (seen_valid) break;
        }
    }
    if (last_failed_exc) {
        // If we failed on one master and succeeded on others the operation is
        // indeterminate.
        if (seen_valid) {
            throw cannot_perform_query_exc_t(last_failed_exc->what(),
                                             query_state_t::INDETERMINATE);
        } else {
            throw *last_failed_exc;
        }
    }

    op.unshard(results.data(), results.size(), response, ctx, interruptor);
}

template<class op_type, class fifo_enforcer_token_type, class op_response_type>
void cluster_namespace_interface_t::perform_immediate_op(
    void (master_access_t::*how_to_run_query)(
        const op_type &,
        op_response_type *,
        order_token_t,
        fifo_enforcer_token_type *,
        signal_t *)
    /* THROWS_ONLY(interrupted_exc_t, resource_lost_exc_t,
       cannot_perform_query_exc_t) */,
    std::vector<scoped_ptr_t<immediate_op_info_t<op_type, fifo_enforcer_token_type> > > *
        masters_to_contact,
    std::vector<op_response_type> *results,
    std::vector<boost::optional<cannot_perform_query_exc_t> > *failures,
    order_token_t order_token,
    size_t i,
    signal_t *interruptor) THROWS_NOTHING {
    guarantee(i < failures->size());
    immediate_op_info_t<op_type, fifo_enforcer_token_type> *master_to_contact
        = (*masters_to_contact)[i].get();

    try {
        (master_to_contact->master_access->*how_to_run_query)(
            master_to_contact->sharded_op,
            &results->at(i),
            order_token,
            &master_to_contact->enforcement_token,
            interruptor);
    } catch (const resource_lost_exc_t&) {
        (*failures)[i] = cannot_perform_query_exc_t(
            "lost contact with primary replica",
            query_state_t::INDETERMINATE);
    } catch (const cannot_perform_query_exc_t& e) {
        (*failures)[i] = e;
    } catch (const interrupted_exc_t&) {
        guarantee(interruptor->is_pulsed());
        /* Ignore `interrupted_exc_t` and just return immediately.
           `dispatch_immediate_op()` will notice the interruptor has been
           pulsed and won't try to access our result. */
    }
}

void
cluster_namespace_interface_t::dispatch_outdated_read(
    const read_t &op,
    read_response_t *response,
    signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t) {

    if (interruptor->is_pulsed()) throw interrupted_exc_t();

    std::vector<scoped_ptr_t<outdated_read_info_t> > direct_readers_to_contact;

    scoped_ptr_t<outdated_read_info_t> new_op_info(new outdated_read_info_t());
    for (auto it = relationships.begin(); it != relationships.end(); ++it) {
        if (op.shard(it->first, &new_op_info->sharded_op)) {
            std::vector<relationship_t *> potential_relationships;
            relationship_t *chosen_relationship = NULL;

            const std::set<relationship_t *> *relationship_map = &it->second;
            for (auto jt = relationship_map->begin();
                 jt != relationship_map->end();
                 ++jt) {
                if ((*jt)->direct_reader_access) {
                    if ((*jt)->is_local) {
                        chosen_relationship = *jt;
                        break;
                    } else {
                        potential_relationships.push_back(*jt);
                    }
                }
            }
            if (!chosen_relationship && !potential_relationships.empty()) {
                chosen_relationship
                    = potential_relationships[
                        distributor_rng.randint(potential_relationships.size())];
            }
            if (!chosen_relationship) {
                /* Don't bother looking for masters; if there are no direct
                   readers, there won't be any masters either. */
                throw cannot_perform_query_exc_t(
                    "No direct reader available",
                    query_state_t::FAILED);
            }
            new_op_info->direct_reader_access
                = chosen_relationship->direct_reader_access;
            new_op_info->keepalive = auto_drainer_t::lock_t(
                &chosen_relationship->drainer);
            direct_readers_to_contact.push_back(std::move(new_op_info));
            new_op_info.init(new outdated_read_info_t());
        }
    }

    std::vector<read_response_t> results(direct_readers_to_contact.size());
    std::vector<std::string> failures(direct_readers_to_contact.size());
    pmap(direct_readers_to_contact.size(),
         std::bind(
             &cluster_namespace_interface_t::perform_outdated_read, this,
             &direct_readers_to_contact, &results, &failures, ph::_1, interruptor));

    if (interruptor->is_pulsed()) throw interrupted_exc_t();

    for (size_t i = 0; i < direct_readers_to_contact.size(); ++i) {
        if (!failures[i].empty()) {
            throw cannot_perform_query_exc_t(failures[i], query_state_t::FAILED);
        }
    }

    op.unshard(results.data(), results.size(), response, ctx, interruptor);
}

void cluster_namespace_interface_t::perform_outdated_read(
        std::vector<scoped_ptr_t<outdated_read_info_t> > *direct_readers_to_contact,
        std::vector<read_response_t> *results,
        std::vector<std::string> *failures,
        size_t i,
        signal_t *interruptor) THROWS_NOTHING {
    outdated_read_info_t *direct_reader_to_contact = (*direct_readers_to_contact)[i].get();

    try {
        cond_t done;
        mailbox_t<void(read_response_t)> cont(mailbox_manager,
            [&](signal_t *, const read_response_t &res) {
                results->at(i) = res;
                done.pulse();
            });

        send(mailbox_manager, direct_reader_to_contact->direct_reader_access->access().read_mailbox, direct_reader_to_contact->sharded_op, cont.get_address());
        wait_any_t waiter(direct_reader_to_contact->direct_reader_access->get_failed_signal(), &done);
        wait_interruptible(&waiter, interruptor);
        direct_reader_to_contact->direct_reader_access->access(); /* throws if `get_failed_signal()->is_pulsed()` */
    } catch (const resource_lost_exc_t &) {
        failures->at(i).assign("lost contact with direct reader");
    } catch (const interrupted_exc_t &) {
        guarantee(interruptor->is_pulsed());
        /* Ignore `interrupted_exc_t` and return immediately.
           `read_outdated()` will notice that the interruptor has been pulsed
           and won't try to access our result. */
    }
}

void cluster_namespace_interface_t::update_registrant(
        const peer_id_t &peer, const namespace_directory_metadata_t *bcard) {
    if (bcard == nullptr) {
        return;
    }
    for (auto amit = bcard->internal->activities.begin();
            amit != bcard->internal->activities.end();
            ++amit) {
        bool has_anything_useful;
        bool is_primary;
        if (const reactor_business_card_details::primary_t *primary =
                boost::get<reactor_business_card_details::primary_t>(
                    &amit->second.activity)) {
            if (primary->master) {
                has_anything_useful = true;
                is_primary = true;
            } else {
                has_anything_useful = false;
                is_primary = false;  // Appease -Wconditional-uninitialized
            }
        } else if (boost::get<reactor_business_card_details::secondary_up_to_date_t>(
                &amit->second.activity)) {
            has_anything_useful = true;
            is_primary = false;
        } else if (boost::get<reactor_business_card_details::secondary_without_primary_t>(
                &amit->second.activity)) {
            has_anything_useful = true;
            is_primary = false;
        } else {
            has_anything_useful = false;
            is_primary = false;  // Appease -Wconditional-uninitialized
        }
        if (has_anything_useful) {
            reactor_activity_id_t id = amit->first;
            if (handled_activity_ids.find(id) == handled_activity_ids.end()) {
                // We have an unhandled activity id.  Say it's handled NOW!  And handle it.
                handled_activity_ids.insert(id);  // We said it.

                if (starting_up) {
                    start_count++;
                }

                /* Now handle it. */
                coro_t::spawn_sometime(std::bind(
                    &cluster_namespace_interface_t::relationship_coroutine, this,
                    peer, id, starting_up, is_primary, amit->second.region,
                    auto_drainer_t::lock_t(&relationship_coroutine_auto_drainer)));
            }
        }
    }
}

boost::optional<boost::optional<master_business_card_t> >
cluster_namespace_interface_t::extract_master_business_card(
        const boost::optional<namespace_directory_metadata_t> &bcard,
        const reactor_activity_id_t &activity_id) {
    boost::optional<boost::optional<master_business_card_t> > ret;
    if (static_cast<bool>(bcard)) {
        ret = boost::optional<master_business_card_t>();
        reactor_business_card_t::activity_map_t::const_iterator jt =
            bcard->internal->activities.find(activity_id);
        if (jt != bcard->internal->activities.end()) {
            if (const reactor_business_card_details::primary_t *primary_record =
                boost::get<reactor_business_card_details::primary_t>(&jt->second.activity)) {
                if (primary_record->master) {
                    ret.get() = primary_record->master.get();
                }
            }
        }
    }
    return ret;
}

boost::optional<boost::optional<direct_reader_business_card_t> >
cluster_namespace_interface_t::extract_direct_reader_business_card_from_primary(
        const boost::optional<namespace_directory_metadata_t> &bcard,
        const reactor_activity_id_t &activity_id) {
    boost::optional<boost::optional<direct_reader_business_card_t> > ret;
    if (static_cast<bool>(bcard)) {
        ret = boost::optional<direct_reader_business_card_t>();
        reactor_business_card_t::activity_map_t::const_iterator jt =
            bcard->internal->activities.find(activity_id);
        if (jt != bcard->internal->activities.end()) {
            if (const reactor_business_card_details::primary_t *primary_record =
                boost::get<reactor_business_card_details::primary_t>(&jt->second.activity)) {
                if (primary_record->direct_reader) {
                    ret.get() = primary_record->direct_reader.get();
                }
            }
        }
    }
    return ret;
}

boost::optional<boost::optional<direct_reader_business_card_t> >
cluster_namespace_interface_t::extract_direct_reader_business_card_from_secondary(
        const boost::optional<namespace_directory_metadata_t> &bcard,
        const reactor_activity_id_t &activity_id) {
    boost::optional<boost::optional<direct_reader_business_card_t> > ret;
    if (static_cast<bool>(bcard)) {
        ret = boost::optional<direct_reader_business_card_t>();
        reactor_business_card_t::activity_map_t::const_iterator jt =
            bcard->internal->activities.find(activity_id);
        if (jt != bcard->internal->activities.end()) {
            if (const reactor_business_card_details::secondary_up_to_date_t *secondary_up_to_date_record =
                boost::get<reactor_business_card_details::secondary_up_to_date_t>(&jt->second.activity)) {
                ret.get() = secondary_up_to_date_record->direct_reader;
            }
            if (const reactor_business_card_details::secondary_without_primary_t *secondary_without_primary =
                boost::get<reactor_business_card_details::secondary_without_primary_t>(&jt->second.activity)) {
                ret.get() = secondary_without_primary->direct_reader;
            }
        }
    }
    return ret;
}

template <class value_t>
class region_map_set_membership_t {
public:
    region_map_set_membership_t(region_map_t<std::set<value_t> > *m, const region_t &r, const value_t &v) :
        map(m), region(r), value(v) {
        region_map_t<std::set<value_t> > submap = map->mask(region);
        for (typename region_map_t<std::set<value_t> >::iterator it = submap.begin(); it != submap.end(); it++) {
            it->second.insert(value);
        }
        map->update(submap);
    }
    ~region_map_set_membership_t() {
        region_map_t<std::set<value_t> > submap = map->mask(region);
        for (typename region_map_t<std::set<value_t> >::iterator it = submap.begin(); it != submap.end(); it++) {
            it->second.erase(value);
        }
        map->update(submap);
    }
private:
    region_map_t<std::set<value_t> > *map;
    region_t region;
    value_t value;
};

void cluster_namespace_interface_t::relationship_coroutine(peer_id_t peer_id, reactor_activity_id_t activity_id,
                                                           bool is_start, bool is_primary, const region_t &region,
                                                           auto_drainer_t::lock_t lock) THROWS_NOTHING {
    try {
        scoped_ptr_t<master_access_t> master_access;
        scoped_ptr_t<resource_access_t<direct_reader_business_card_t> > direct_reader_access;
        if (is_primary) {
            master_access.init(new master_access_t(
                mailbox_manager,
                get_watchable_for_key(directory_view, peer_id)->subview(std::bind(
                    &cluster_namespace_interface_t::extract_master_business_card,
                    ph::_1, activity_id)),
                    lock.get_drain_signal()));
            direct_reader_access.init(
                new resource_access_t<direct_reader_business_card_t>(
                    get_watchable_for_key(directory_view, peer_id)->subview(std::bind(
                        &cluster_namespace_interface_t::
                            extract_direct_reader_business_card_from_primary,
                        ph::_1, activity_id))
                    ));
        } else {
            direct_reader_access.init(
                new resource_access_t<direct_reader_business_card_t>(
                    get_watchable_for_key(directory_view, peer_id)->subview(std::bind(
                        &cluster_namespace_interface_t::
                            extract_direct_reader_business_card_from_secondary,
                        ph::_1, activity_id))
                    ));
        }

        relationship_t relationship_record;
        relationship_record.is_local =
            (peer_id == mailbox_manager->get_connectivity_cluster()->get_me());
        relationship_record.region = region;
        relationship_record.master_access = master_access.has() ? master_access.get() : NULL;
        relationship_record.direct_reader_access = direct_reader_access.has() ? direct_reader_access.get() : NULL;

        region_map_set_membership_t<relationship_t *> relationship_map_insertion(&relationships,
                                                                                 region,
                                                                                 &relationship_record);

        if (is_start) {
            guarantee(start_count > 0);
            start_count--;
            if (start_count == 0) {
                start_cond.pulse();
            }
            is_start = false;
        }

        wait_any_t waiter(lock.get_drain_signal());
        if (master_access.has()) {
            waiter.add(master_access->get_failed_signal());
        }
        if (direct_reader_access.has()) {
            waiter.add(direct_reader_access->get_failed_signal());
        }
        waiter.wait_lazily_unordered();

    } catch (const resource_lost_exc_t &e) {
        /* ignore */
    } catch (const interrupted_exc_t &e) {
        /* ignore */
    }

    if (is_start) {
        guarantee(start_count > 0);
        start_count--;
        if (start_count == 0) {
            start_cond.pulse();
        }
    }

    handled_activity_ids.erase(activity_id);

    // Maybe we got reconnected really quickly, and didn't handle
    // the reconnection, because `handled_activity_ids` already noted
    // ourselves as handled.
    if (!lock.get_drain_signal()->is_pulsed()) {
        directory_view->read_key(peer_id,
            [&](const namespace_directory_metadata_t *bcard) {
                update_registrant(peer_id, bcard);
            });
    }
}


