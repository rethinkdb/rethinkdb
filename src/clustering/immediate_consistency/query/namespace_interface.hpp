#ifndef __CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_NAMESPACE_INTERFACE_HPP__
#define __CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_NAMESPACE_INTERFACE_HPP__

#include "errors.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include "concurrency/pmap.hpp"
#include "clustering/immediate_consistency/query/metadata.hpp"
#include "concurrency/promise.hpp"
#include "protocol_api.hpp"

template<class protocol_t>
class cluster_namespace_interface_t :
    public namespace_interface_t<protocol_t>
{
public:
    typedef std::map<master_id_t, master_business_card_t<protocol_t> > master_map_t;

    class ambiguity_exc_t : public std::exception {
        const char *what() const throw () {
            return "There are two or more masters for this region.";
        }
    };

    class gap_exc_t : public std::exception {
        const char *what() const throw () {
            return "There are zero masters for this region.";
        }
    };

    class forwarded_exc_t : public std::exception {
        const char *what() const throw () {
            return "Something went wrong at the master.";
        }
    };

    cluster_namespace_interface_t(
            mailbox_manager_t *mm,
            clone_ptr_t<directory_rview_t<master_map_t> > mv) :
        mailbox_manager(mm),
        masters_view(mv)
        { }

    typename protocol_t::read_response_t read(typename protocol_t::read_t r, order_token_t order_token, signal_t *interruptor) {
        return generic_dispatch<typename protocol_t::read_t, typename protocol_t::read_response_t>(
            &master_business_card_t<protocol_t>::read_mailbox,
            r, order_token, interruptor);
    }

    typename protocol_t::write_response_t write(typename protocol_t::write_t w, order_token_t order_token, signal_t *interruptor) {
        return generic_dispatch<typename protocol_t::write_t, typename protocol_t::write_response_t>(
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
        op_response_t operator()(UNUSED std::string error_message) const {
            /* TODO: Throw a more informative exception. */
            throw forwarded_exc_t();
        }
    };

    template<class op_t, class op_response_t>
    op_response_t generic_dispatch(
            /* This is a pointer-to-member. It's always going to be either
            `&master_business_card_t<protocol_t>::read_mailbox` or
            `&master_business_card_t<protocol_t>::write_mailbox`. */
            typename async_mailbox_t<void(op_t, order_token_t, typename async_mailbox_t<void(boost::variant<op_response_t, std::string>)>::address_t)>::address_t master_business_card_t<protocol_t>::*mailbox_field,
            op_t op, order_token_t order_token, signal_t *interruptor)
            THROWS_ONLY(ambiguity_exc_t, gap_exc_t, forwarded_exc_t, interrupted_exc_t)
    {
        if (interruptor->is_pulsed()) throw interrupted_exc_t();

        std::vector<typename protocol_t::region_t> regions;
        std::vector<boost::shared_ptr<resource_access_t<master_business_card_t<protocol_t> > > > master_accesses;
        cover(op.get_region(), &regions, &master_accesses);
        rassert(regions.size() == master_accesses.size());

        std::vector<boost::variant<op_response_t, std::string> > results_or_failures(regions.size());
        pmap(regions.size(), boost::bind(&cluster_namespace_interface_t::generic_perform<op_t, op_response_t>, this,
            mailbox_field, &regions, &master_accesses, &op, order_token, &results_or_failures, _1, interruptor));

        if (interruptor->is_pulsed()) throw interrupted_exc_t();

        std::vector<op_response_t> results;
        for (int i = 0; i < (int)regions.size(); i++) {
            /* `response_handler_function_t` will throw an exception if there
            was a failure. */
            results.push_back(boost::apply_visitor(response_handler_functor_t<op_response_t>(), results_or_failures[i]));
        }

        return op.unshard(results, &temporary_cache);
    }

    template<class op_t, class op_response_t>
    void generic_perform(
            typename async_mailbox_t<void(op_t, order_token_t, typename async_mailbox_t<void(boost::variant<op_response_t, std::string>)>::address_t)>::address_t master_business_card_t<protocol_t>::*mailbox_field,
            std::vector<typename protocol_t::region_t> *regions,
            std::vector<boost::shared_ptr<resource_access_t<master_business_card_t<protocol_t> > > > *master_accesses,
            op_t *operation,
            order_token_t order_token,
            std::vector<boost::variant<op_response_t, std::string> > *results_or_failures,
            int i,
            signal_t *interruptor)
            THROWS_NOTHING
    {
        op_t shard = operation->shard((*regions)[i]);
        rassert(region_is_superset((*regions)[i], shard.get_region()));

        try {
            promise_t<boost::variant<op_response_t, std::string> > result_or_failure;
            async_mailbox_t<void(boost::variant<op_response_t, std::string>)> result_or_failure_mailbox(
                mailbox_manager, boost::bind(&promise_t<boost::variant<op_response_t, std::string> >::pulse, &result_or_failure, _1));

            master_business_card_t<protocol_t> bcard = (*master_accesses)[i]->access();

            typename async_mailbox_t<void(op_t, order_token_t, typename async_mailbox_t<void(boost::variant<op_response_t, std::string>)>::address_t)>::address_t query_address =
                bcard.*mailbox_field;

            send(mailbox_manager, query_address,
                shard, order_token, result_or_failure_mailbox.get_address());

            wait_any_t waiter(result_or_failure.get_ready_signal(), (*master_accesses)[i]->get_failed_signal());
            wait_interruptible(&waiter, interruptor);

            /* Throw an exception if the master went down */
            (*master_accesses)[i]->access();

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

    void cover(
            typename protocol_t::region_t target_region,
            std::vector<typename protocol_t::region_t> *regions_out,
            std::vector<boost::shared_ptr<resource_access_t<master_business_card_t<protocol_t> > > > *masters_out)
            THROWS_ONLY(ambiguity_exc_t, gap_exc_t)
    {
        rassert(regions_out->empty());
        rassert(masters_out->empty());

        /* Find all the masters that might possibly be relevant */
        {
            connectivity_service_t::peers_list_freeze_t connectivity_freeze(
                masters_view->get_directory_service()->get_connectivity_service());
            ASSERT_FINITE_CORO_WAITING;
            std::set<peer_id_t> peers = masters_view->get_directory_service()->
                get_connectivity_service()->get_peers_list();
            for (std::set<peer_id_t>::iterator it = peers.begin(); it != peers.end(); it++) {
                directory_read_service_t::peer_value_freeze_t value_freeze(
                    masters_view->get_directory_service(), *it);
                master_map_t masters = masters_view->get_value(*it).get();
                for (typename master_map_t::iterator it2 = masters.begin(); it2 != masters.end(); it2++) {
                    typename protocol_t::region_t intersection =
                        region_intersection(target_region, (*it2).second.region);
                    if (!region_is_empty(intersection)) {
                        regions_out->push_back(intersection);
                        masters_out->push_back(boost::make_shared<resource_access_t<master_business_card_t<protocol_t> > >(
                            masters_view
                                ->get_peer_view(*it)
                                ->template subview<boost::optional<master_business_card_t<protocol_t> > >(
                                    optional_member_lens<master_id_t, master_business_card_t<protocol_t> >((*it2).first)
                                    )
                            ));
                    }
                }
            }
        }

        /* Check if they form a proper cover */
        typename protocol_t::region_t join;
        try {
            join = region_join(*regions_out);
        } catch (bad_region_exc_t) {
            throw gap_exc_t();
        } catch (bad_join_exc_t) {
            throw ambiguity_exc_t();
        }
        if (join != target_region) throw gap_exc_t();
    }

    mailbox_manager_t *mailbox_manager;
    clone_ptr_t<directory_rview_t<master_map_t> > masters_view;

    typename protocol_t::temporary_cache_t temporary_cache;
};

#endif /* __CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_NAMESPACE_INTERFACE_HPP__ */
