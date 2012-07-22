#include "clustering/immediate_consistency/query/master.hpp"

#include "containers/archive/boost_types.hpp"

template <class protocol_t>
master_t<protocol_t>::master_t(mailbox_manager_t *mm, ack_checker_t *ac,
                               typename protocol_t::region_t r, broadcaster_t<protocol_t> *b) THROWS_ONLY(interrupted_exc_t)
    : mailbox_manager(mm),
      ack_checker(ac),
      broadcaster(b),
      region(r),
      enqueued_writes(0),
      registrar(mm, this),
      read_mailbox(mailbox_manager, boost::bind(&master_t<protocol_t>::on_read,
                                                this, _1, _2, _3, _4, _5, auto_drainer_t::lock_t(&drainer))),
      write_mailbox(mailbox_manager, boost::bind(&master_t<protocol_t>::on_write,
                                                 this, _1, _2, _3, _4, _5, auto_drainer_t::lock_t(&drainer))) {
    rassert(ack_checker);
}

template <class protocol_t>
master_business_card_t<protocol_t> master_t<protocol_t>::get_business_card() {
    return master_business_card_t<protocol_t>(region,
                                              read_mailbox.get_address(),
                                              write_mailbox.get_address(),
                                              registrar.get_business_card());
}


template <class protocol_t>
void master_t<protocol_t>::on_read(master_access_id_t parser_id, typename protocol_t::read_t read, order_token_t otok, fifo_enforcer_read_token_t token,
                                   mailbox_addr_t<void(boost::variant<typename protocol_t::read_response_t, std::string>)> response_address,
                                   auto_drainer_t::lock_t keepalive) THROWS_NOTHING {
    try {
        keepalive.assert_is_holding(&drainer);
        boost::variant<typename protocol_t::read_response_t, std::string> reply;
        try {
            typename std::map<master_access_id_t, parser_lifetime_t *>::iterator it = sink_map.find(parser_id);
            if (it == sink_map.end()) {
                /* We got a message from a parser that already deregistered
                   itself. This happened because the deregistration message
                   raced ahead of the query. Ignore it. */
                return;
            }

            auto_drainer_t::lock_t peer_lifetime_lock(it->second->drainer());
            fifo_enforcer_sink_t::exit_read_t exiter(it->second->sink(), token);
            reply = broadcaster->read(read, &exiter, otok, peer_lifetime_lock.get_drain_signal());
        } catch (cannot_perform_query_exc_t e) {
            reply = e.what();
        }
        send(mailbox_manager, response_address, reply);
    } catch (interrupted_exc_t) {
        /* Bail out and don't even send a response. The reason an exception
           was thrown is because the `master_t` is being destroyed, and the
           client will find out some other way. */
    }
}

template <class protocol_t>
void master_t<protocol_t>::on_write(master_access_id_t parser_id, typename protocol_t::write_t write, order_token_t otok, fifo_enforcer_write_token_t token,
                                    mailbox_addr_t<void(boost::variant<typename protocol_t::write_response_t, std::string>)> response_address,
                                    auto_drainer_t::lock_t keepalive) THROWS_NOTHING {
    keepalive.assert_is_holding(&drainer);

    typename std::map<master_access_id_t, parser_lifetime_t *>::iterator it = sink_map.find(parser_id);
    if (it == sink_map.end()) {
        /* We got a message from a parser that already deregistered
           itself. This happened because the deregistration message
           raced ahead of the query. Ignore it. */
        return;
    }
    /* TODO: Store `auto_drainer_t::lock_t` along with
       `parser_lifetime_t *` in the map, like we do with `dispatchee_t` in
       the broadcaster */
    auto_drainer_t::lock_t parser_lifetime_lock(it->second->drainer());

    enqueued_writes++;
    it->second->allocated_ops--;

    try {
        class write_callback_t : public broadcaster_t<protocol_t>::write_callback_t {
        public:
            explicit write_callback_t(ack_checker_t *ac) : ack_checker(ac) { }
            void on_response(peer_id_t peer, const typename protocol_t::write_response_t &response) {
                if (!response_promise.get_ready_signal()->is_pulsed()) {
                    ack_set.insert(peer);
                    if (ack_checker->is_acceptable_ack_set(ack_set)) {
                        response_promise.pulse(response);
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
        } write_callback(ack_checker);

        /* It's essential that we use `parser_lifetime_lock.get_drain_signal()`
           rather than `keepalive.get_drain_signal()` because the operations
           from the parser may be arriving out of order, and if we lose contact
           with the parser, we need to be able to interrupt outstanding
           operations rather than blocking until the `master_t` itself is
           destroyed. */
        fifo_enforcer_sink_t::exit_write_t exiter(it->second->sink(), token);
        broadcaster->spawn_write(write, &exiter, otok, &write_callback, parser_lifetime_lock.get_drain_signal());

        wait_any_t waiter(&write_callback.done_cond, write_callback.response_promise.get_ready_signal());
        wait_interruptible(&waiter, parser_lifetime_lock.get_drain_signal());

        if (write_callback.response_promise.get_ready_signal()->is_pulsed()) {
            send(mailbox_manager, response_address,
                 boost::variant<typename protocol_t::write_response_t, std::string>(write_callback.response_promise.get_value())
                 );
        } else {
            rassert(write_callback.done_cond.is_pulsed());
            send(mailbox_manager, response_address,
                 boost::variant<typename protocol_t::write_response_t, std::string>("not enough replicas responded")
                 );
        }

    } catch (interrupted_exc_t) {
        /* ignore */
    }

    enqueued_writes--;
    consider_allocating_more_writes();
}


template <class protocol_t>
void master_t<protocol_t>::consider_allocating_more_writes() {
    if (enqueued_writes + ALLOCATION_CHUNK < TARGET_OPERATION_QUEUE_LENGTH) {
        parser_lifetime_t *most_depleted_parser = NULL;
        for (typename std::map<master_access_id_t, parser_lifetime_t *>::iterator it  = sink_map.begin();
             it != sink_map.end();
             ++it) {
            if (most_depleted_parser == NULL || most_depleted_parser->allocated_ops > it->second->allocated_ops) {
                most_depleted_parser = it->second;
            }
        }
        if (most_depleted_parser && most_depleted_parser->allocated_ops < MAX_ALLOCATION) {
            coro_t::spawn_sometime(boost::bind(&master_t<protocol_t>::allocate_more_writes, this,
                                               most_depleted_parser->allocate_mailbox, auto_drainer_t::lock_t(&drainer)
                                               ));
            most_depleted_parser->allocated_ops += ALLOCATION_CHUNK;
        }
    }
}

template <class protocol_t>
void master_t<protocol_t>::allocate_more_writes(mailbox_addr_t<void(int)> addr, UNUSED auto_drainer_t::lock_t keepalive) {
    send(mailbox_manager, addr, ALLOCATION_CHUNK);
}


#include "memcached/protocol.hpp"
template class master_t<memcached_protocol_t>;

#include "mock/dummy_protocol.hpp"
template class master_t<mock::dummy_protocol_t>;

#include "rdb_protocol/protocol.hpp"
template class master_t<rdb_protocol_t>;
