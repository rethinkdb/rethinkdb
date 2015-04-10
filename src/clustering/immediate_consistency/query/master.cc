// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/query/master.hpp"

#include "containers/archive/boost_types.hpp"

master_t::master_t(mailbox_manager_t *mm, ack_checker_t *ac,
                   region_t r, broadcaster_t *b) THROWS_ONLY(interrupted_exc_t)
    : mailbox_manager(mm),
      ack_checker(ac),
      broadcaster(b),
      region(r),
      multi_client_server(mm, this) {
    guarantee(ack_checker);
}

master_t::~master_t() {
    shutdown_cond.pulse();
}

master_business_card_t master_t::get_business_card() {
    return master_business_card_t(region,
                                  multi_client_server.get_business_card());
}

void master_t::client_t::perform_request(
        const master_business_card_t::request_t &request,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t)
{
    if (const master_business_card_t::read_request_t *read =
            boost::get<master_business_card_t::read_request_t>(&request)) {

        read->order_token.assert_read_mode();

        boost::variant<read_response_t, std::string> reply;
        try {
            reply = read_response_t();
            read_response_t &resp = boost::get<read_response_t>(reply);

            fifo_enforcer_sink_t::exit_read_t exiter(&fifo_sink, read->fifo_token);
            parent->broadcaster->read(read->read, &resp, &exiter, read->order_token, interruptor);
        } catch (const cannot_perform_query_exc_t &e) {
            reply = e.what();
        }
        send(parent->mailbox_manager, read->cont_addr, reply);

    } else if (const master_business_card_t::write_request_t *write =
            boost::get<master_business_card_t::write_request_t>(&request)) {

        write->order_token.assert_write_mode();

        // TODO: avoid extra response_t copies here
        class write_callback_t : public broadcaster_t::write_callback_t {
        public:
            void on_success(const write_response_t &response) {
                success_promise.pulse(response);
            }
            void on_failure(bool might_have_run_anyway) {
                failure_promise.pulse(might_have_run_anyway);
            }
            promise_t<write_response_t> success_promise;
            promise_t<bool> failure_promise;
        } write_callback;

        /* Avoid a potential race condition where `parent->shutting_down` has
        been pulsed but the `multi_client_server_t` hasn't stopped accepting
        requests yet. If we didn't do this, we might let a whole bunch of
        improperly-throttled requests through in a short period of time. */
        if (parent->shutdown_cond.is_pulsed()) {
            return;
        }

        fifo_enforcer_sink_t::exit_write_t exiter(&fifo_sink, write->fifo_token);
        parent->broadcaster->spawn_write(write->write, &exiter, write->order_token, &write_callback, interruptor, parent->ack_checker);

        wait_any_t waiter(write_callback.success_promise.get_ready_signal(),
                          write_callback.failure_promise.get_ready_signal());
        /* Now that we've called `spawn_write()`, we've added another entry to
        the broadcaster's write queue, and that entry will remain there until
        `on_done()` is called on the write callback above. If we were to respect
        `interruptor` here, then when we bailed out our multi-throttler ticket
        would be returned to the free pool. Then if clients repeatedly
        connected, sent a bunch of operations, and then disconnected, then the
        broadcaster's write queue would grow without bound. So instead we use
        our parent's `shutting_down` signal as the interruptor. That way we
        won't bail out unless we're actually shutting down the broadcaster too.
        */
        wait_interruptible(&waiter, &parent->shutdown_cond);

        write_response_t write_response;
        if (write_callback.success_promise.try_get_value(&write_response)) {
            send(parent->mailbox_manager, write->cont_addr,
                 boost::variant<write_response_t, std::string>(write_response));
        } else {
            guarantee(write_callback.failure_promise.get_ready_signal()->is_pulsed());
            if (write_callback.failure_promise.wait()) {
                send(parent->mailbox_manager, write->cont_addr,
                     boost::variant<write_response_t, std::string>(
                         "The number of replicas that responded to the write "
                         "was fewer than the requested number. The write may "
                         "or may not have been performed."));
            } else {
                send(parent->mailbox_manager, write->cont_addr,
                     boost::variant<write_response_t, std::string>(
                         "The number of available replicas is smaller than the "
                         "requested number of replicas to acknowledge a write. "
                         "The write was not performed."));
            }
        }

    } else {
        unreachable();
    }
}
