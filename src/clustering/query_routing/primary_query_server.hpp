// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_QUERY_ROUTING_PRIMARY_QUERY_SERVER_HPP_
#define CLUSTERING_QUERY_ROUTING_PRIMARY_QUERY_SERVER_HPP_

#include <map>
#include <set>
#include <string>
#include <utility>

#include "clustering/generic/multi_client_server.hpp"
#include "clustering/query_routing/metadata.hpp"

/* Each shard has a `primary_query_server_t` on its primary replica server. The
`primary_query_server_t` is responsible for receiving queries from the servers that the
clients connect to and forwarding those queries to the `query_callback_t`. Specifically,
the class `primary_query_client_t`, which is instantiated by `table_query_client_t`,
sends the queries to the `primary_query_server_t`.

`primary_query_server_t` internally contains a `multi_client_server_t`, which is
responsible for managing clients from the different `primary_query_client_t`s.
We use it in combination with `primary_query_server_t::client_t` to ensure the
ordering of requests that originate from a given client. */

class primary_query_server_t {
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

    primary_query_server_t(
        mailbox_manager_t *mm, region_t r, query_callback_t *callback);
    ~primary_query_server_t();

    primary_query_bcard_t get_bcard();

private:
    class client_t {
    public:
        client_t(primary_query_server_t *p) :
            parent(p) { }
        void perform_request(
                const primary_query_bcard_t::request_t &,
                signal_t *interruptor)
                THROWS_ONLY(interrupted_exc_t);
    private:
        primary_query_server_t *parent;
        fifo_enforcer_sink_t fifo_sink;
    };

    mailbox_manager_t *const mailbox_manager;
    query_callback_t *const query_callback;
    region_t const region;

    /* See note in `client_t::perform_request()` for what this is about */
    cond_t shutdown_cond;

    multi_client_server_t<
            primary_query_bcard_t::request_t,
            primary_query_server_t *,
            client_t
            > multi_client_server;

    DISABLE_COPYING(primary_query_server_t);
};

#endif /* CLUSTERING_QUERY_ROUTING_PRIMARY_QUERY_SERVER_HPP_ */
