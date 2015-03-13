// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_REMOTE_REPLICATOR_SERVER_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_REMOTE_REPLICATOR_SERVER_HPP_

#include "clustering/immediate_consistency/primary_dispatcher.hpp"
#include "clustering/immediate_consistency/remote_replicator_metadata.hpp"

class remote_replicator_server_t {
public:
    remote_replicator_server_t(
        mailbox_manager_t *mailbox_manager,
        primary_dispatcher_t *primary);

    remote_replicator_server_bcard_t get_bcard() {
        return remote_replicator_server_bcard_t { registrar.get_business_card() };
    }

private:
    class proxy_replica_t : public primary_dispatcher_t::dispatchee_t {
    public:
        proxy_replica_t(
            const remote_replicator_client_bcard_t &client_bcard,
            remote_replicator_server_t *parent);

        void do_read(
            const read_t &read,
            state_timestamp_t min_timestamp,
            signal_t *interruptor,
            read_response_t *response_out);

        void do_write_sync(
            const write_t &write,
            state_timestamp_t timestamp,
            order_token_t order_token,
            write_durability_t durability,
            signal_t *interruptor,
            write_response_t *response_out);

        void do_write_async(
            const write_t &write,
            state_timestamp_t timestamp,
            order_token_t order_token,
            signal_t *interruptor);

    private:
        void on_ready(signal_t *interruptor);

        remote_replicator_client_bcard_t client_bcard;
        remote_replicator_server_t *parent;
        bool is_ready;

        remote_replicator_server_intro_t::ready_mailbox_t ready_mailbox;
        scoped_ptr_t<primary_query_router_t::dispatchee_registration_t> registration;
    };

    mailbox_manager_t *mailbox_manager;
    primary_query_router_t *primary;

    registrar_t<
        remote_replicator_client_bcard_t,
        remote_replicator_server_t *,
        proxy_replica_t> registrar;
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_REMOTE_REPLICATOR_SERVER_HPP_ */

