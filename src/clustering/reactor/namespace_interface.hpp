#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_NAMESPACE_INTERFACE_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_NAMESPACE_INTERFACE_HPP_

#include <math.h>
#include <algorithm>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/ptr_container/ptr_map.hpp>

#include "arch/timing.hpp"
#include "clustering/immediate_consistency/query/master_access.hpp"
#include "clustering/reactor/metadata.hpp"
#include "clustering/registrant.hpp"
#include "concurrency/fifo_enforcer.hpp"
#include "concurrency/pmap.hpp"
#include "concurrency/promise.hpp"
#include "containers/archive/boost_types.hpp"
#include "protocol_api.hpp"

template<class protocol_t>
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
        return generic_dispatch<typename protocol_t::read_t, fifo_enforcer_sink_t::exit_read_t, typename protocol_t::read_response_t>(
            &master_access_t<protocol_t>::new_read_token, &master_access_t<protocol_t>::read,
            r, order_token, interruptor);
    }

    typename protocol_t::write_response_t write(typename protocol_t::write_t w, order_token_t order_token, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t) {
        return generic_dispatch<typename protocol_t::write_t, fifo_enforcer_sink_t::exit_write_t, typename protocol_t::write_response_t>(
            &master_access_t<protocol_t>::new_write_token, &master_access_t<protocol_t>::write,
            w, order_token, interruptor);
    }

    std::set<typename protocol_t::region_t> get_sharding_scheme() THROWS_ONLY(cannot_perform_query_exc_t) {
        std::vector<typename protocol_t::region_t> s;
        for (typename std::map<reactor_activity_id_t, master_connection_t *>::const_iterator it = open_connections.begin(); it != open_connections.end(); it++) {
            s.push_back(it->second->master_access->get_region());
        }
        typename protocol_t::region_t whole;
        region_join_result_t res = region_join(s, &whole);
        if (res != REGION_JOIN_OK || whole != protocol_t::region_t::universe()) {
            throw cannot_perform_query_exc_t("cannot compute sharding scheme because masters are missing or duplicate");
        }
        return std::set<typename protocol_t::region_t>(s.begin(), s.end());
    }

private:
    class master_connection_t {
    public:
        master_access_t<protocol_t> *master_access;
        auto_drainer_t drainer;
    };

    /* The code for handling reads is 99% the same as the code for handling
    writes, so it's factored out into the `generic_dispatch()` function. */

    template<class fifo_enforcer_token_type>
    class master_info_t {
    public:
        master_access_t<protocol_t> *master_access;
        fifo_enforcer_token_type enforcement_token;
        auto_drainer_t::lock_t keepalive;
    };

    template<class op_type, class fifo_enforcer_token_type, class op_response_type>
    op_response_type generic_dispatch(
            /* `how_to_make_token` and `how_to_run_query` have type pointer-to-
            member-function. */
            void (master_access_t<protocol_t>::*how_to_make_token)(fifo_enforcer_token_type *),
            op_response_type (master_access_t<protocol_t>::*how_to_run_query)(const op_type &, order_token_t, fifo_enforcer_token_type *, signal_t *) THROWS_ONLY(interrupted_exc_t, resource_lost_exc_t, cannot_perform_query_exc_t),
            op_type op, order_token_t order_token, signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t)
    {
        if (interruptor->is_pulsed()) throw interrupted_exc_t();

        std::vector<typename protocol_t::region_t> regions;
        boost::ptr_vector<master_info_t<fifo_enforcer_token_type> > masters_to_contact;
        cover<fifo_enforcer_token_type>(how_to_make_token, op.get_region(), &regions, &masters_to_contact);
        rassert(regions.size() == masters_to_contact.size());

        std::vector<boost::variant<op_response_type, std::string> > results_or_failures(masters_to_contact.size());
        pmap(masters_to_contact.size(), boost::bind(
            &cluster_namespace_interface_t::generic_perform<op_type, fifo_enforcer_token_type, op_response_type>, this,
            how_to_run_query, &regions, &masters_to_contact, &op, order_token, &results_or_failures, _1, interruptor));

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
    void generic_perform(
            op_response_type (master_access_t<protocol_t>::*how_to_run_query)(const op_type &, order_token_t, fifo_enforcer_token_type *, signal_t *) THROWS_ONLY(interrupted_exc_t, resource_lost_exc_t, cannot_perform_query_exc_t),
            const std::vector<typename protocol_t::region_t> *regions,
            boost::ptr_vector<master_info_t<fifo_enforcer_token_type> > *masters_to_contact,
            const op_type *operation,
            order_token_t order_token,
            std::vector<boost::variant<op_response_type, std::string> > *results_or_failures,
            int i,
            signal_t *interruptor)
            THROWS_NOTHING
    {
        const typename protocol_t::region_t &region = (*regions)[i];
        master_info_t<fifo_enforcer_token_type> *master_to_contact = &(*masters_to_contact)[i];
        op_type shard = operation->shard(region);
        rassert(region_is_superset(region, shard.get_region()));

        try {
            (*results_or_failures)[i] = (master_to_contact->master_access->*how_to_run_query)(
                shard,
                order_token,
                &master_to_contact->enforcement_token,
                interruptor);
        } catch (resource_lost_exc_t) {
            (*results_or_failures)[i] = "lost contact with master";
        } catch (cannot_perform_query_exc_t e) {
            (*results_or_failures)[i] = "master error: " + std::string(e.what());
        } catch (interrupted_exc_t) {
            rassert(interruptor->is_pulsed());
            /* Ignore `interrupted_exc_t` and just return immediately.
            `generic_dispatch()` will notice the interruptor has been pulsed and
            won't try to access our result. */
        }
    }

    template<class fifo_enforcer_token_type>
    void cover(
            void (master_access_t<protocol_t>::*how_to_make_token)(fifo_enforcer_token_type *),
            typename protocol_t::region_t target_region,
            std::vector<typename protocol_t::region_t> *regions_out,
            boost::ptr_vector<master_info_t<fifo_enforcer_token_type> > *info_out)
            THROWS_ONLY(cannot_perform_query_exc_t)
    {
        ASSERT_NO_CORO_WAITING;
        rassert(regions_out->empty());
        rassert(info_out->empty());

        /* Find all the masters that might possibly be relevant */
        for (typename std::map<reactor_activity_id_t, master_connection_t *>::const_iterator it = open_connections.begin(); it != open_connections.end(); it++) {
            typename protocol_t::region_t intersection =
                region_intersection(target_region, it->second->master_access->get_region());
            if (!region_is_empty(intersection)) {
                regions_out->push_back(intersection);
                master_info_t<fifo_enforcer_token_type> *mi = new master_info_t<fifo_enforcer_token_type>;
                mi->master_access = it->second->master_access;
                (it->second->master_access->*how_to_make_token)(&mi->enforcement_token);
                mi->keepalive = auto_drainer_t::lock_t(&it->second->drainer);
                info_out->push_back(mi);
            }
        }

        /* Check if they form a proper cover */
        typename protocol_t::region_t join;
        region_join_result_t join_result = region_join(*regions_out, &join);
        switch (join_result) {
        case REGION_JOIN_BAD_REGION:
            throw cannot_perform_query_exc_t("No master available");
        case REGION_JOIN_BAD_JOIN:
            throw cannot_perform_query_exc_t("Multiple masters available");
        case REGION_JOIN_OK:
            break;
        default:
            unreachable();
        }

        if (join != target_region) {
            throw cannot_perform_query_exc_t("No master available");
        }
    }

    void update_registrants(bool is_start) {
        ASSERT_NO_CORO_WAITING;
        std::map<peer_id_t, reactor_business_card_t<protocol_t> > existings = directory_view->get();

        for (typename std::map<peer_id_t, reactor_business_card_t<protocol_t> >::const_iterator it = existings.begin(); it != existings.end(); ++it) {
            for (typename reactor_business_card_t<protocol_t>::activity_map_t::const_iterator amit = it->second.activities.begin(); amit != it->second.activities.end(); ++amit) {
                if (boost::get<reactor_business_card_details::primary_t<protocol_t> >(&amit->second.second)) {
                    reactor_activity_id_t id = amit->first;
                    if (handled_activity_ids.find(id) == handled_activity_ids.end()) {
                        // We have an unhandled activity id.  Say it's handled NOW!  And handle it.
                        handled_activity_ids.insert(id);  // We said it.

                        if (is_start) {
                            start_count++;
                        }

                        /* Now handle it. */
                        coro_t::spawn_sometime(boost::bind(
                            &cluster_namespace_interface_t::registrant_coroutine, this,
                            it->first, id, is_start,
                            auto_drainer_t::lock_t(&registrant_coroutine_auto_drainer)
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

    void registrant_coroutine(peer_id_t peer_id, reactor_activity_id_t activity_id, bool is_start, auto_drainer_t::lock_t lock) THROWS_NOTHING {
        try {
            master_access_t<protocol_t> master_access(
                mailbox_manager,
                directory_view->subview(boost::bind(&cluster_namespace_interface_t<protocol_t>::extract_master_business_card, _1, peer_id, activity_id)),
                lock.get_drain_signal());

            master_connection_t master_connection_record;
            master_connection_record.master_access = &master_access;

            map_insertion_sentry_t<reactor_activity_id_t, master_connection_t *> map_insertion(
                &open_connections, activity_id, &master_connection_record);

            if (is_start) {
                rassert(start_count > 0);
                start_count--;
                if (start_count == 0) {
                    start_cond.pulse();
                }
                is_start = false;
            }

            wait_any_t waiter(master_access.get_failed_signal(), lock.get_drain_signal());
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

    std::set<reactor_activity_id_t> handled_activity_ids;
    std::map<reactor_activity_id_t, master_connection_t *> open_connections;

    /* `start_cond` will be pulsed when we have either successfully connected to
    or tried and failed to connect to every peer present when the constructor
    was called. `start_count` is the number of peers we're still waiting for. */
    int start_count;
    cond_t start_cond;

    auto_drainer_t registrant_coroutine_auto_drainer;

    typename watchable_t< std::map<peer_id_t, reactor_business_card_t<protocol_t> > >::subscription_t watcher_subscription;

    DISABLE_COPYING(cluster_namespace_interface_t);
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_NAMESPACE_INTERFACE_HPP_ */
