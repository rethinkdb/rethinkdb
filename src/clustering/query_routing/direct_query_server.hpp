// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_QUERY_ROUTING_DIRECT_QUERY_SERVER_HPP_
#define CLUSTERING_QUERY_ROUTING_DIRECT_QUERY_SERVER_HPP_

#include "clustering/query_routing/metadata.hpp"
#include "concurrency/fifo_checker.hpp"

class store_view_t;

/* For each primary or secondary replica of each shard, there is a
`direct_query_server_t`. The `direct_query_server_t` allows the `table_query_server_t` to
bypass the `broadcaster_t` and read directly from the B-tree itself. This reduces network
traffic and is possible even when the primary replica is unavailable, but the data it
returns might be out of date. */

class direct_query_server_t {
public:
    direct_query_server_t(
            mailbox_manager_t *mm,
            store_view_t *svs);

    direct_query_bcard_t get_bcard();

private:
    void on_read(
            signal_t *interruptor,
            const read_t &,
            const mailbox_addr_t<void(read_response_t)> &);

    mailbox_manager_t *mailbox_manager;
    store_view_t *svs;

    order_source_t order_source;  // TODO: order_token_t::ignore

    direct_query_bcard_t::read_mailbox_t read_mailbox;
};

#endif /* CLUSTERING_QUERY_ROUTING_DIRECT_QUERY_SERVER_HPP_ */

