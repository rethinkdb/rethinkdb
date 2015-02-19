// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_MASTER_ACCESS_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_MASTER_ACCESS_HPP_

#include "errors.hpp"
#include <boost/optional.hpp>

#include "clustering/generic/multi_throttling_client.hpp"
#include "clustering/generic/registrant.hpp"
#include "clustering/immediate_consistency/query/metadata.hpp"
#include "protocol_api.hpp"

/* `master_access_t` is responsible for sending queries to `master_t`. It is
instantiated by `cluster_namespace_interface_t`. The `master_access_t`
internally contains a `multi_throttling_client_t` that works with the
`multi_throttling_server_t` in the `master_t` to throttle read and write queries
that are being sent to the master. */

class master_access_t : public home_thread_mixin_debug_only_t {
public:
    master_access_t(
            mailbox_manager_t *mm,
            const master_business_card_t &master,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t);

    region_t get_region() {
        return region;
    }

    void new_read_token(fifo_enforcer_sink_t::exit_read_t *out);

    void read(
            const read_t &read,
            read_response_t *response,
            order_token_t otok,
            fifo_enforcer_sink_t::exit_read_t *token,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t);

    void new_write_token(fifo_enforcer_sink_t::exit_write_t *out);

    void write(
            const write_t &write,
            write_response_t *response,
            order_token_t otok,
            fifo_enforcer_sink_t::exit_write_t *token,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t);

private:
    typedef multi_throttling_business_card_t<
            master_business_card_t::request_t,
            master_business_card_t::inner_client_business_card_t
            > mt_business_card_t;

    void on_allocation(int);

    mailbox_manager_t *mailbox_manager;

    region_t region;
    fifo_enforcer_source_t internal_fifo_source;
    fifo_enforcer_sink_t internal_fifo_sink;

    fifo_enforcer_source_t source_for_master;

    multi_throttling_client_t<
            master_business_card_t::request_t,
            master_business_card_t::inner_client_business_card_t
            > multi_throttling_client;

    DISABLE_COPYING(master_access_t);
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_MASTER_ACCESS_HPP_ */
