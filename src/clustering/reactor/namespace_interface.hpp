#ifndef CLUSTERING_REACTOR_NAMESPACE_INTERFACE_HPP_
#define CLUSTERING_REACTOR_NAMESPACE_INTERFACE_HPP_

#include <math.h>

#include <algorithm>
#include <map>
#include <string>
#include <vector>
#include <set>

#include "errors.hpp"
#include <boost/ptr_container/ptr_vector.hpp>

#include "arch/timing.hpp"
#include "clustering/immediate_consistency/query/master_access.hpp"
#include "clustering/reactor/metadata.hpp"
#include "clustering/registrant.hpp"
#include "concurrency/fifo_enforcer.hpp"
#include "concurrency/pmap.hpp"
#include "concurrency/promise.hpp"
#include "protocol_api.hpp"

template <class protocol_t, class value_t>
class region_map_set_membership_t {
public:
    region_map_set_membership_t(region_map_t<protocol_t, std::set<value_t> > *m, const typename protocol_t::region_t &r, const value_t &v) :
        map(m), region(r), value(v) {
        region_map_t<protocol_t, std::set<value_t> > submap = map->mask(region);
        for (typename region_map_t<protocol_t, std::set<value_t> >::iterator it = submap.begin(); it != submap.end(); it++) {
            it->second.insert(value);
        }
        map->update(submap);
    }
    ~region_map_set_membership_t() {
        region_map_t<protocol_t, std::set<value_t> > submap = map->mask(region);
        for (typename region_map_t<protocol_t, std::set<value_t> >::iterator it = submap.begin(); it != submap.end(); it++) {
            it->second.erase(value);
        }
        map->update(submap);
    }
private:
    region_map_t<protocol_t, std::set<value_t> > *map;
    typename protocol_t::region_t region;
    value_t value;
};

template <class protocol_t>
class cluster_namespace_interface_t :
    public namespace_interface_t<protocol_t>
{
public:
    cluster_namespace_interface_t(
            mailbox_manager_t *mm,
            clone_ptr_t<watchable_t<std::map<peer_id_t, reactor_business_card_t<protocol_t> > > > dv) :
        mailbox_manager(mm),
        directory_view(dv),
        start_count(0),
        watcher_subscription(boost::bind(&cluster_namespace_interface_t::update_registrants, this, false))
    {
        {
            typename watchable_t< std::map<peer_id_t, reactor_business_card_t<protocol_t> > >::freeze_t freeze(directory_view);
            update_registrants(true);
            watcher_subscription.reset(directory_view, &freeze);
        }
        if (start_count == 0) {
            start_cond.pulse();
        }
    }

    /* Returns a signal that will be pulsed when we have either successfully
    connected or tried and failed to connect to every master that was present
    at the time that the constructor was called. This is to avoid the case where
    we get errors like "lost contact with master" when really we just haven't
    finished connecting yet. */
    signal_t *get_initial_ready_signal() {
        return &start_cond;
    }

    typename protocol_t::read_response_t read(typename protocol_t::read_t r, order_token_t order_token, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t) {
        return dispatch_immediate_op<typename protocol_t::read_t, fifo_enforcer_sink_t::exit_read_t, typename protocol_t::read_response_t>(
            &master_access_t<protocol_t>::new_read_token, &master_access_t<protocol_t>::read,
            r, order_token, interruptor);
    }

    typename protocol_t::read_response_t read_outdated(typename protocol_t::read_t r, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t) {
        /* This seems kind of silly. We do it this way because
        `dispatch_outdated_read` needs to be able to see `outdated_read_info_t`,
        which is defined in the `private` section. */
        return dispatch_outdated_read(r, interruptor);
    }

    typename protocol_t::write_response_t write(typename protocol_t::write_t w, order_token_t order_token, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t) {
        return dispatch_immediate_op<typename protocol_t::write_t, fifo_enforcer_sink_t::exit_write_t, typename protocol_t::write_response_t>(
            &master_access_t<protocol_t>::new_write_token, &master_access_t<protocol_t>::write,
            w, order_token, interruptor);
    }

    std::set<typename protocol_t::region_t> get_sharding_scheme() THROWS_ONLY(cannot_perform_query_exc_t) {
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

private:
    class relationship_t {
    public:
        bool is_local;
        typename protocol_t::region_t region;
        master_access_t<protocol_t> *master_access;
        resource_access_t<direct_reader_business_card_t<protocol_t> > *direct_reader_access;
        auto_drainer_t drainer;
    };

    /* The code for handling immediate reads is 99% the same as the code for
    handling writes, so it's factored out into the `dispatch_immediate_op()`
    function. */

    template<class fifo_enforcer_token_type>
    class immediate_op_info_t {
    public:
        typename protocol_t::region_t region;
        master_access_t<protocol_t> *master_access;
        fifo_enforcer_token_type enforcement_token;
        auto_drainer_t::lock_t keepalive;
    };

    class outdated_read_info_t {
    public:
        typename protocol_t::region_t region;
        resource_access_t<direct_reader_business_card_t<protocol_t> > *direct_reader_access;
        auto_drainer_t::lock_t keepalive;
    };

    template<class op_type, class fifo_enforcer_token_type, class op_response_type>
    op_response_type dispatch_immediate_op(
            /* `how_to_make_token` and `how_to_run_query` have type pointer-to-
            member-function. */
            void (master_access_t<protocol_t>::*how_to_make_token)(fifo_enforcer_token_type *),
            op_response_type (master_access_t<protocol_t>::*how_to_run_query)(const op_type &, order_token_t, fifo_enforcer_token_type *, signal_t *) THROWS_ONLY(interrupted_exc_t, resource_lost_exc_t, cannot_perform_query_exc_t),
            op_type op,
            order_token_t order_token,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t)
    {
        if (interruptor->is_pulsed()) throw interrupted_exc_t();

        boost::ptr_vector<immediate_op_info_t<fifo_enforcer_token_type> > masters_to_contact;
        {
            region_map_t<protocol_t, std::set<relationship_t *> > submap = relationships.mask(op.get_region());
            for (typename region_map_t<protocol_t, std::set<relationship_t *> >::iterator it = submap.begin(); it != submap.end(); it++) {
                relationship_t *chosen_relationship = NULL;
                for (typename std::set<relationship_t *>::const_iterator jt = it->second.begin(); jt != it->second.end(); jt++) {
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
                immediate_op_info_t<fifo_enforcer_token_type> *new_op_info =
                    new immediate_op_info_t<fifo_enforcer_token_type>();
                new_op_info->region = it->first;
                new_op_info->master_access = chosen_relationship->master_access;
                (new_op_info->master_access->*how_to_make_token)(&new_op_info->enforcement_token);
                new_op_info->keepalive = auto_drainer_t::lock_t(&chosen_relationship->drainer);
                masters_to_contact.push_back(new_op_info);
            }
        }

        std::vector<boost::variant<op_response_type, std::string> > results_or_failures(masters_to_contact.size());
        pmap(masters_to_contact.size(), boost::bind(
            &cluster_namespace_interface_t::template perform_immediate_op<op_type, fifo_enforcer_token_type, op_response_type>, this,
            how_to_run_query, &masters_to_contact, &op, order_token, &results_or_failures, _1, interruptor));

        if (interruptor->is_pulsed()) throw interrupted_exc_t();

        std::vector<op_response_type> results;
        for (int i = 0; i < int(masters_to_contact.size()); i++) {
            if (std::string *error = boost::get<std::string>(&results_or_failures[i])) {
                throw cannot_perform_query_exc_t(*error);
            } else {
                results.push_back(boost::get<op_response_type>(results_or_failures[i]));
            }
        }

        return op.unshard(results, &temporary_cache);
    }

    template<class op_type, class fifo_enforcer_token_type, class op_response_type>
    void perform_immediate_op(
            op_response_type (master_access_t<protocol_t>::*how_to_run_query)(const op_type &, order_token_t, fifo_enforcer_token_type *, signal_t *) THROWS_ONLY(interrupted_exc_t, resource_lost_exc_t, cannot_perform_query_exc_t),
            boost::ptr_vector<immediate_op_info_t<fifo_enforcer_token_type> > *masters_to_contact,
            const op_type *operation,
            order_token_t order_token,
            std::vector<boost::variant<op_response_type, std::string> > *results_or_failures,
            int i,
            signal_t *interruptor)
            THROWS_NOTHING
    {
        immediate_op_info_t<fifo_enforcer_token_type> *master_to_contact = &(*masters_to_contact)[i];
        op_type shard = operation->shard(master_to_contact->region);
        rassert(region_is_superset(master_to_contact->region, shard.get_region()));
        rassert(region_is_superset(operation->get_region(), shard.get_region()));

        try {
            (*results_or_failures)[i] = (master_to_contact->master_access->*how_to_run_query)(
                shard,
                order_token,
                &master_to_contact->enforcement_token,
                interruptor);
        } catch (const resource_lost_exc_t&) {
            (*results_or_failures)[i] = "lost contact with master";
        } catch (const cannot_perform_query_exc_t& e) {
            (*results_or_failures)[i] = "master error: " + std::string(e.what());
        } catch (const interrupted_exc_t&) {
            rassert(interruptor->is_pulsed());
            /* Ignore `interrupted_exc_t` and just return immediately.
            `dispatch_immediate_op()` will notice the interruptor has been
            pulsed and won't try to access our result. */
        }
    }

    typename protocol_t::read_response_t dispatch_outdated_read(
            const typename protocol_t::read_t &op,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t)
    {
        if (interruptor->is_pulsed()) throw interrupted_exc_t();

        boost::ptr_vector<outdated_read_info_t> direct_readers_to_contact;
        {
            region_map_t<protocol_t, std::set<relationship_t *> > submap = relationships.mask(op.get_region());
            for (typename region_map_t<protocol_t, std::set<relationship_t *> >::iterator it = submap.begin(); it != submap.end(); it++) {
                std::vector<relationship_t *> potential_relationships;
                relationship_t *chosen_relationship = NULL;
                for (typename std::set<relationship_t *>::const_iterator jt = it->second.begin(); jt != it->second.end(); jt++) {
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
                    chosen_relationship = potential_relationships[distributor_rng.randint(potential_relationships.size())];
                }
                if (!chosen_relationship) {
                    /* Don't bother looking for masters; if there are no direct
                    readers, there won't be any masters either. */
                    throw cannot_perform_query_exc_t("No direct reader available");
                }
                outdated_read_info_t *new_op_info = new outdated_read_info_t();
                new_op_info->region = it->first;
                new_op_info->direct_reader_access = chosen_relationship->direct_reader_access;
                new_op_info->keepalive = auto_drainer_t::lock_t(&chosen_relationship->drainer);
                direct_readers_to_contact.push_back(new_op_info);
            }
        }

        std::vector<boost::variant<typename protocol_t::read_response_t, std::string> > results_or_failures(direct_readers_to_contact.size());
        pmap(direct_readers_to_contact.size(), boost::bind(
            &cluster_namespace_interface_t::perform_outdated_read, this,
            &direct_readers_to_contact, &op, &results_or_failures, _1, interruptor));

        if (interruptor->is_pulsed()) throw interrupted_exc_t();

        std::vector<typename protocol_t::read_response_t> results;
        for (int i = 0; i < int(direct_readers_to_contact.size()); i++) {
            if (std::string *error = boost::get<std::string>(&results_or_failures[i])) {
                throw cannot_perform_query_exc_t(*error);
            } else {
                results.push_back(boost::get<typename protocol_t::read_response_t>(results_or_failures[i]));
            }
        }

        return op.unshard(results, &temporary_cache);
    }

    void perform_outdated_read(
            boost::ptr_vector<outdated_read_info_t> *direct_readers_to_contact,
            const typename protocol_t::read_t *operation,
            std::vector<boost::variant<typename protocol_t::read_response_t, std::string> > *results_or_failures,
            int i,
            signal_t *interruptor)
            THROWS_NOTHING
    {
        outdated_read_info_t *direct_reader_to_contact = &(*direct_readers_to_contact)[i];
        typename protocol_t::read_t shard = operation->shard(direct_reader_to_contact->region);
        rassert(region_is_superset(direct_reader_to_contact->region, shard.get_region()));
        rassert(region_is_superset(operation->get_region(), shard.get_region()));

        try {
            promise_t<typename protocol_t::read_response_t> promise;
            mailbox_t<void(typename protocol_t::read_response_t)> cont(
                mailbox_manager,
                boost::bind(&promise_t<typename protocol_t::read_response_t>::pulse, &promise, _1),
                mailbox_callback_mode_inline);
            send(mailbox_manager, direct_reader_to_contact->direct_reader_access->access().read_mailbox, shard, cont.get_address());
            wait_any_t waiter(direct_reader_to_contact->direct_reader_access->get_failed_signal(), promise.get_ready_signal());
            wait_interruptible(&waiter, interruptor);
            direct_reader_to_contact->direct_reader_access->access();   /* throws if `get_failed_signal()->is_pulsed()` */
            (*results_or_failures)[i] = promise.get_value();
        } catch (resource_lost_exc_t) {
            (*results_or_failures)[i] = "lost contact with direct reader";
        } catch (interrupted_exc_t) {
            rassert(interruptor->is_pulsed());
            /* Ignore `interrupted_exc_t` and return immediately.
            `read_outdated()` will notice that the interruptor has been pulsed
            and won't try to access our result. */
        }
    }

    void update_registrants(bool is_start) {
        ASSERT_NO_CORO_WAITING;
        std::map<peer_id_t, reactor_business_card_t<protocol_t> > existings = directory_view->get();

        for (typename std::map<peer_id_t, reactor_business_card_t<protocol_t> >::const_iterator it = existings.begin(); it != existings.end(); ++it) {
            for (typename reactor_business_card_t<protocol_t>::activity_map_t::const_iterator amit = it->second.activities.begin(); amit != it->second.activities.end(); ++amit) {
                bool has_anything_useful, is_primary;
                if (const reactor_business_card_details::primary_t<protocol_t> *primary = boost::get<reactor_business_card_details::primary_t<protocol_t> >(&amit->second.second)) {
                    if (primary->master) {
                        has_anything_useful = true;
                        is_primary = true;
                    } else {
                        has_anything_useful = false;
                    }
                } else if (boost::get<reactor_business_card_details::secondary_up_to_date_t<protocol_t> >(&amit->second.second)) {
                    has_anything_useful = true;
                    is_primary = false;
                } else {
                    has_anything_useful = false;
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
                        coro_t::spawn_sometime(boost::bind(
                            &cluster_namespace_interface_t::relationship_coroutine, this,
                            it->first, id, is_start, is_primary, amit->second.first,
                            auto_drainer_t::lock_t(&relationship_coroutine_auto_drainer)
                            ));
                    }
                }
            }
        }
    }

    static boost::optional<boost::optional<master_business_card_t<protocol_t> > > extract_master_business_card(
            const std::map<peer_id_t, reactor_business_card_t<protocol_t> > &map, const peer_id_t &peer, const reactor_activity_id_t &activity_id) {
        boost::optional<boost::optional<master_business_card_t<protocol_t> > > ret;
        typename std::map<peer_id_t, reactor_business_card_t<protocol_t> >::const_iterator it = map.find(peer);
        if (it != map.end()) {
            ret = boost::optional<master_business_card_t<protocol_t> >();
            typename reactor_business_card_t<protocol_t>::activity_map_t::const_iterator jt = it->second.activities.find(activity_id);
            if (jt != it->second.activities.end()) {
                if (const reactor_business_card_details::primary_t<protocol_t> *primary_record =
                        boost::get<reactor_business_card_details::primary_t<protocol_t> >(&jt->second.second)) {
                    if (primary_record->master) {
                        ret.get() = primary_record->master.get();
                    }
                }
            }
        }
        return ret;
    }

    static boost::optional<boost::optional<direct_reader_business_card_t<protocol_t> > > extract_direct_reader_business_card_from_primary(
            const std::map<peer_id_t, reactor_business_card_t<protocol_t> > &map, const peer_id_t &peer, const reactor_activity_id_t &activity_id) {
        boost::optional<boost::optional<direct_reader_business_card_t<protocol_t> > > ret;
        typename std::map<peer_id_t, reactor_business_card_t<protocol_t> >::const_iterator it = map.find(peer);
        if (it != map.end()) {
            ret = boost::optional<direct_reader_business_card_t<protocol_t> >();
            typename reactor_business_card_t<protocol_t>::activity_map_t::const_iterator jt = it->second.activities.find(activity_id);
            if (jt != it->second.activities.end()) {
                if (const reactor_business_card_details::primary_t<protocol_t> *primary_record =
                        boost::get<reactor_business_card_details::primary_t<protocol_t> >(&jt->second.second)) {
                    if (primary_record->direct_reader) {
                        ret.get() = primary_record->direct_reader.get();
                    }
                }
            }
        }
        return ret;
    }

    static boost::optional<boost::optional<direct_reader_business_card_t<protocol_t> > > extract_direct_reader_business_card_from_secondary_up_to_date(
            const std::map<peer_id_t, reactor_business_card_t<protocol_t> > &map, const peer_id_t &peer, const reactor_activity_id_t &activity_id) {
        boost::optional<boost::optional<direct_reader_business_card_t<protocol_t> > > ret;
        typename std::map<peer_id_t, reactor_business_card_t<protocol_t> >::const_iterator it = map.find(peer);
        if (it != map.end()) {
            ret = boost::optional<direct_reader_business_card_t<protocol_t> >();
            typename reactor_business_card_t<protocol_t>::activity_map_t::const_iterator jt = it->second.activities.find(activity_id);
            if (jt != it->second.activities.end()) {
                if (const reactor_business_card_details::secondary_up_to_date_t<protocol_t> *secondary_up_to_date_record =
                        boost::get<reactor_business_card_details::secondary_up_to_date_t<protocol_t> >(&jt->second.second)) {
                    ret.get() = secondary_up_to_date_record->direct_reader;
                }
            }
        }
        return ret;
    }

    void relationship_coroutine(peer_id_t peer_id, reactor_activity_id_t activity_id,
            bool is_start, bool is_primary, const typename protocol_t::region_t &region,
            auto_drainer_t::lock_t lock) THROWS_NOTHING {
        try {
            scoped_ptr_t<master_access_t<protocol_t> > master_access;
            scoped_ptr_t<resource_access_t<direct_reader_business_card_t<protocol_t> > > direct_reader_access;
            if (is_primary) {
                master_access.init(new master_access_t<protocol_t>(
                    mailbox_manager,
                    directory_view->subview(boost::bind(&cluster_namespace_interface_t<protocol_t>::extract_master_business_card, _1, peer_id, activity_id)),
                    lock.get_drain_signal()
                    ));
                direct_reader_access.init(new resource_access_t<direct_reader_business_card_t<protocol_t> >(
                    directory_view->subview(boost::bind(&cluster_namespace_interface_t<protocol_t>::extract_direct_reader_business_card_from_primary, _1, peer_id, activity_id))
                    ));
            } else {
                direct_reader_access.init(new resource_access_t<direct_reader_business_card_t<protocol_t> >(
                    directory_view->subview(boost::bind(&cluster_namespace_interface_t<protocol_t>::extract_direct_reader_business_card_from_secondary_up_to_date, _1, peer_id, activity_id))
                    ));
            }

            relationship_t relationship_record;
            relationship_record.is_local = (peer_id == mailbox_manager->get_connectivity_service()->get_me());
            relationship_record.region = region;
            relationship_record.master_access = master_access.get();
            relationship_record.direct_reader_access = direct_reader_access.get();

            region_map_set_membership_t<protocol_t, relationship_t *> relationship_map_insertion(
                &relationships,
                region,
                &relationship_record);

            if (is_start) {
                rassert(start_count > 0);
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

        } catch (resource_lost_exc_t e) {
            /* ignore */
        } catch (interrupted_exc_t e) {
            /* ignore */
        }

        if (is_start) {
            rassert(start_count > 0);
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

    mailbox_manager_t *mailbox_manager;
    clone_ptr_t<watchable_t<std::map<peer_id_t, reactor_business_card_t<protocol_t> > > > directory_view;

    typename protocol_t::temporary_cache_t temporary_cache;

    rng_t distributor_rng;

    std::set<reactor_activity_id_t> handled_activity_ids;
    region_map_t<protocol_t, std::set<relationship_t *> > relationships;

    /* `start_cond` will be pulsed when we have either successfully connected to
    or tried and failed to connect to every peer present when the constructor
    was called. `start_count` is the number of peers we're still waiting for. */
    int start_count;
    cond_t start_cond;

    auto_drainer_t relationship_coroutine_auto_drainer;

    typename watchable_t< std::map<peer_id_t, reactor_business_card_t<protocol_t> > >::subscription_t watcher_subscription;

    DISABLE_COPYING(cluster_namespace_interface_t);
};

#endif /* CLUSTERING_REACTOR_NAMESPACE_INTERFACE_HPP_ */
