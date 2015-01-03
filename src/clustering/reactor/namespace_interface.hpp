// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_REACTOR_NAMESPACE_INTERFACE_HPP_
#define CLUSTERING_REACTOR_NAMESPACE_INTERFACE_HPP_

#include <math.h>

#include <algorithm>
#include <map>
#include <string>
#include <vector>
#include <set>

#include "arch/timing.hpp"
#include "clustering/generic/resource.hpp"
#include "clustering/reactor/metadata.hpp"
#include "clustering/administration/tables/table_metadata.hpp"
#include "containers/clone_ptr.hpp"
#include "containers/cow_ptr.hpp"
#include "concurrency/fifo_enforcer.hpp"
#include "concurrency/pmap.hpp"
#include "concurrency/promise.hpp"
#include "concurrency/watchable.hpp"
#include "protocol_api.hpp"
#include "rdb_protocol/protocol.hpp"

template <class> class cow_ptr_t;
class master_access_t;
template <class> class resource_access_t;
class resource_lost_exc_t;
template <class> class watchable_t;
template <class> class watchable_subscription_t;

class cluster_namespace_interface_t : public namespace_interface_t {
public:
    cluster_namespace_interface_t(
            mailbox_manager_t *mm,
            const std::map<namespace_id_t, std::map<key_range_t, server_id_t> >
                *region_to_primary_maps_,
            watchable_map_t<peer_id_t, namespace_directory_metadata_t> *dv,
            const namespace_id_t &namespace_id_,
            rdb_context_t *);

    /* Returns a signal that will be pulsed when we have either successfully connected
    or tried and failed to connect to every primary replica that was present at the time
    that the constructor was called. This is to avoid the case where we get errors like
    "lost contact with primary replica" when really we just haven't finished connecting
    yet. */
    signal_t *get_initial_ready_signal() {
        return &start_cond;
    }

    bool check_readiness(table_readiness_t readiness, signal_t *interruptor);

    void read(const read_t &r, read_response_t *response, order_token_t order_token, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t);

    void read_outdated(const read_t &r, read_response_t *response, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t);

    void write(const write_t &w, write_response_t *response, order_token_t order_token, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t);

    std::set<region_t> get_sharding_scheme() THROWS_ONLY(cannot_perform_query_exc_t);

private:
    class relationship_t {
    public:
        bool is_local;
        region_t region;
        master_access_t *master_access;
        resource_access_t<direct_reader_business_card_t> *direct_reader_access;
        auto_drainer_t drainer;
    };

    /* The code for handling immediate reads is 99% the same as the code for
    handling writes, so it's factored out into the `dispatch_immediate_op()`
    function. */

    template <class op_type, class fifo_enforcer_token_type>
    class immediate_op_info_t {
    public:
        op_type sharded_op;
        master_access_t *master_access;
        fifo_enforcer_token_type enforcement_token;
        auto_drainer_t::lock_t keepalive;
    };

    class outdated_read_info_t {
    public:
        read_t sharded_op;
        resource_access_t<direct_reader_business_card_t> *direct_reader_access;
        auto_drainer_t::lock_t keepalive;
    };

    template <class op_type, class fifo_enforcer_token_type, class op_response_type>
    void dispatch_immediate_op(
            /* `how_to_make_token` and `how_to_run_query` have type pointer-to-member-function. */
            void (master_access_t::*how_to_make_token)(fifo_enforcer_token_type *),
            void (master_access_t::*how_to_run_query)(const op_type &, op_response_type *response, order_token_t, fifo_enforcer_token_type *, signal_t *) THROWS_ONLY(interrupted_exc_t, resource_lost_exc_t, cannot_perform_query_exc_t),
            const op_type &op,
            op_response_type *response,
            order_token_t order_token,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t);

    // The exception specification of `how_to_run_query` is commented out in
    // order to bypass a clang bug seen on certain versions for OS X, or perhaps
    // fix a confusing violation of C++.  This function is called only by
    // dispatch_immediate_op, which still has the exception specification.
    template <class op_type, class fifo_enforcer_token_type, class op_response_type>
    void perform_immediate_op(
            void (master_access_t::*how_to_run_query)(const op_type &, op_response_type *, order_token_t, fifo_enforcer_token_type *, signal_t *) /* THROWS_ONLY(interrupted_exc_t, resource_lost_exc_t, cannot_perform_query_exc_t) */,
            std::vector<scoped_ptr_t<immediate_op_info_t<op_type, fifo_enforcer_token_type> > > *masters_to_contact,
            std::vector<op_response_type> *results,
            std::vector<std::string> *failures,
            order_token_t order_token,
            int i,
            signal_t *interruptor)
        THROWS_NOTHING;

    void dispatch_outdated_read(
            const read_t &op,
            read_response_t *response,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t);

    void perform_outdated_read(
            std::vector<scoped_ptr_t<outdated_read_info_t> > *direct_readers_to_contact,
            std::vector<read_response_t> *results,
            std::vector<std::string> *failures,
            int i,
            signal_t *interruptor)
        THROWS_NOTHING;

    void update_registrant(const peer_id_t &peer,
                           const namespace_directory_metadata_t *bcard);

    static boost::optional<boost::optional<master_business_card_t> >
    extract_master_business_card(
        const boost::optional<namespace_directory_metadata_t> &bcard,
        const reactor_activity_id_t &activity_id);
    static boost::optional<boost::optional<direct_reader_business_card_t> > 
    extract_direct_reader_business_card_from_primary(
        const boost::optional<namespace_directory_metadata_t> &bcard,
        const reactor_activity_id_t &activity_id);

    static boost::optional<boost::optional<direct_reader_business_card_t> > 
    extract_direct_reader_business_card_from_secondary(
        const boost::optional<namespace_directory_metadata_t> &bcard,
        const reactor_activity_id_t &activity_id);

    void relationship_coroutine(peer_id_t peer_id, reactor_activity_id_t activity_id,
                                bool is_start, bool is_primary, const region_t &region,
                                auto_drainer_t::lock_t lock) THROWS_NOTHING;

    mailbox_manager_t *mailbox_manager;
    const std::map<namespace_id_t, std::map<key_range_t, server_id_t> >
        *region_to_primary_maps;
    watchable_map_t<peer_id_t, namespace_directory_metadata_t> *directory_view;
    namespace_id_t namespace_id;
    rdb_context_t *ctx;

    rng_t distributor_rng;

    std::set<reactor_activity_id_t> handled_activity_ids;
    region_map_t<std::set<relationship_t *> > relationships;

    /* `start_cond` will be pulsed when we have either successfully connected to
    or tried and failed to connect to every peer present when the constructor
    was called. `start_count` is the number of peers we're still waiting for.
    `starting_up` is true during the constructor so `update_registrants()` knows to
    increment `start_count` when it finds a peer. */
    cond_t start_cond;
    int start_count;
    bool starting_up;

    auto_drainer_t relationship_coroutine_auto_drainer;

    watchable_map_t<peer_id_t, namespace_directory_metadata_t>::all_subs_t subs;

    DISABLE_COPYING(cluster_namespace_interface_t);
};

#endif /* CLUSTERING_REACTOR_NAMESPACE_INTERFACE_HPP_ */
