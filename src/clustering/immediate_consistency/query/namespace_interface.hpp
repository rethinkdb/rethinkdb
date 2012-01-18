#ifndef __CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_NAMESPACE_INTERFACE_HPP__
#define __CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_NAMESPACE_INTERFACE_HPP__

#include "errors.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include "clustering/immediate_consistency/query/metadata.hpp"
#include "concurrency/promise.hpp"
#include "protocol_api.hpp"

template<class protocol_t>
class cluster_namespace_interface_t :
    public namespace_interface_t<protocol_t>
{
public:
    typedef std::map<master_id_t, resource_metadata_t<master_metadata_t<protocol_t> > > master_map_t;

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
            mailbox_cluster_t *c,
            boost::shared_ptr<metadata_read_view_t<namespace_master_metadata_t<protocol_t> > > masters) :
        cluster(c),
        masters_view(metadata_field(&namespace_master_metadata_t<protocol_t>::masters, masters))
        { }

    typename protocol_t::read_response_t read(typename protocol_t::read_t r, order_token_t order_token, signal_t *interruptor) {
        return generic_dispatch<typename protocol_t::read_t, typename protocol_t::read_response_t>(
            &master_metadata_t<protocol_t>::read_mailbox,
            r, order_token, interruptor);
    }

    typename protocol_t::write_response_t write(typename protocol_t::write_t w, order_token_t order_token, signal_t *interruptor) {
        return generic_dispatch<typename protocol_t::write_t, typename protocol_t::write_response_t>(
            &master_metadata_t<protocol_t>::write_mailbox,
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
            `&master_metadata_t<protocol_t>::read_mailbox` or
            `&master_metadata_t<protocol_t>::write_mailbox`. */
            typename async_mailbox_t<void(op_t, order_token_t, typename async_mailbox_t<void(boost::variant<op_response_t, std::string>)>::address_t)>::address_t master_metadata_t<protocol_t>::*mailbox_field,
            op_t op, order_token_t order_token, signal_t *interruptor)
            THROWS_ONLY(ambiguity_exc_t, gap_exc_t, forwarded_exc_t, interrupted_exc_t)
    {
        if (interruptor->is_pulsed()) throw interrupted_exc_t();

        std::vector<typename protocol_t::region_t> regions;
        std::vector<boost::shared_ptr<resource_access_t<master_metadata_t<protocol_t> > > > master_accesses;
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
            typename async_mailbox_t<void(op_t, order_token_t, typename async_mailbox_t<void(boost::variant<op_response_t, std::string>)>::address_t)>::address_t master_metadata_t<protocol_t>::*mailbox_field,
            std::vector<typename protocol_t::region_t> *regions,
            std::vector<boost::shared_ptr<resource_access_t<master_metadata_t<protocol_t> > > > *master_accesses,
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
                cluster, boost::bind(&promise_t<boost::variant<op_response_t, std::string> >::pulse, &result_or_failure, _1));

            typename async_mailbox_t<void(op_t, order_token_t, typename async_mailbox_t<void(boost::variant<op_response_t, std::string>)>::address_t)>::address_t query_address =
                (*master_accesses)[i]->access().*mailbox_field;

            send(cluster, query_address,
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
            std::vector<boost::shared_ptr<resource_access_t<master_metadata_t<protocol_t> > > > *masters_out)
            THROWS_ONLY(ambiguity_exc_t, gap_exc_t)
    {
        rassert(regions_out->empty());
        rassert(masters_out->empty());
        ASSERT_FINITE_CORO_WAITING;

        /* Find all the masters that might possibly be relevant */
        master_map_t masters = masters_view->get();
        for (typename master_map_t::iterator it = masters.begin(); it != masters.end(); it++) {
            boost::shared_ptr<resource_access_t<master_metadata_t<protocol_t> > > access;
            typename protocol_t::region_t region;
            try {
                access = boost::make_shared<resource_access_t<master_metadata_t<protocol_t> > >(
                    cluster,
                    metadata_member((*it).first, masters_view)
                    );
                region = access->access().region;
            } catch (resource_lost_exc_t) {
                /* Ignore masters that have been shut down or are currently
                inaccessible. */
                continue;
            }
            typename protocol_t::region_t intersection = region_intersection(region, target_region);
            if (!region_is_empty(intersection)) {
                regions_out->push_back(intersection);
                masters_out->push_back(access);
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

    mailbox_cluster_t *cluster;
    boost::shared_ptr<metadata_read_view_t<master_map_t> > masters_view;

    typename protocol_t::temporary_cache_t temporary_cache;
};

#endif /* __CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_NAMESPACE_INTERFACE_HPP__ */
