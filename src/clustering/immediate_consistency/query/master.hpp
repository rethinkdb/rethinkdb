// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_MASTER_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_MASTER_HPP_

#include <map>
#include <set>
#include <string>
#include <utility>

#include "clustering/generic/multi_client_server.hpp"
#include "clustering/immediate_consistency/branch/broadcaster.hpp"
#include "clustering/immediate_consistency/query/master_metadata.hpp"

class ack_checker_t;

/* Each shard has a `master_t` on its primary replica server. The `master_t` is
responsible for receiving queries from the servers that the clients connect to
and forwarding those queries to the `broadcaster_t`. Specifically, the class
`master_access_t`, which is instantiated by `cluster_namespace_interface_t`,
sends the queries to the `master_t`.

`master_t` internally contains a `multi_client_server_t`, which is
responsible for managing clients from the different `master_access_t`s.
We use it in combination with `master_t::client_t` to ensure the ordering
of requests that originate from a given client. */

class master_t {
public:
    master_t(mailbox_manager_t *mm, ack_checker_t *ac,
             region_t r,
             broadcaster_t *b) THROWS_ONLY(interrupted_exc_t);

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

    mailbox_manager_t *mailbox_manager;
    ack_checker_t *ack_checker;
    broadcaster_t *broadcaster;
    region_t region;

    /* See note in `client_t::perform_request()` for what this is about */
    cond_t shutdown_cond;

    multi_client_server_t<
            master_business_card_t::request_t,
            master_business_card_t::inner_client_business_card_t,
            master_t *,
            client_t
            > multi_client_server;

    DISABLE_COPYING(master_t);
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_MASTER_HPP_ */
