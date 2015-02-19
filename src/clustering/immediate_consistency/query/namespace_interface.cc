// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/query/namespace_interface.hpp"

#include <functional>

#include "clustering/immediate_consistency/query/master_access.hpp"
#include "concurrency/fifo_enforcer.hpp"
#include "concurrency/watchable.hpp"
#include "rdb_protocol/env.hpp"

cluster_namespace_interface_t::cluster_namespace_interface_t(
        mailbox_manager_t *mm,
        watchable_map_t<std::pair<peer_id_t, uuid_u>, replica_business_card_t> *d,
        rdb_context_t *_ctx)
    : mailbox_manager(mm),
      directory(d),
      ctx(_ctx),
      start_count(0),
      starting_up(true),
      subs(directory,
        std::bind(&cluster_namespace_interface_t::update_registrant,
            this, ph::_1, ph::_2),
        true) {
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

void cluster_namespace_interface_t::read_outdated(
        const read_t &r,
        read_response_t *response,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t) {
    /* This seems kind of silly. We do it this way because
       `dispatch_outdated_read` needs to be able to see `outdated_read_info_t`,
       which is defined in the `private` section. */
    dispatch_outdated_read(r, response, interruptor);
}

void cluster_namespace_interface_t::write(
        const write_t &w,
        write_response_t *response,
        order_token_t order_token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t) {
    order_token.assert_write_mode();
    dispatch_immediate_op<write_t, fifo_enforcer_sink_t::exit_write_t, write_response_t>(
        &master_access_t::new_write_token,
        &master_access_t::write,
        w, response, order_token, interruptor);
}

std::set<region_t>
cluster_namespace_interface_t::get_sharding_scheme()
        THROWS_ONLY(cannot_perform_query_exc_t) {
    std::vector<region_t> s;
    for (const auto &pair : relationships) {
        for (relationship_t *rel : pair.second) {
            s.push_back(rel->region);
        }
    }
    region_t whole;
    region_join_result_t res = region_join(s, &whole);
    if (res != REGION_JOIN_OK || whole != region_t::universe()) {
        throw cannot_perform_query_exc_t("cannot compute sharding scheme "
                                         "because primary replicas are "
                                         "unavailable or duplicated");
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
        signal_t *) THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t),
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
                            "too many primary replicas available");
                    }
                    chosen_relationship = *jt;
                }
            }
            if (!chosen_relationship) {
                throw cannot_perform_query_exc_t(
                    strprintf("primary replica for shard %s not available",
                              key_range_to_string(it->first.inner).c_str()));
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
    std::vector<std::string> failures(masters_to_contact.size());
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

    for (size_t i = 0; i < masters_to_contact.size(); ++i) {
        if (!failures[i].empty()) {
            throw cannot_perform_query_exc_t(failures[i]);
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
    /* THROWS_ONLY(interrupted_exc_t, resource_lost_exc_t, cannot_perform_query_exc_t) */,
    std::vector<scoped_ptr_t<immediate_op_info_t<op_type, fifo_enforcer_token_type> > > *
        masters_to_contact,
    std::vector<op_response_type> *results,
    std::vector<std::string> *failures,
    order_token_t order_token,
    int i,
    signal_t *interruptor)
    THROWS_NOTHING
{
    immediate_op_info_t<op_type, fifo_enforcer_token_type> *master_to_contact
        = (*masters_to_contact)[i].get();

    try {
        wait_any_t waiter(master_to_contact->keepalive.get_drain_signal(), interruptor);
        (master_to_contact->master_access->*how_to_run_query)(
            master_to_contact->sharded_op,
            &results->at(i),
            order_token,
            &master_to_contact->enforcement_token,
            &waiter);
    } catch (const cannot_perform_query_exc_t& e) {
        failures->at(i).assign("primary replica error: " + std::string(e.what()));
    } catch (const interrupted_exc_t&) {
        if (interruptor->is_pulsed()) {
            /* Return immediately. `dispatch_immediate_op()` will notice that the
            interruptor has been pulsed. */
            return;
        } else {
            /* `keepalive.get_drain_signal()` was pulsed because the other server
            disconnected or stopped being a primary */
            failures->at(i).assign("lost contact with primary replica");
        }
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
                if ((*jt)->direct_reader_bcard != nullptr) {
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
                throw cannot_perform_query_exc_t("no replica is available");
            }
            new_op_info->direct_reader_bcard
                = chosen_relationship->direct_reader_bcard;
            new_op_info->keepalive = auto_drainer_t::lock_t(
                &chosen_relationship->drainer);
            direct_readers_to_contact.push_back(std::move(new_op_info));
            new_op_info.init(new outdated_read_info_t());
        }
    }

    std::vector<read_response_t> results(direct_readers_to_contact.size());
    std::vector<std::string> failures(direct_readers_to_contact.size());
    pmap(direct_readers_to_contact.size(), std::bind(&cluster_namespace_interface_t::perform_outdated_read, this,
                                                     &direct_readers_to_contact, &results, &failures, ph::_1, interruptor));

    if (interruptor->is_pulsed()) throw interrupted_exc_t();

    for (size_t i = 0; i < direct_readers_to_contact.size(); ++i) {
        if (!failures[i].empty()) {
            throw cannot_perform_query_exc_t(failures[i]);
        }
    }

    op.unshard(results.data(), results.size(), response, ctx, interruptor);
}

void cluster_namespace_interface_t::perform_outdated_read(
        std::vector<scoped_ptr_t<outdated_read_info_t> > *direct_readers_to_contact,
        std::vector<read_response_t> *results,
        std::vector<std::string> *failures,
        int i,
        signal_t *interruptor) THROWS_NOTHING {
    outdated_read_info_t *direct_reader_to_contact = (*direct_readers_to_contact)[i].get();

    try {
        cond_t done;
        mailbox_t<void(read_response_t)> cont(mailbox_manager,
            [&](signal_t *, const read_response_t &res) {
                results->at(i) = res;
                done.pulse();
            });

        send(mailbox_manager,
            direct_reader_to_contact->direct_reader_bcard->read_mailbox,
            direct_reader_to_contact->sharded_op,
            cont.get_address());
        wait_any_t waiter(direct_reader_to_contact->keepalive.get_drain_signal(), &done);
        wait_interruptible(&waiter, interruptor);
    } catch (const interrupted_exc_t &) {
        if (interruptor->is_pulsed()) {
            /* Return immediately. `dispatch_immediate_op()` will notice that the
            interruptor has been pulsed. */
            return;
        } else {
            /* `keepalive.get_drain_signal()` was pulsed because the other server
            disconnected or stopped being a replica */
            failures->at(i).assign("lost contact with replica");
        }
    }
}

void cluster_namespace_interface_t::update_registrant(
        const std::pair<peer_id_t, uuid_u> &key,
        const replica_business_card_t *bcard) {
    auto it = coro_stoppers.find(key);
    if (bcard == nullptr && it != coro_stoppers.end()) {
        it->second->pulse_if_not_already_pulsed();
    } else if (bcard != nullptr && it == coro_stoppers.end()) {
        coro_stoppers.insert(std::make_pair(key, make_scoped<cond_t>()));
        if (starting_up) {
            start_count++;
        }
        coro_t::spawn_sometime(std::bind(
            &cluster_namespace_interface_t::relationship_coroutine, this,
            key, *bcard, starting_up, relationship_coroutine_auto_drainer.lock()));
    }
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

void cluster_namespace_interface_t::relationship_coroutine(
        const std::pair<peer_id_t, uuid_u> &key,
        const replica_business_card_t &bcard,
        bool is_start,
        auto_drainer_t::lock_t lock) THROWS_NOTHING {
    wait_any_t stop_signal(lock.get_drain_signal(), coro_stoppers.at(key).get());
    try {
        relationship_t relationship_record;
        relationship_record.is_local =
            (key.first == mailbox_manager->get_connectivity_cluster()->get_me());
        relationship_record.region = bcard.region;

        scoped_ptr_t<master_access_t> master_access;
        if (static_cast<bool>(bcard.master)) {
            master_access.init(new master_access_t(
                mailbox_manager, *bcard.master, &stop_signal));
            relationship_record.master_access = master_access.get();
        } else {
            relationship_record.master_access = nullptr;
        }

        if (static_cast<bool>(bcard.direct_reader)) {
            relationship_record.direct_reader_bcard = &*bcard.direct_reader;
        } else {
            relationship_record.direct_reader_bcard = nullptr;
        }

        region_map_set_membership_t<relationship_t *> relationship_map_insertion(
            &relationships, bcard.region, &relationship_record);

        if (is_start) {
            guarantee(start_count > 0);
            start_count--;
            if (start_count == 0) {
                start_cond.pulse();
            }
            is_start = false;
        }

        wait_any_t waiter(lock.get_drain_signal(), coro_stoppers[key].get());
        waiter.wait_lazily_unordered();
    } catch (const interrupted_exc_t &) {
        /* do nothing */
    } 

    if (is_start) {
        guarantee(start_count > 0);
        start_count--;
        if (start_count == 0) {
            start_cond.pulse();
        }
    }

    coro_stoppers.erase(key);

    /* If we disconnect and then reconnect quickly, then `update_registrant()` won't
    spawn a new coroutine because the old entry is still present in `coro_stoppers`. So
    we have to manually call `update_registrant()` again to spawn a new coroutine in this
    case. */
    if (!lock.get_drain_signal()->is_pulsed()) {
        directory->read_key(key,
            [&](const replica_business_card_t *new_bcard) {
                update_registrant(key, new_bcard);
            });
    }
}


