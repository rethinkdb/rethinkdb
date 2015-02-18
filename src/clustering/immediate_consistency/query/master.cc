// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/query/master.hpp"

#include "containers/archive/boost_types.hpp"

/* If the master has received and not yet processed more than
   `MAX_OUTSTANDING_MASTER_REQUESTS` requests, clients will be throttled. This limit
   is enforced in a soft way by a combination of a multi_throttling_server_t on the
   master and a number of multi_throttling_client_t instances on the client nodes. */
const int MAX_OUTSTANDING_MASTER_REQUESTS = 2000;

master_t::master_t(mailbox_manager_t *mm, region_t r, query_callback_t *cb)
    : mailbox_manager(mm),
      query_callback(cb),
      region(r),
      multi_throttling_server(mm, this, MAX_OUTSTANDING_MASTER_REQUESTS) {
}

master_t::~master_t() {
    shutdown_cond.pulse();
}

master_business_card_t master_t::get_business_card() {
    return master_business_card_t(region,
                                  multi_throttling_server.get_business_card());
}

void master_t::client_t::perform_request(
        const master_business_card_t::request_t &request,
        UNUSED signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t)
{
    /* Note that we only pay attention to `parent->shutdown_cond`; we don't ever use
    `interruptor` for anything. This is because as soon as we return, the multi-throttler
    tickets will be returned to the free pool; but if we interrupt
    `query_callback_t::on_*()`, it might not immediately release its resources. So if
    clients repeatedly connected and disconnected, and if we interrupted the call to
    `on_*()` every time, then we would use far more resources than we are supposed to. So
    we wait for requests to finish on their own before returning the tickets to the free
    pool. */

    if (parent->shutdown_cond.is_pulsed()) {
        return;
    }

    try {
        if (const master_business_card_t::read_request_t *read =
                boost::get<master_business_card_t::read_request_t>(&request)) {

            read->order_token.assert_read_mode();
            fifo_enforcer_sink_t::exit_read_t exiter(&fifo_sink, read->fifo_token);
            boost::variant<read_response_t, std::string> reply = read_response_t();
            std::string error;
            bool ok = parent->query_callback->on_read(read->read, &exiter,
                read->order_token, &parent->shutdown_cond,
                boost::get<read_response_t>(&reply), &error);
            if (!ok) {
                reply = error;
            }
            send(parent->mailbox_manager, read->cont_addr, reply);

        } else if (const master_business_card_t::write_request_t *write =
                boost::get<master_business_card_t::write_request_t>(&request)) {

            write->order_token.assert_write_mode();
            fifo_enforcer_sink_t::exit_write_t exiter(&fifo_sink, write->fifo_token);
            boost::variant<write_response_t, std::string> reply = write_response_t();
            std::string error;
            bool ok = parent->query_callback->on_write(write->write, &exiter,
                write->order_token, &parent->shutdown_cond,
                boost::get<write_response_t>(&reply), &error);
            if (!ok) {
                reply = error;
            }
            send(parent->mailbox_manager, write->cont_addr, reply);

        } else {
            unreachable();
        }
    } catch (const interrupted_exc_t &) {
        /* We have to trap the `interrupted_exc_t` because we're listening on
        `parent->shutdown_cond`, not on `interruptor`. It would be illegal for us to
        throw `interrupted_exc_t` when `interruptor` was not pulsed. So instead we just
        return normally. */
    }
}

