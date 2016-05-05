// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_QUERY_ROUTING_TABLE_QUERY_CLIENT_HPP_
#define CLUSTERING_QUERY_ROUTING_TABLE_QUERY_CLIENT_HPP_

#include <math.h>

#include <map>
#include <string>
#include <vector>
#include <set>

#include "clustering/administration/auth/permission_error.hpp"
#include "clustering/query_routing/metadata.hpp"
#include "containers/clone_ptr.hpp"
#include "concurrency/fifo_enforcer.hpp"
#include "concurrency/watchable_map.hpp"
#include "protocol_api.hpp"
#include "rdb_protocol/protocol.hpp"

class multi_table_manager_t;
class primary_query_client_t;
class table_meta_client_t;

/* `table_query_client_t` is responsible for sending queries to the cluster. It
instantiates `primary_query_client_t` and `direct_query_client_t` internally; it covers
the entire table whereas they cover single shards. */

class table_query_client_t : public namespace_interface_t {
public:
    table_query_client_t(
            const namespace_id_t &table_id,
            mailbox_manager_t *mm,
            watchable_map_t<std::pair<peer_id_t, uuid_u>, table_query_bcard_t>
                *directory,
            multi_table_manager_t *multi_table_manager,
            rdb_context_t *,
            table_meta_client_t *table_meta_client);

    /* Returns a signal that will be pulsed when we have either successfully connected
    or tried and failed to connect to every primary replica that was present at the time
    that the constructor was called. This is to avoid the case where we get errors like
    "lost contact with primary replica" when really we just haven't finished connecting
    yet. */
    signal_t *get_initial_ready_signal() {
        return &start_cond;
    }

    bool check_readiness(table_readiness_t readiness, signal_t *interruptor);

    void read(
            auth::user_context_t const &user_context,
            const read_t &r,
            read_response_t *response,
            order_token_t order_token,
            signal_t *interruptor)
            THROWS_ONLY(
                interrupted_exc_t, cannot_perform_query_exc_t, auth::permission_error_t);

    void write(
            auth::user_context_t const &user_context,
            const write_t &w,
            write_response_t *response,
            order_token_t order_token,
            signal_t *interruptor)
            THROWS_ONLY(
                interrupted_exc_t, cannot_perform_query_exc_t, auth::permission_error_t);

    std::set<region_t> get_sharding_scheme() THROWS_ONLY(cannot_perform_query_exc_t);

private:
    class relationship_t {
    public:
        bool is_local;
        region_t region;
        primary_query_client_t *primary_client;
        const direct_query_bcard_t *direct_bcard;
        auto_drainer_t drainer;
    };

    /* The code for handling immediate reads is 99% the same as the code for
    handling writes, so it's factored out into the `dispatch_immediate_op()`
    function. */

    template <class op_type, class fifo_enforcer_token_type>
    class immediate_op_info_t {
    public:
        op_type sharded_op;
        primary_query_client_t *primary_client;
        fifo_enforcer_token_type enforcement_token;
        auto_drainer_t::lock_t keepalive;
    };

    class outdated_read_info_t {
    public:
        read_t sharded_op;
        const direct_query_bcard_t *direct_bcard;
        auto_drainer_t::lock_t keepalive;
    };

    template <class op_type, class fifo_enforcer_token_type, class op_response_type>
    void dispatch_immediate_op(
            /* `how_to_make_token` and `how_to_run_query` have type pointer-to-member-function. */
            void (primary_query_client_t::*how_to_make_token)(fifo_enforcer_token_type *),
            void (primary_query_client_t::*how_to_run_query)(const op_type &, op_response_type *response, order_token_t, fifo_enforcer_token_type *, signal_t *) THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t),
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
            void (primary_query_client_t::*how_to_run_query)(const op_type &, op_response_type *, order_token_t, fifo_enforcer_token_type *, signal_t *) /* THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t) */,
            std::vector<scoped_ptr_t<immediate_op_info_t<op_type, fifo_enforcer_token_type> > > *masters_to_contact,
            std::vector<op_response_type> *results,
            std::vector<boost::optional<cannot_perform_query_exc_t> > *failures,
            order_token_t order_token,
            size_t i,
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
            size_t i,
            signal_t *interruptor)
        THROWS_NOTHING;

    void dispatch_debug_direct_read(
            const read_t &op,
            read_response_t *response,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t);

    void update_registrant(const std::pair<peer_id_t, uuid_u> &key,
                           const table_query_bcard_t *bcard);

    void relationship_coroutine(
        const std::pair<peer_id_t, uuid_u> &key,
        const table_query_bcard_t &bcard,
        bool is_start,
        auto_drainer_t::lock_t lock) THROWS_NOTHING;

    namespace_id_t const table_id;
    mailbox_manager_t *const mailbox_manager;
    watchable_map_t<std::pair<peer_id_t, uuid_u>, table_query_bcard_t>
        *const directory;
    multi_table_manager_t *const multi_table_manager;
    rdb_context_t *const ctx;
    table_meta_client_t *m_table_meta_client;

    std::map<std::pair<peer_id_t, uuid_u>, scoped_ptr_t<cond_t> > coro_stoppers;
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

    watchable_map_t<std::pair<peer_id_t, uuid_u>, table_query_bcard_t>::all_subs_t subs;

    DISABLE_COPYING(table_query_client_t);
};

#endif /* CLUSTERING_QUERY_ROUTING_TABLE_QUERY_CLIENT_HPP_ */
