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
            &master_connection_t::read_mailbox,
            r, order_token, interruptor);
    }

    typename protocol_t::write_response_t write(typename protocol_t::write_t w, order_token_t order_token, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t) {
        return generic_dispatch<typename protocol_t::write_t, fifo_enforcer_write_token_t, typename protocol_t::write_response_t>(
            &master_connection_t::write_mailbox,
            w, order_token, interruptor);
    }

private:
    class master_connection_t {
    public:
        master_connection_t(peer_id_t p,
                const typename protocol_t::region_t &r,
                const typename master_business_card_t<protocol_t>::read_mailbox_t::address_t &rm,
                const typename master_business_card_t<protocol_t>::write_mailbox_t::address_t &wm) :
            peer_id(p), region(r), read_mailbox(rm), write_mailbox(wm) { }
        peer_id_t peer_id;
        fifo_enforcer_source_t fifo_enforcer_source;
        typename protocol_t::region_t region;
        typename master_business_card_t<protocol_t>::read_mailbox_t::address_t read_mailbox;
        typename master_business_card_t<protocol_t>::write_mailbox_t::address_t write_mailbox;
        auto_drainer_t drainer;
    };

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

    template<class fifo_enforcer_token_type>
    class master_info_t {
    public:
        master_info_t(master_connection_t *m, const fifo_enforcer_token_type &et, auto_drainer_t::lock_t l) :
            master(m), enforcement_token(et), keepalive(l) { }
        master_connection_t *master;
        fifo_enforcer_token_type enforcement_token;
        auto_drainer_t::lock_t keepalive;
    };

    template<class op_type, class fifo_enforcer_token_type, class op_response_type>
    op_response_type generic_dispatch(
            /* This is a pointer-to-member. It's always going to be either
            `&master_connection_t::read_mailbox` or
            `&master_connection_t::write_mailbox`. */
            mailbox_addr_t<void(namespace_interface_id_t, op_type, order_token_t, fifo_enforcer_token_type, mailbox_addr_t<void(boost::variant<op_response_type, std::string>)>)> master_connection_t::*mailbox_field,
            op_type op, order_token_t order_token, signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t)
    {
        if (interruptor->is_pulsed()) throw interrupted_exc_t();

        std::vector<typename protocol_t::region_t> regions;
        std::vector<master_info_t<fifo_enforcer_token_type> > masters_to_contact;
        cover<fifo_enforcer_token_type>(op.get_region(), &regions, &masters_to_contact);
        rassert(regions.size() == masters_to_contact.size());

        std::vector<boost::variant<op_response_type, std::string> > results_or_failures(masters_to_contact.size());
        pmap(masters_to_contact.size(), boost::bind(
            &cluster_namespace_interface_t::generic_perform<op_type, fifo_enforcer_token_type, op_response_type>, this,
            mailbox_field, &regions, &masters_to_contact, &op, order_token, &results_or_failures, _1, interruptor));

        if (interruptor->is_pulsed()) throw interrupted_exc_t();

        std::vector<op_response_type> results;
        for (int i = 0; i < int(masters_to_contact.size()); i++) {
            /* `response_handler_function_t` will throw an exception if there
            was a failure. */
            results.push_back(boost::apply_visitor(response_handler_functor_t<op_response_type>(), results_or_failures[i]));
        }

        return op.unshard(results, &temporary_cache);
    }

    template<class op_type, class fifo_enforcer_token_type, class op_response_type>
    void generic_perform(
            mailbox_addr_t<void(namespace_interface_id_t, op_type, order_token_t, fifo_enforcer_token_type, mailbox_addr_t<void(boost::variant<op_response_type, std::string>)>)> master_connection_t::*mailbox_field,
            const std::vector<typename protocol_t::region_t> *regions,
            const std::vector<master_info_t<fifo_enforcer_token_type> > *masters_to_contact,
            const op_type *operation,
            order_token_t order_token,
            std::vector<boost::variant<op_response_type, std::string> > *results_or_failures,
            int i,
            signal_t *interruptor)
            THROWS_NOTHING
    {
        const typename protocol_t::region_t &region = (*regions)[i];
        const master_info_t<fifo_enforcer_token_type> &master_to_contact = (*masters_to_contact)[i];
        op_type shard = operation->shard(region);
        rassert(region_is_superset(region, shard.get_region()));

        promise_t<boost::variant<op_response_type, std::string> > result_or_failure;
        mailbox_t<void(boost::variant<op_response_type, std::string>)> result_or_failure_mailbox(
            mailbox_manager,
            boost::bind(&promise_t<boost::variant<op_response_type, std::string> >::pulse, &result_or_failure, _1),
            mailbox_callback_mode_inline);

        mailbox_addr_t<void(namespace_interface_id_t, op_type, order_token_t, fifo_enforcer_token_type, mailbox_addr_t<void(boost::variant<op_response_type, std::string>)>)> query_address =
            master_to_contact.master->*mailbox_field;

        fifo_enforcer_token_type enforcement_token = master_to_contact.enforcement_token;

        send(mailbox_manager, query_address,
             self_id, shard, order_token, enforcement_token, result_or_failure_mailbox.get_address());

        try {
            wait_any_t waiter(result_or_failure.get_ready_signal(), master_to_contact.keepalive.get_drain_signal());
            wait_interruptible(&waiter, interruptor);

            if (result_or_failure.get_ready_signal()->is_pulsed()) {
                (*results_or_failures)[i] = result_or_failure.get_value();
            } else {
                (*results_or_failures)[i] = "lost contact with master";
            }

        } catch (interrupted_exc_t) {
            rassert(interruptor->is_pulsed());
            /* Ignore `interrupted_exc_t` and just return immediately.
            `generic_dispatch()` will notice the interruptor has been pulsed and
            won't try to access our result. */
        }
    }

    template<class fifo_enforcer_token_type>
    void cover(
            typename protocol_t::region_t target_region,
            std::vector<typename protocol_t::region_t> *regions_out,
            std::vector<master_info_t<fifo_enforcer_token_type> > *info_out)
            THROWS_ONLY(cannot_perform_query_exc_t)
    {
        ASSERT_NO_CORO_WAITING;
        rassert(regions_out->empty());
        rassert(info_out->empty());

        /* Find all the masters that might possibly be relevant */
        for (typename std::map<master_id_t, master_connection_t *>::const_iterator it = open_connections.begin(); it != open_connections.end(); it++) {
            typename protocol_t::region_t intersection =
                region_intersection(target_region, it->second->region);
            if (!region_is_empty(intersection)) {
                regions_out->push_back(intersection);
                info_out->push_back(master_info_t<fifo_enforcer_token_type>(
                    it->second,
                    get_fifo_enforcer_token<fifo_enforcer_token_type>(&it->second->fifo_enforcer_source),
                    auto_drainer_t::lock_t(&it->second->drainer)
                    ));
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

                    if (is_start) {
                        start_count++;
                    }

                    /* Now handle it. */
                    coro_t::spawn_sometime(boost::bind(
                        &cluster_namespace_interface_t::registrant_coroutine, this,
                        it->first, id, mmit->second, is_start,
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

    void registrant_coroutine(peer_id_t peer_id, master_id_t master_id, master_business_card_t<protocol_t> initial_bcard, bool is_start, auto_drainer_t::lock_t lock) {
        // this function is responsible for removing master_id from handled_master_ids before it returns.

        cond_t ack_signal;
        namespace_interface_business_card_t::ack_mailbox_type ack_mailbox(
            mailbox_manager,
            boost::bind(&cond_t::pulse, &ack_signal),
            mailbox_callback_mode_inline);

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
            master_connection_t connection(
                peer_id,
                initial_bcard.region,
                initial_bcard.read_mailbox,
                initial_bcard.write_mailbox
                );
            map_insertion_sentry_t<master_id_t, master_connection_t *> map_insertion(
                &open_connections, master_id, &connection);

            wait_any_t waiter(failed_signal, drain_signal);
            waiter.wait_lazily_unordered();
        }

        handled_master_ids.erase(master_id);

        // Maybe we got reconnected really quickly, and didn't handle
        // the reconnection, because `handled_master_ids` already noted
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
    std::map<master_id_t, master_connection_t *> open_connections;

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
