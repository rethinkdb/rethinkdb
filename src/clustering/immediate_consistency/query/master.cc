#include "clustering/immediate_consistency/query/master.hpp"

#include "containers/archive/boost_types.hpp"

template <class protocol_t>
master_t<protocol_t>::master_t(mailbox_manager_t *mm, ack_checker_t *ac,
                               typename protocol_t::region_t r, broadcaster_t<protocol_t> *b) THROWS_ONLY(interrupted_exc_t)
    : mailbox_manager(mm),
      ack_checker(ac),
      broadcaster(b),
      region(r),
      multi_throttling_server(mm, this, broadcaster_t<protocol_t>::MAX_OUTSTANDING_WRITES) {
    rassert(ack_checker);
}

template <class protocol_t>
master_t<protocol_t>::~master_t() {
    shutdown_cond.pulse();
}

template <class protocol_t>
master_business_card_t<protocol_t> master_t<protocol_t>::get_business_card() {
    return master_business_card_t<protocol_t>(region,
                                              multi_throttling_server.get_business_card());
}

template <class protocol_t>
void master_t<protocol_t>::client_t::perform_request(
        const typename master_business_card_t<protocol_t>::request_t &request,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t)
{
    if (const typename master_business_card_t<protocol_t>::read_request_t *read =
            boost::get<typename master_business_card_t<protocol_t>::read_request_t>(&request)) {

        read->order_token.assert_read_mode();

        boost::variant<typename protocol_t::read_response_t, std::string> reply;
        try {
            reply = typename protocol_t::read_response_t();
            typename protocol_t::read_response_t &resp = boost::get<typename protocol_t::read_response_t>(reply);

            fifo_enforcer_sink_t::exit_read_t exiter(&fifo_sink, read->fifo_token);
            parent->broadcaster->read(read->read, &resp, &exiter, read->order_token, interruptor);
        } catch (cannot_perform_query_exc_t e) {
            reply = e.what();
        }
        send(parent->mailbox_manager, read->cont_addr, reply);

    } else if (const typename master_business_card_t<protocol_t>::write_request_t *write =
            boost::get<typename master_business_card_t<protocol_t>::write_request_t>(&request)) {

        write->order_token.assert_write_mode();

        // TODO: avoid extra response_t copies here
        class write_callback_t : public broadcaster_t<protocol_t>::write_callback_t {
        public:
            explicit write_callback_t(ack_checker_t *ac) : ack_checker(ac) { }
            void on_response(peer_id_t peer, const typename protocol_t::write_response_t &response) {
                if (!response_promise.get_ready_signal()->is_pulsed()) {
                    ack_set.insert(peer);
                    bool is_acceptable;
                    {
                        // TODO: This is horrible.  It was horrible before we had to switch threads, because it is (probably) an unnecessary location of side effects.  Make us not have to switch threads in order to call is_acceptable_ack_set.  (That won't be 100% easy, I don't think.)
                        std::set<peer_id_t> ack_set_copy = ack_set;
                        on_thread_t th(ack_checker->home_thread());
                        is_acceptable = ack_checker->is_acceptable_ack_set(ack_set_copy);
                    }
                    if (is_acceptable) {
                        if (!response_promise.is_pulsed()) {
                            response_promise.pulse(response);
                        }
                    }
                }
            }
            void on_done() {
                done_cond.pulse();
            }
            ack_checker_t *ack_checker;
            std::set<peer_id_t> ack_set;
            promise_t<typename protocol_t::write_response_t> response_promise;
            cond_t done_cond;
        } write_callback(parent->ack_checker);

        /* Avoid a potential race condition where `parent->shutting_down` has
        been pulsed but the `multi_throttling_server_t` hasn't stopped accepting
        requests yet. If we didn't do this, we might let a whole bunch of
        improperly-throttled requests through in a short period of time. */
        if (parent->shutdown_cond.is_pulsed()) {
            return;
        }

        fifo_enforcer_sink_t::exit_write_t exiter(&fifo_sink, write->fifo_token);
        parent->broadcaster->spawn_write(write->write, &exiter, write->order_token, &write_callback, interruptor);

        wait_any_t waiter(&write_callback.done_cond, write_callback.response_promise.get_ready_signal());
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

        if (write_callback.response_promise.get_ready_signal()->is_pulsed()) {
            send(parent->mailbox_manager, write->cont_addr,
                 boost::variant<typename protocol_t::write_response_t, std::string>(write_callback.response_promise.get_value()));
        } else {
            rassert(write_callback.done_cond.is_pulsed());
            send(parent->mailbox_manager, write->cont_addr,
                 boost::variant<typename protocol_t::write_response_t, std::string>("not enough replicas responded"));
        }

        /* When we return, our multi-throttler ticket will be returned to the
        free pool. So don't return until the entry that we made on the
        broadcaster's write queue is gone. */
        wait_interruptible(&write_callback.done_cond, &parent->shutdown_cond);

    } else {
        unreachable();
    }
}


#include "memcached/protocol.hpp"
template class master_t<memcached_protocol_t>;

#include "mock/dummy_protocol.hpp"
template class master_t<mock::dummy_protocol_t>;

#include "rdb_protocol/protocol.hpp"
template class master_t<rdb_protocol_t>;
