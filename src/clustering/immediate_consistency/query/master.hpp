// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_MASTER_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_MASTER_HPP_

#include <map>
#include <set>
#include <string>
#include <utility>

#include "clustering/generic/multi_throttling_server.hpp"
#include "clustering/immediate_consistency/query/master_metadata.hpp"

/* Each shard has a `master_t` on its primary replica server. The `master_t` is
responsible for receiving queries from the servers that the clients connect to
and forwarding those queries to the `query_clalback_t`. Specifically, the class
`master_access_t`, which is instantiated by `cluster_namespace_interface_t`,
sends the queries to the `master_t`.

`master_t` internally contains a `multi_throttling_server_t`, which is
responsible for throttling queries from the different `master_access_t`s. */

class master_t {
public:
    class query_callback_t {
    public:
        virtual bool on_write(
            const write_t &request,
            fifo_enforcer_sink_t::exit_write_t *exiter,
            order_token_t order_token,
            signal_t *interruptor,
            write_response_t *response_out,
            std::string *error_out) = 0;
        virtual bool on_read(
            const read_t &request,
            fifo_enforcer_sink_t::exit_read_t *exiter,
            order_token_t order_token,
            signal_t *interruptor,
            read_response_t *response_out,
            std::string *error_out) = 0;
    protected:
        virtual ~query_callback_t() { }
    };

    master_t(mailbox_manager_t *mm, region_t r, query_callback_t *callback);
    ~master_t();

    master_business_card_t get_business_card();

private:
    class client_t {
    public:
        client_t(
                master_t *p,
                UNUSED const master_business_card_t::inner_client_business_card_t &) :
            parent(p) { }
        void perform_request(
                const master_business_card_t::request_t &,
                signal_t *interruptor)
                THROWS_ONLY(interrupted_exc_t);
    private:
        master_t *parent;
        fifo_enforcer_sink_t fifo_sink;
    };

    mailbox_manager_t *const mailbox_manager;
    query_callback_t *const query_callback;
    region_t const region;

    /* See note in `client_t::perform_request()` for what this is about */
    cond_t shutdown_cond;

    multi_throttling_server_t<
            master_business_card_t::request_t,
            master_business_card_t::inner_client_business_card_t,
            master_t *,
            client_t
            > multi_throttling_server;

    DISABLE_COPYING(master_t);
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_MASTER_HPP_ */
