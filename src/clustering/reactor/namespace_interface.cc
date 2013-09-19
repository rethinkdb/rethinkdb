// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "clustering/reactor/namespace_interface.hpp"
#include "clustering/immediate_consistency/query/master_access.hpp"
#include "concurrency/fifo_enforcer.hpp"
#include "concurrency/watchable.hpp"

template <class protocol_t>
cluster_namespace_interface_t<protocol_t>::cluster_namespace_interface_t(
        mailbox_manager_t *mm,
        clone_ptr_t<watchable_t<std::map<peer_id_t, cow_ptr_t<reactor_business_card_t<protocol_t> > > > > dv,
        typename protocol_t::context_t *_ctx)
    : mailbox_manager(mm),
      directory_view(dv),
      ctx(_ctx),
      start_count(0),
      watcher_subscription(new watchable_subscription_t<std::map<peer_id_t, cow_ptr_t<reactor_business_card_t<protocol_t> > > >(boost::bind(&cluster_namespace_interface_t::update_registrants, this, false))) {
    {
        typename watchable_t<std::map<peer_id_t, cow_ptr_t<reactor_business_card_t<protocol_t> > > >::freeze_t freeze(directory_view);
        update_registrants(true);
        // TODO: See if this watcher_subscription use is a significant use.
        watcher_subscription->reset(directory_view, &freeze);
    }
    if (start_count == 0) {
        start_cond.pulse();
    }
}


template <class protocol_t>
void cluster_namespace_interface_t<protocol_t>::read(
    const typename protocol_t::read_t &r,
    typename protocol_t::read_response_t *response,
    order_token_t order_token,
    signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t) {

    order_token.assert_read_mode();
    dispatch_immediate_op<
        typename protocol_t::read_t,
        fifo_enforcer_sink_t::exit_read_t,
        typename protocol_t::read_response_t>(
            &master_access_t<protocol_t>::new_read_token,
            &master_access_t<protocol_t>::read,
            r, response, order_token, interruptor);
}

template <class protocol_t>
void cluster_namespace_interface_t<protocol_t>::read_outdated(const typename protocol_t::read_t &r, typename protocol_t::read_response_t *response, signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t) {
    /* This seems kind of silly. We do it this way because
       `dispatch_outdated_read` needs to be able to see `outdated_read_info_t`,
       which is defined in the `private` section. */
    dispatch_outdated_read(r, response, interruptor);
}

template <class protocol_t>
void cluster_namespace_interface_t<protocol_t>::write(const typename protocol_t::write_t &w,
                                                      typename protocol_t::write_response_t *response,
                                                      order_token_t order_token,
                                                      signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t) {
    order_token.assert_write_mode();
    dispatch_immediate_op<typename protocol_t::write_t, fifo_enforcer_sink_t::exit_write_t, typename protocol_t::write_response_t>(&master_access_t<protocol_t>::new_write_token, &master_access_t<protocol_t>::write,
                                                                                                                                   w, response, order_token, interruptor);
}

template <class protocol_t>
std::set<typename protocol_t::region_t>
cluster_namespace_interface_t<protocol_t>::get_sharding_scheme() THROWS_ONLY(cannot_perform_query_exc_t) {
    std::vector<typename protocol_t::region_t> s;
    for (typename region_map_t<protocol_t, std::set<relationship_t *> >::iterator it = relationships.begin(); it != relationships.end(); it++) {
        for (typename std::set<relationship_t *>::iterator jt = it->second.begin(); jt != it->second.end(); jt++) {
            s.push_back((*jt)->region);
        }
    }
    typename protocol_t::region_t whole;
    region_join_result_t res = region_join(s, &whole);
    if (res != REGION_JOIN_OK || whole != protocol_t::region_t::universe()) {
        throw cannot_perform_query_exc_t("cannot compute sharding scheme because masters are missing or duplicate");
    }
    return std::set<typename protocol_t::region_t>(s.begin(), s.end());
}

template <class protocol_t>
template<class op_type, class fifo_enforcer_token_type, class op_response_type>
void cluster_namespace_interface_t<protocol_t>::dispatch_immediate_op(
    /* `how_to_make_token` and `how_to_run_query` have type pointer-to-
       member-function. */
    void (master_access_t<protocol_t>::*how_to_make_token)(
        fifo_enforcer_token_type *),  // NOLINT
    void (master_access_t<protocol_t>::*how_to_run_query)(
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

    boost::ptr_vector<immediate_op_info_t<op_type, fifo_enforcer_token_type> >
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
                        throw cannot_perform_query_exc_t("Too many masters available");
                    }
                    chosen_relationship = *jt;
                }
            }
            if (!chosen_relationship) {
                throw cannot_perform_query_exc_t("No master available");
            }
            new_op_info->master_access = chosen_relationship->master_access;
            (new_op_info->master_access->*how_to_make_token)(
                &new_op_info->enforcement_token);
            new_op_info->keepalive = auto_drainer_t::lock_t(
                &chosen_relationship->drainer);
            masters_to_contact.push_back(new_op_info.release());
            new_op_info.init(
                new immediate_op_info_t<op_type, fifo_enforcer_token_type>());
        }
    }

    std::vector<op_response_type> results(masters_to_contact.size());
    std::vector<std::string> failures(masters_to_contact.size());
    pmap(masters_to_contact.size(), boost::bind(
             &cluster_namespace_interface_t::template perform_immediate_op<
                 op_type, fifo_enforcer_token_type, op_response_type>,
             this,
             how_to_run_query,
             &masters_to_contact,
             &results,
             &failures,
             order_token,
             _1,
             interruptor));

    if (interruptor->is_pulsed()) throw interrupted_exc_t();

    for (size_t i = 0; i < masters_to_contact.size(); ++i) {
        if (!failures[i].empty()) {
            throw cannot_perform_query_exc_t(failures[i]);
        }
    }

    op.unshard(results.data(), results.size(), response, ctx, interruptor);
}

template <class protocol_t>
template<class op_type, class fifo_enforcer_token_type, class op_response_type>
void cluster_namespace_interface_t<protocol_t>::perform_immediate_op(
    void (master_access_t<protocol_t>::*how_to_run_query)(
        const op_type &,
        op_response_type *,
        order_token_t,
        fifo_enforcer_token_type *,
        signal_t *) THROWS_ONLY(interrupted_exc_t,
                                resource_lost_exc_t,
                                cannot_perform_query_exc_t),
    boost::ptr_vector<immediate_op_info_t<op_type, fifo_enforcer_token_type> > *
        masters_to_contact,
    std::vector<op_response_type> *results,
    std::vector<std::string> *failures,
    order_token_t order_token,
    int i,
    signal_t *interruptor)
    THROWS_NOTHING
{
    immediate_op_info_t<op_type, fifo_enforcer_token_type> *master_to_contact
        = &(*masters_to_contact)[i];

    try {
        (master_to_contact->master_access->*how_to_run_query)(
            master_to_contact->sharded_op,
            &results->at(i),
            order_token,
            &master_to_contact->enforcement_token,
            interruptor);
    } catch (const resource_lost_exc_t&) {
        failures->at(i).assign("lost contact with master");
    } catch (const cannot_perform_query_exc_t& e) {
        failures->at(i).assign("master error: " + std::string(e.what()));
    } catch (const interrupted_exc_t&) {
        guarantee(interruptor->is_pulsed());
        /* Ignore `interrupted_exc_t` and just return immediately.
           `dispatch_immediate_op()` will notice the interruptor has been
           pulsed and won't try to access our result. */
    }
}

template <class protocol_t>
void
cluster_namespace_interface_t<protocol_t>::dispatch_outdated_read(
    const typename protocol_t::read_t &op,
    typename protocol_t::read_response_t *response,
    signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t) {

    if (interruptor->is_pulsed()) throw interrupted_exc_t();

    boost::ptr_vector<outdated_read_info_t> direct_readers_to_contact;

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
                throw cannot_perform_query_exc_t("No direct reader available");
            }
            new_op_info->direct_reader_access
                = chosen_relationship->direct_reader_access;
            new_op_info->keepalive = auto_drainer_t::lock_t(
                &chosen_relationship->drainer);
            direct_readers_to_contact.push_back(new_op_info.release());
            new_op_info.init(new outdated_read_info_t());
        }
    }

    std::vector<typename protocol_t::read_response_t> results(direct_readers_to_contact.size());
    std::vector<std::string> failures(direct_readers_to_contact.size());
    pmap(direct_readers_to_contact.size(), boost::bind(&cluster_namespace_interface_t::perform_outdated_read, this,
                                                       &direct_readers_to_contact, &results, &failures, _1, interruptor));

    if (interruptor->is_pulsed()) throw interrupted_exc_t();

    for (size_t i = 0; i < direct_readers_to_contact.size(); ++i) {
        if (!failures[i].empty()) {
            throw cannot_perform_query_exc_t(failures[i]);
        }
    }

    op.unshard(results.data(), results.size(), response, ctx, interruptor);
}

template <class protocol_t>
void outdated_read_store_result(typename protocol_t::read_response_t *result_out, const typename protocol_t::read_response_t &result_in, cond_t *done) {
    *result_out = result_in;
    done->pulse();
}

template <class protocol_t>
void cluster_namespace_interface_t<protocol_t>::perform_outdated_read(
    boost::ptr_vector<outdated_read_info_t> *direct_readers_to_contact,
    std::vector<typename protocol_t::read_response_t> *results,
    std::vector<std::string> *failures,
    int i,
    signal_t *interruptor)
    THROWS_NOTHING
{
    outdated_read_info_t *direct_reader_to_contact = &(*direct_readers_to_contact)[i];

    try {
        cond_t done;
        mailbox_t<void(typename protocol_t::read_response_t)> cont(mailbox_manager,
                                                                   boost::bind(&outdated_read_store_result<protocol_t>, &results->at(i), _1, &done));

        send(mailbox_manager, direct_reader_to_contact->direct_reader_access->access().read_mailbox, direct_reader_to_contact->sharded_op, cont.get_address());
        wait_any_t waiter(direct_reader_to_contact->direct_reader_access->get_failed_signal(), &done);
        wait_interruptible(&waiter, interruptor);
        direct_reader_to_contact->direct_reader_access->access();   /* throws if `get_failed_signal()->is_pulsed()` */
    } catch (const resource_lost_exc_t &) {
        failures->at(i).assign("lost contact with direct reader");
    } catch (const interrupted_exc_t &) {
        guarantee(interruptor->is_pulsed());
        /* Ignore `interrupted_exc_t` and return immediately.
           `read_outdated()` will notice that the interruptor has been pulsed
           and won't try to access our result. */
    }
}

template <class protocol_t>
void cluster_namespace_interface_t<protocol_t>::update_registrants(bool is_start) {
    ASSERT_NO_CORO_WAITING;
    std::map<peer_id_t, cow_ptr_t<reactor_business_card_t<protocol_t> > > existings = directory_view->get();

    for (typename std::map<peer_id_t, cow_ptr_t<reactor_business_card_t<protocol_t> > >::const_iterator it = existings.begin(); it != existings.end(); ++it) {
        for (typename reactor_business_card_t<protocol_t>::activity_map_t::const_iterator amit = it->second->activities.begin(); amit != it->second->activities.end(); ++amit) {
            bool has_anything_useful;
            bool is_primary;
            if (const reactor_business_card_details::primary_t<protocol_t> *primary = boost::get<reactor_business_card_details::primary_t<protocol_t> >(&amit->second.activity)) {
                if (primary->master) {
                    has_anything_useful = true;
                    is_primary = true;
                } else {
                    has_anything_useful = false;
                    is_primary = false;  // Appease -Wconditional-uninitialized
                }
            } else if (boost::get<reactor_business_card_details::secondary_up_to_date_t<protocol_t> >(&amit->second.activity)) {
                has_anything_useful = true;
                is_primary = false;
            } else if (boost::get<reactor_business_card_details::secondary_without_primary_t<protocol_t> >(&amit->second.activity)) {
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

                    if (is_start) {
                        start_count++;
                    }

                    /* Now handle it. */
                    coro_t::spawn_sometime(boost::bind(&cluster_namespace_interface_t::relationship_coroutine, this,
                                                       it->first, id, is_start, is_primary, amit->second.region,
                                                       auto_drainer_t::lock_t(&relationship_coroutine_auto_drainer)));
                }
            }
        }
    }
}

template <class protocol_t>
boost::optional<boost::optional<master_business_card_t<protocol_t> > >
cluster_namespace_interface_t<protocol_t>:: extract_master_business_card(const std::map<peer_id_t, cow_ptr_t<reactor_business_card_t<protocol_t> > > &map, const peer_id_t &peer, const reactor_activity_id_t &activity_id) {
    boost::optional<boost::optional<master_business_card_t<protocol_t> > > ret;
    typename std::map<peer_id_t, cow_ptr_t<reactor_business_card_t<protocol_t> > >::const_iterator it = map.find(peer);
    if (it != map.end()) {
        ret = boost::optional<master_business_card_t<protocol_t> >();
        typename reactor_business_card_t<protocol_t>::activity_map_t::const_iterator jt = it->second->activities.find(activity_id);
        if (jt != it->second->activities.end()) {
            if (const reactor_business_card_details::primary_t<protocol_t> *primary_record =
                boost::get<reactor_business_card_details::primary_t<protocol_t> >(&jt->second.activity)) {
                if (primary_record->master) {
                    ret.get() = primary_record->master.get();
                }
            }
        }
    }
    return ret;
}

template <class protocol_t>
boost::optional<boost::optional<direct_reader_business_card_t<protocol_t> > >
cluster_namespace_interface_t<protocol_t>::extract_direct_reader_business_card_from_primary(const std::map<peer_id_t, cow_ptr_t<reactor_business_card_t<protocol_t> > > &map, const peer_id_t &peer, const reactor_activity_id_t &activity_id) {
    boost::optional<boost::optional<direct_reader_business_card_t<protocol_t> > > ret;
    typename std::map<peer_id_t, cow_ptr_t<reactor_business_card_t<protocol_t> > >::const_iterator it = map.find(peer);
    if (it != map.end()) {
        ret = boost::optional<direct_reader_business_card_t<protocol_t> >();
        typename reactor_business_card_t<protocol_t>::activity_map_t::const_iterator jt = it->second->activities.find(activity_id);
        if (jt != it->second->activities.end()) {
            if (const reactor_business_card_details::primary_t<protocol_t> *primary_record =
                boost::get<reactor_business_card_details::primary_t<protocol_t> >(&jt->second.activity)) {
                if (primary_record->direct_reader) {
                    ret.get() = primary_record->direct_reader.get();
                }
            }
        }
    }
    return ret;
}

template <class protocol_t>
boost::optional<boost::optional<direct_reader_business_card_t<protocol_t> > >
cluster_namespace_interface_t<protocol_t>::extract_direct_reader_business_card_from_secondary(const std::map<peer_id_t, cow_ptr_t<reactor_business_card_t<protocol_t> > > &map, const peer_id_t &peer, const reactor_activity_id_t &activity_id) {
    boost::optional<boost::optional<direct_reader_business_card_t<protocol_t> > > ret;
    typename std::map<peer_id_t, cow_ptr_t<reactor_business_card_t<protocol_t> > >::const_iterator it = map.find(peer);
    if (it != map.end()) {
        ret = boost::optional<direct_reader_business_card_t<protocol_t> >();
        typename reactor_business_card_t<protocol_t>::activity_map_t::const_iterator jt = it->second->activities.find(activity_id);
        if (jt != it->second->activities.end()) {
            if (const reactor_business_card_details::secondary_up_to_date_t<protocol_t> *secondary_up_to_date_record =
                boost::get<reactor_business_card_details::secondary_up_to_date_t<protocol_t> >(&jt->second.activity)) {
                ret.get() = secondary_up_to_date_record->direct_reader;
            }
            if (const reactor_business_card_details::secondary_without_primary_t<protocol_t> *secondary_without_primary =
                boost::get<reactor_business_card_details::secondary_without_primary_t<protocol_t> >(&jt->second.activity)) {
                ret.get() = secondary_without_primary->direct_reader;
            }
        }
    }
    return ret;
}

template <class protocol_t>
void cluster_namespace_interface_t<protocol_t>::relationship_coroutine(peer_id_t peer_id, reactor_activity_id_t activity_id,
                                                                       bool is_start, bool is_primary, const typename protocol_t::region_t &region,
                                                                       auto_drainer_t::lock_t lock) THROWS_NOTHING {
    try {
        scoped_ptr_t<master_access_t<protocol_t> > master_access;
        scoped_ptr_t<resource_access_t<direct_reader_business_card_t<protocol_t> > > direct_reader_access;
        if (is_primary) {
            master_access.init(new master_access_t<protocol_t>(mailbox_manager,
                                                               directory_view->subview(boost::bind(&cluster_namespace_interface_t<protocol_t>::extract_master_business_card, _1, peer_id, activity_id)),
                                                               lock.get_drain_signal()));
            direct_reader_access.init(new resource_access_t<direct_reader_business_card_t<protocol_t> >(directory_view->subview(boost::bind(&cluster_namespace_interface_t<protocol_t>::extract_direct_reader_business_card_from_primary, _1, peer_id, activity_id))));
        } else {
            direct_reader_access.init(new resource_access_t<direct_reader_business_card_t<protocol_t> >(directory_view->subview(boost::bind(&cluster_namespace_interface_t<protocol_t>::extract_direct_reader_business_card_from_secondary, _1, peer_id, activity_id))));
        }

        relationship_t relationship_record;
        relationship_record.is_local = (peer_id == mailbox_manager->get_connectivity_service()->get_me());
        relationship_record.region = region;
        relationship_record.master_access = master_access.has() ? master_access.get() : NULL;
        relationship_record.direct_reader_access = direct_reader_access.has() ? direct_reader_access.get() : NULL;

        region_map_set_membership_t<protocol_t, relationship_t *> relationship_map_insertion(&relationships,
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
        update_registrants(false);
    }
}


#include "memcached/protocol.hpp"
template class cluster_namespace_interface_t<memcached_protocol_t>;

#include "mock/dummy_protocol.hpp"
template class cluster_namespace_interface_t<mock::dummy_protocol_t>;

#include "rdb_protocol/protocol.hpp"
template class cluster_namespace_interface_t<rdb_protocol_t>;


