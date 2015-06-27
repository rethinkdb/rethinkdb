// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/query_routing/table_query_client.hpp"

#include <functional>

#include "clustering/query_routing/primary_query_client.hpp"
#include "clustering/table_contract/cpu_sharding.hpp"
#include "clustering/table_manager/multi_table_manager.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "concurrency/fifo_enforcer.hpp"
#include "concurrency/watchable.hpp"
#include "rdb_protocol/env.hpp"

table_query_client_t::table_query_client_t(
        const namespace_id_t &_table_id,
        mailbox_manager_t *mm,
        watchable_map_t<std::pair<peer_id_t, uuid_u>, table_query_bcard_t> *d,
        multi_table_manager_t *mtm,
        rdb_context_t *_ctx)
    : table_id(_table_id),
      mailbox_manager(mm),
      directory(d),
      multi_table_manager(mtm),
      ctx(_ctx),
      start_count(0),
      starting_up(true),
      subs(directory,
        std::bind(&table_query_client_t::update_registrant,
            this, ph::_1, ph::_2),
        initial_call_t::YES) {
    rassert(ctx != NULL);
    starting_up = false;
    if (start_count == 0) {
        start_cond.pulse();
    }
}

bool table_query_client_t::check_readiness(table_readiness_t readiness,
                                           signal_t *interruptor) {
    rassert(readiness != table_readiness_t::finished,
            "Cannot check for the 'finished' state with namespace_interface_t.");
    try {
        switch (readiness) {
        case table_readiness_t::outdated_reads:
            {
                read_response_t res;
                read_t r(dummy_read_t(), profile_bool_t::DONT_PROFILE,
                         read_mode_t::OUTDATED);
                read(r, &res, order_token_t::ignore, interruptor);
            }
            break;
        case table_readiness_t::reads:
            {
                read_response_t res;
                read_t r(dummy_read_t(), profile_bool_t::DONT_PROFILE,
                         read_mode_t::SINGLE);
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

void table_query_client_t::read(
        const read_t &r,
        read_response_t *response,
        order_token_t order_token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t) {
    order_token.assert_read_mode();
    if (r.read_mode == read_mode_t::OUTDATED) {
        guarantee(!r.route_to_primary());
        /* This seems kind of silly. We do it this way because
           `dispatch_outdated_read` needs to be able to see `outdated_read_info_t`,
           which is defined in the `private` section. */
        dispatch_outdated_read(r, response, interruptor);
    } else if (r.read_mode == read_mode_t::DEBUG_DIRECT) {
        guarantee(!r.route_to_primary());
        dispatch_debug_direct_read(r, response, interruptor);
    } else {
        dispatch_immediate_op<read_t, fifo_enforcer_sink_t::exit_read_t, read_response_t>(
                &primary_query_client_t::new_read_token,
                &primary_query_client_t::read,
                r, response, order_token, interruptor);
    }
}

void table_query_client_t::write(
        const write_t &w,
        write_response_t *response,
        order_token_t order_token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t) {
    order_token.assert_write_mode();
    dispatch_immediate_op<write_t, fifo_enforcer_sink_t::exit_write_t, write_response_t>(
        &primary_query_client_t::new_write_token,
        &primary_query_client_t::write,
        w, response, order_token, interruptor);
}

std::set<region_t> table_query_client_t::get_sharding_scheme()
        THROWS_ONLY(cannot_perform_query_exc_t) {
    std::vector<region_t> s;
    relationships.visit(relationships.get_domain(),
    [&](const region_t &, const std::set<relationship_t *> &rels) {
        for (relationship_t *rel : rels) {
            s.push_back(rel->region);
        }
    });
    region_t whole;
    region_join_result_t res = region_join(s, &whole);
    if (res != REGION_JOIN_OK || whole != region_t::universe()) {
        throw cannot_perform_query_exc_t("cannot compute sharding scheme "
                                         "because primary replicas are "
                                         "unavailable or duplicated",
                                         query_state_t::FAILED);
    }
    return std::set<region_t>(
        std::make_move_iterator(s.begin()),
        std::make_move_iterator(s.end()));
}

template<class op_type, class fifo_enforcer_token_type, class op_response_type>
void table_query_client_t::dispatch_immediate_op(
    /* `how_to_make_token` and `how_to_run_query` have type pointer-to-
       member-function. */
    void (primary_query_client_t::*how_to_make_token)(
        fifo_enforcer_token_type *),
    void (primary_query_client_t::*how_to_run_query)(
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
        primaries_to_contact;
    scoped_ptr_t<immediate_op_info_t<op_type, fifo_enforcer_token_type> >
        new_op_info(new immediate_op_info_t<op_type, fifo_enforcer_token_type>());
    relationships.visit(region_t::universe(),
    [&](const region_t &reg, const std::set<relationship_t *> &rels) {
        if (op.shard(reg, &new_op_info->sharded_op)) {
            relationship_t *chosen_relationship = NULL;
            for (auto jt = rels.begin(); jt != rels.end(); ++jt) {
                if ((*jt)->primary_client) {
                    if (chosen_relationship) {
                        throw cannot_perform_query_exc_t(
                            "too many primary replicas available",
                            query_state_t::FAILED);
                    }
                    chosen_relationship = *jt;
                }
            }
            if (!chosen_relationship) {
                throw cannot_perform_query_exc_t(
                    strprintf("primary replica for shard %s not available",
                              key_range_to_string(reg.inner).c_str()),
                    query_state_t::FAILED);
            }
            new_op_info->primary_client = chosen_relationship->primary_client;
            (new_op_info->primary_client->*how_to_make_token)(
                &new_op_info->enforcement_token);
            new_op_info->keepalive = auto_drainer_t::lock_t(
                &chosen_relationship->drainer);
            primaries_to_contact.push_back(std::move(new_op_info));
            new_op_info.init(
                new immediate_op_info_t<op_type, fifo_enforcer_token_type>());
        }
    });

    std::vector<op_response_type> results(primaries_to_contact.size());
    std::vector<boost::optional<cannot_perform_query_exc_t> >
        failures(primaries_to_contact.size());
    pmap(primaries_to_contact.size(), std::bind(
             &table_query_client_t::template perform_immediate_op<
                 op_type, fifo_enforcer_token_type, op_response_type>,
             this,
             how_to_run_query,
             &primaries_to_contact,
             &results,
             &failures,
             order_token,
             ph::_1,
             interruptor));

    if (interruptor->is_pulsed()) throw interrupted_exc_t();

    bool seen_non_failure = false;
    boost::optional<cannot_perform_query_exc_t> first_failure;
    for (size_t i = 0; i < primaries_to_contact.size(); ++i) {
        if (failures[i]) {
            switch (failures[i]->get_query_state()) {
            case query_state_t::FAILED:
                if (!first_failure) first_failure = failures[i];
                break;
            case query_state_t::INDETERMINATE: throw *failures[i];
            default: unreachable();
            }
        } else {
            seen_non_failure = true;
        }
        if (seen_non_failure && first_failure) {
            // If we got different responses from the different shards, we
            // default to the safest error type.
            throw cannot_perform_query_exc_t(
                first_failure->what(), query_state_t::INDETERMINATE);
        }
    }
    if (first_failure) {
        guarantee(!seen_non_failure);
        throw *first_failure;
    }

    op.unshard(results.data(), results.size(), response, ctx, interruptor);
}

template<class op_type, class fifo_enforcer_token_type, class op_response_type>
void table_query_client_t::perform_immediate_op(
    void (primary_query_client_t::*how_to_run_query)(
        const op_type &,
        op_response_type *,
        order_token_t,
        fifo_enforcer_token_type *,
        signal_t *)
    /* THROWS_ONLY(interrupted_exc_t, resource_lost_exc_t,
       cannot_perform_query_exc_t) */,
    std::vector<scoped_ptr_t<immediate_op_info_t<op_type, fifo_enforcer_token_type> > >
        *primaries_to_contact,
    std::vector<op_response_type> *results,
    std::vector<boost::optional<cannot_perform_query_exc_t> > *failures,
    order_token_t order_token,
    size_t i,
    signal_t *interruptor)
    THROWS_NOTHING
{
    immediate_op_info_t<op_type, fifo_enforcer_token_type> *primary_to_contact
        = (*primaries_to_contact)[i].get();

    try {
        wait_any_t waiter(primary_to_contact->keepalive.get_drain_signal(), interruptor);
        (primary_to_contact->primary_client->*how_to_run_query)(
            primary_to_contact->sharded_op,
            &results->at(i),
            order_token,
            &primary_to_contact->enforcement_token,
            &waiter);
    } catch (const cannot_perform_query_exc_t& e) {
        (*failures)[i] = e;
    } catch (const interrupted_exc_t&) {
        if (interruptor->is_pulsed()) {
            /* Return immediately. `dispatch_immediate_op()` will notice that the
            interruptor has been pulsed. */
            return;
        } else {
            /* `keepalive.get_drain_signal()` was pulsed because the other server
            disconnected or stopped being a primary */
            (*failures)[i] = cannot_perform_query_exc_t(
                "lost contact with primary replica",
                query_state_t::INDETERMINATE);
        }
    }
}

void table_query_client_t::dispatch_outdated_read(
    const read_t &op,
    read_response_t *response,
    signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t) {

    if (interruptor->is_pulsed()) throw interrupted_exc_t();

    std::vector<scoped_ptr_t<outdated_read_info_t> > replicas_to_contact;

    scoped_ptr_t<outdated_read_info_t> new_op_info(new outdated_read_info_t());
    relationships.visit(region_t::universe(),
    [&](const region_t &region, const std::set<relationship_t *> &rels) {
        if (op.shard(region, &new_op_info->sharded_op)) {
            std::vector<relationship_t *> potential_relationships;
            relationship_t *chosen_relationship = NULL;
            for (auto jt = rels.begin(); jt != rels.end(); ++jt) {
                if ((*jt)->direct_bcard != nullptr) {
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
                    "no replica is available",
                    query_state_t::FAILED);
            }
            new_op_info->direct_bcard = chosen_relationship->direct_bcard;
            new_op_info->keepalive = auto_drainer_t::lock_t(
                &chosen_relationship->drainer);
            replicas_to_contact.push_back(std::move(new_op_info));
            new_op_info.init(new outdated_read_info_t());
        }
    });

    std::vector<read_response_t> results(replicas_to_contact.size());
    std::vector<std::string> failures(replicas_to_contact.size());
    pmap(replicas_to_contact.size(),
        std::bind(&table_query_client_t::perform_outdated_read, this,
            &replicas_to_contact, &results, &failures, ph::_1, interruptor));

    if (interruptor->is_pulsed()) throw interrupted_exc_t();

    for (size_t i = 0; i < replicas_to_contact.size(); ++i) {
        if (!failures[i].empty()) {
            // Reads are never indeterminate.
            throw cannot_perform_query_exc_t(failures[i], query_state_t::FAILED);
        }
    }

    op.unshard(results.data(), results.size(), response, ctx, interruptor);
}

void table_query_client_t::perform_outdated_read(
        std::vector<scoped_ptr_t<outdated_read_info_t> > *replicas_to_contact,
        std::vector<read_response_t> *results,
        std::vector<std::string> *failures,
        size_t i,
        signal_t *interruptor) THROWS_NOTHING {
    outdated_read_info_t *replica_to_contact = (*replicas_to_contact)[i].get();

    try {
        cond_t done;
        mailbox_t<void(read_response_t)> cont(mailbox_manager,
            [&](signal_t *, const read_response_t &res) {
                results->at(i) = res;
                done.pulse();
            });

        send(mailbox_manager,
            replica_to_contact->direct_bcard->read_mailbox,
            replica_to_contact->sharded_op,
            cont.get_address());
        wait_any_t waiter(replica_to_contact->keepalive.get_drain_signal(), &done);
        wait_interruptible(&waiter, interruptor);
        if (!done.is_pulsed()) {
            /* `wait_interruptible()` returned because
            `replica_to_contact->keepalive.get_drain_signal()` was pulsed */
            failures->at(i).assign("lost contact with replica");
        }
    } catch (const interrupted_exc_t &) {
        /* Return immediately. `dispatch_immediate_op()` will notice that the
        interruptor has been pulsed. */
    }
}

void table_query_client_t::dispatch_debug_direct_read(
        const read_t &op,
        read_response_t *response,
        signal_t *interruptor_on_caller)
        THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t) {
    cross_thread_signal_t interruptor_on_mtm(
        interruptor_on_caller, multi_table_manager->home_thread());
    on_thread_t thread_switcher(multi_table_manager->home_thread());
    multi_table_manager->visit_table(table_id, &interruptor_on_mtm,
    [&](multistore_ptr_t *multistore, table_manager_t *) {
        if (multistore == nullptr) {
            throw cannot_perform_query_exc_t(
                "This server does not have any data available for the given table.",
                query_state_t::FAILED);
        }
        std::vector<read_response_t> responses;
        pmap(CPU_SHARDING_FACTOR, [&](size_t shard_number) {
            try {
                region_t region = cpu_sharding_subspace(shard_number);
                read_t subread;
                if (!op.shard(region, &subread)) {
                    return;
                }
                read_response_t subresponse;
                {
                    store_view_t *store =
                        multistore->get_cpu_sharded_store(shard_number);
                    cross_thread_signal_t interruptor_on_store(
                        &interruptor_on_mtm, store->home_thread());
                    on_thread_t thread_switcher_2(store->home_thread());
#ifndef NDEBUG
                    metainfo_checker_t checker(
                        region, [](const region_t &, const binary_blob_t &) {});
#endif /* NDEBUG */
                    read_token_t token;
                    store->new_read_token(&token);
                    store->read(
                        DEBUG_ONLY(checker, )
                        subread, &subresponse, &token,
                        &interruptor_on_store);
                }
                responses.push_back(subresponse);
            } catch (const interrupted_exc_t &) {
                /* ignore; we'll catch it outside the pmap */
            }
        });
        if (interruptor_on_mtm.is_pulsed()) {
            throw interrupted_exc_t();
        }
        op.unshard(responses.data(), responses.size(), response, ctx,
            &interruptor_on_mtm);
    });
}

void table_query_client_t::update_registrant(
        const std::pair<peer_id_t, uuid_u> &key,
        const table_query_bcard_t *bcard) {
    auto it = coro_stoppers.find(key);
    if (bcard == nullptr && it != coro_stoppers.end()) {
        it->second->pulse_if_not_already_pulsed();
    } else if (bcard != nullptr && it == coro_stoppers.end()) {
        coro_stoppers.insert(std::make_pair(key, make_scoped<cond_t>()));
        if (starting_up) {
            start_count++;
        }
        coro_t::spawn_sometime(std::bind(
            &table_query_client_t::relationship_coroutine, this,
            key, *bcard, starting_up, relationship_coroutine_auto_drainer.lock()));
    }
}

template <class value_t>
class region_map_set_membership_t {
public:
    region_map_set_membership_t(region_map_t<std::set<value_t> > *m, const region_t &r, const value_t &v) :
        map(m), region(r), value(v) {
        map->visit_mutable(region, [&](const region_t &, std::set<value_t> *set) {
            rassert(set->count(value) == 0);
            set->insert(value);
        });
    }
    ~region_map_set_membership_t() {
        map->visit_mutable(region, [&](const region_t &, std::set<value_t> *set) {
            rassert(set->count(value) == 1);
            set->erase(value);
        });
    }
private:
    region_map_t<std::set<value_t> > *map;
    region_t region;
    value_t value;
};

void table_query_client_t::relationship_coroutine(
        const std::pair<peer_id_t, uuid_u> &key,
        const table_query_bcard_t &bcard,
        bool is_start,
        auto_drainer_t::lock_t lock) THROWS_NOTHING {
    try {
        wait_any_t stop_signal(lock.get_drain_signal(), coro_stoppers.at(key).get());

        relationship_t relationship_record;
        relationship_record.is_local =
            (key.first == mailbox_manager->get_connectivity_cluster()->get_me());
        relationship_record.region = bcard.region;

        scoped_ptr_t<primary_query_client_t> primary_client;
        if (static_cast<bool>(bcard.primary)) {
            primary_client.init(new primary_query_client_t(
                mailbox_manager, *bcard.primary, &stop_signal));
            relationship_record.primary_client = primary_client.get();
        } else {
            relationship_record.primary_client = nullptr;
        }

        if (static_cast<bool>(bcard.direct)) {
            relationship_record.direct_bcard = &*bcard.direct;
        } else {
            relationship_record.direct_bcard = nullptr;
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

        stop_signal.wait_lazily_unordered();
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
            [&](const table_query_bcard_t *new_bcard) {
                update_registrant(key, new_bcard);
            });
    }
}


