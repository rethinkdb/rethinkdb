#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_NAMESPACE_INTERFACE_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_NAMESPACE_INTERFACE_HPP_

#include "errors.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/ptr_container/ptr_map.hpp>

#include "concurrency/fifo_enforcer.hpp"
#include "concurrency/pmap.hpp"
#include "clustering/immediate_consistency/query/metadata.hpp"
#include "clustering/registrant.hpp"
#include "concurrency/promise.hpp"
#include "containers/archive/boost_types.hpp"
#include "containers/archive/order_token.hpp"
#include "protocol_api.hpp"


template <class fifo_enforcer_token_type> fifo_enforcer_token_type get_fifo_enforcer_token(fifo_enforcer_source_t *source);

template <>
inline
fifo_enforcer_read_token_t get_fifo_enforcer_token<fifo_enforcer_read_token_t>(fifo_enforcer_source_t *source) {
    return source->enter_read();
}

template <>
inline
fifo_enforcer_write_token_t get_fifo_enforcer_token<fifo_enforcer_write_token_t>(fifo_enforcer_source_t *source) {
    return source->enter_write();
}


template<class protocol_t>
class cluster_namespace_interface_t :
    public namespace_interface_t<protocol_t>
{
public:
    typedef std::map<master_id_t, master_business_card_t<protocol_t> > master_map_t;

    cluster_namespace_interface_t(
            mailbox_manager_t *mm,
            clone_ptr_t<watchable_t<std::map<peer_id_t, master_map_t> > > mv) :
        self_id(generate_uuid()),
        mailbox_manager(mm),
        masters_view(mv),
        start_count(0),
        watcher_subscription(boost::bind(&cluster_namespace_interface_t::update_registrants, this, false))
    {
        {
            typename watchable_t< std::map<peer_id_t, master_map_t> >::freeze_t freeze(masters_view);
            update_registrants(true);
            watcher_subscription.reset(masters_view, &freeze);
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
        return generic_dispatch<typename protocol_t::read_t, fifo_enforcer_read_token_t, typename protocol_t::read_response_t>(
            &master_business_card_t<protocol_t>::read_mailbox,
            r, order_token, interruptor);
    }

    typename protocol_t::write_response_t write(typename protocol_t::write_t w, order_token_t order_token, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t) {
        return generic_dispatch<typename protocol_t::write_t, fifo_enforcer_write_token_t, typename protocol_t::write_response_t>(
            &master_business_card_t<protocol_t>::write_mailbox,
            w, order_token, interruptor);
    }

private:
    /* The code for handling reads is 99% the same as the code for handling
    writes, so it's factored out into the `generic_dispatch()` function. */

    template<class op_response_t>
    class response_handler_functor_t : public boost::static_visitor<op_response_t> {
    public:
        op_response_t operator()(op_response_t response) const {
            return response;
        }
        op_response_t operator()(UNUSED const std::string& error_message) const {
            throw cannot_perform_query_exc_t(error_message);
        }
    };

    // Used to overcome boost::bind parameter limitations.
    template<class fifo_enforcer_token_type>
    struct master_info_t {
        std::vector<typename protocol_t::region_t> regions;
        std::vector<boost::shared_ptr<resource_access_t<master_business_card_t<protocol_t> > > > master_accesses;
        std::vector<master_id_t> master_ids;
        std::vector<fifo_enforcer_token_type> enforcement_tokens;
        std::vector<bool> enforcement_token_found;  // a ghetto hack, i know.  send your prayers to valgrind.
    };


    template<class op_type, class fifo_enforcer_token_type, class op_response_type>
    op_response_type generic_dispatch(
            /* This is a pointer-to-member. It's always going to be either
            `&master_business_card_t<protocol_t>::read_mailbox` or
            `&master_business_card_t<protocol_t>::write_mailbox`. */
            mailbox_addr_t<void(namespace_interface_id_t, op_type, order_token_t, fifo_enforcer_token_type, mailbox_addr_t<void(boost::variant<op_response_type, std::string>)>)> master_business_card_t<protocol_t>::*mailbox_field,
            op_type op, order_token_t order_token, signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t)
    {
        if (interruptor->is_pulsed()) throw interrupted_exc_t();

        master_info_t<fifo_enforcer_token_type> info;
        cover(op.get_region(),
              &info.regions,
              &info.master_accesses,
              &info.master_ids);
        int regions_size = info.regions.size();
        rassert(info.regions.size() == info.master_accesses.size());

        // Create fifo enforcement tokens to be sent over the wire.
        for (int i = 0, e = info.master_ids.size(); i < e; ++i) {
            boost::ptr_map<master_id_t, fifo_enforcer_source_t>::iterator enf_it = fifo_enforcers.find(info.master_ids[i]);
            if (enf_it == fifo_enforcers.end()) {
                info.enforcement_tokens.push_back(fifo_enforcer_token_type());
                info.enforcement_token_found.push_back(false);
            } else {
                fifo_enforcer_source_t *fifo_source = enf_it->second;
                info.enforcement_tokens.push_back(get_fifo_enforcer_token<fifo_enforcer_token_type>(fifo_source));
                info.enforcement_token_found.push_back(true);
            }
        }

        std::vector<boost::variant<op_response_type, std::string> > results_or_failures(regions_size);
        pmap(regions_size, boost::bind(&cluster_namespace_interface_t::generic_perform<op_type, fifo_enforcer_token_type, op_response_type>, this,
                                       mailbox_field, &info, &op, order_token, &results_or_failures, _1, interruptor));

        if (interruptor->is_pulsed()) throw interrupted_exc_t();

        std::vector<op_response_type> results;
        for (int i = 0; i < regions_size; i++) {
            /* `response_handler_function_t` will throw an exception if there
            was a failure. */
            results.push_back(boost::apply_visitor(response_handler_functor_t<op_response_type>(), results_or_failures[i]));
        }

        return op.unshard(results, &temporary_cache);
    }

    template<class op_type, class fifo_enforcer_token_type, class op_response_type>
    void generic_perform(
            mailbox_addr_t<void(namespace_interface_id_t, op_type, order_token_t, fifo_enforcer_token_type, mailbox_addr_t<void(boost::variant<op_response_type, std::string>)>)> master_business_card_t<protocol_t>::*mailbox_field,
            const master_info_t<fifo_enforcer_token_type> *info,
            const op_type *operation,
            order_token_t order_token,
            std::vector<boost::variant<op_response_type, std::string> > *results_or_failures,
            int i,
            signal_t *interruptor)
            THROWS_NOTHING
    {
        op_type shard = operation->shard(info->regions[i]);
        rassert(region_is_superset(info->regions[i], shard.get_region()));

        try {
            promise_t<boost::variant<op_response_type, std::string> > result_or_failure;
            mailbox_t<void(boost::variant<op_response_type, std::string>)> result_or_failure_mailbox(
                mailbox_manager, boost::bind(&promise_t<boost::variant<op_response_type, std::string> >::pulse, &result_or_failure, _1));

            master_business_card_t<protocol_t> bcard = info->master_accesses[i]->access();

            mailbox_addr_t<void(namespace_interface_id_t, op_type, order_token_t, fifo_enforcer_token_type, mailbox_addr_t<void(boost::variant<op_response_type, std::string>)>)> query_address =
                bcard.*mailbox_field;

            if (!info->enforcement_token_found[i]) {
                throw resource_lost_exc_t();
            }

            fifo_enforcer_token_type enforcement_token = info->enforcement_tokens[i];

            send(mailbox_manager, query_address,
                 self_id, shard, order_token, enforcement_token, result_or_failure_mailbox.get_address());

            wait_any_t waiter(result_or_failure.get_ready_signal(), info->master_accesses[i]->get_failed_signal());
            wait_interruptible(&waiter, interruptor);

            /* Throw an exception if the master went down */
            info->master_accesses[i]->access();

            (*results_or_failures)[i] = result_or_failure.get_value();

        } catch (resource_lost_exc_t) {
            (*results_or_failures)[i] = "lost contact with master";

        } catch (interrupted_exc_t) {
            rassert(interruptor->is_pulsed());
            /* Ignore `interrupted_exc_t` and just return immediately.
            `generic_dispatch()` will notice the interruptor has been pulsed and
            won't try to access our result. */
        }
    }

    static boost::optional<boost::optional<master_business_card_t<protocol_t> > > extract_master_business_card(const std::map<peer_id_t, master_map_t> &map, const peer_id_t &peer, const master_id_t &master) {
        boost::optional<boost::optional<master_business_card_t<protocol_t> > > ret;
        typename std::map<peer_id_t, master_map_t>::const_iterator it = map.find(peer);
        if (it != map.end()) {
            ret = boost::optional<master_business_card_t<protocol_t> >();
            typename master_map_t::const_iterator jt = it->second.find(master);
            if (jt != it->second.end()) {
                ret.get() = jt->second;
            }
        }
        return ret;
    }

    void cover(
            typename protocol_t::region_t target_region,
            std::vector<typename protocol_t::region_t> *regions_out,
            std::vector<boost::shared_ptr<resource_access_t<master_business_card_t<protocol_t> > > > *masters_out,
            std::vector<master_id_t> *master_ids_out)
            THROWS_ONLY(cannot_perform_query_exc_t)
    {
        rassert(regions_out->empty());
        rassert(masters_out->empty());

        /* Find all the masters that might possibly be relevant */
        {
            typename watchable_t< std::map<peer_id_t, master_map_t> >::freeze_t freeze(masters_view);
            std::map<peer_id_t, master_map_t> snapshot = masters_view->get();
            for (typename std::map<peer_id_t, master_map_t>::iterator it = snapshot.begin(); it != snapshot.end(); it++) {
                for (typename master_map_t::iterator it2 = it->second.begin(); it2 != it->second.end(); it2++) {
                    typename protocol_t::region_t intersection =
                        region_intersection(target_region, it2->second.region);
                    if (!region_is_empty(intersection)) {
                        regions_out->push_back(intersection);
                        masters_out->push_back(boost::make_shared<resource_access_t<master_business_card_t<protocol_t> > >(
                            masters_view->subview(boost::bind(&cluster_namespace_interface_t<protocol_t>::extract_master_business_card, _1, it->first, it2->first))
                            ));
                        master_ids_out->push_back(it2->first);
                    }
                }
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
        std::map<peer_id_t, master_map_t> existings = masters_view->get();

        for (typename std::map<peer_id_t, master_map_t>::const_iterator it = existings.begin(); it != existings.end(); ++it) {
            for (typename master_map_t::const_iterator mmit = it->second.begin(); mmit != it->second.end(); ++mmit) {
                master_id_t id = mmit->first;
                if (handled_master_ids.find(id) == handled_master_ids.end()) {
                    // We have an unhandled master id.  Say it's handled NOW!  And handle it.
                    handled_master_ids.insert(id);  // We said it.

                    start_count++;

                    /* We handled it. */
                    coro_t::spawn_sometime(boost::bind(
                        &cluster_namespace_interface_t::registrant_coroutine, this,
                        it->first, id, is_start,
                        auto_drainer_t::lock_t(&registrant_coroutine_auto_drainer)
                        ));
                }
            }
        }
    }

    static boost::optional<boost::optional<registrar_business_card_t<namespace_interface_business_card_t> > > extract_registrar_business_card(const std::map<peer_id_t, master_map_t> &map, const peer_id_t &peer, const master_id_t &master) {
        boost::optional<boost::optional<registrar_business_card_t<namespace_interface_business_card_t> > > ret;
        typename std::map<peer_id_t, master_map_t>::const_iterator it = map.find(peer);
        if (it != map.end()) {
            ret = boost::optional<registrar_business_card_t<namespace_interface_business_card_t> >();
            typename master_map_t::const_iterator jt = it->second.find(master);
            if (jt != it->second.end()) {
                ret.get() = jt->second.namespace_interface_registration_business_card;
            }
        }
        return ret;
    }

    void registrant_coroutine(peer_id_t peer_id, master_id_t master_id, bool is_start, auto_drainer_t::lock_t lock) {
        // this function is responsible for removing master_id from handled_master_ids before it returns.

        cond_t ack_signal;
        namespace_interface_business_card_t::ack_mailbox_type ack_mailbox(mailbox_manager, boost::bind(&cond_t::pulse, &ack_signal));

        registrant_t<namespace_interface_business_card_t>
            registrant(mailbox_manager,
                       masters_view->subview(boost::bind(&cluster_namespace_interface_t<protocol_t>::extract_registrar_business_card, _1, peer_id, master_id)),
                       namespace_interface_business_card_t(self_id, ack_mailbox.get_address()));

        signal_t *drain_signal = lock.get_drain_signal();
        signal_t *failed_signal = registrant.get_failed_signal();

        {
            wait_any_t ack_waiter(&ack_signal, drain_signal, failed_signal);
            ack_waiter.wait_lazily_unordered();
        }

        if (is_start) {
            rassert(start_count > 0);
            start_count--;
            if (start_count == 0) {
                start_cond.pulse();
            }
        }

        if (ack_signal.is_pulsed()) {
            fifo_enforcers.insert(master_id, new fifo_enforcer_source_t);

            {
                wait_any_t waiter(failed_signal, drain_signal);
                waiter.wait_lazily_unordered();
            }

            fifo_enforcers.erase(master_id);
        }

        handled_master_ids.erase(master_id);

        // Maybe we got reconnected really quickly, and didn't handle
        // the reconnection, because handled_master_ids already noted
        // ourselves as handled.
        if (!drain_signal->is_pulsed()) {
            update_registrants(false);
        }
    }

    namespace_interface_id_t self_id;
    mailbox_manager_t *mailbox_manager;
    clone_ptr_t<watchable_t<std::map<peer_id_t, master_map_t> > > masters_view;

    typename protocol_t::temporary_cache_t temporary_cache;

    std::set<master_id_t> handled_master_ids;
    boost::ptr_map<master_id_t, fifo_enforcer_source_t> fifo_enforcers;

    /* `start_cond` will be pulsed when we have either successfully connected to
    or tried and failed to connect to every peer present when the constructor
    was called. `start_count` is the number of peers we're still waiting for. */
    int start_count;
    cond_t start_cond;

    auto_drainer_t registrant_coroutine_auto_drainer;

    typename watchable_t< std::map<peer_id_t, master_map_t> >::subscription_t watcher_subscription;

    DISABLE_COPYING(cluster_namespace_interface_t);
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_NAMESPACE_INTERFACE_HPP_ */
