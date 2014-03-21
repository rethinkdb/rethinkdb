// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/query/master_access.hpp"

#include <math.h>

#include "arch/timing.hpp"
#include "concurrency/promise.hpp"
#include "containers/archive/boost_types.hpp"

// TODO: Was this macro supposed to be used?
// #define THROTTLE_THRESHOLD 200

template <class protocol_t>
master_access_t<protocol_t>::master_access_t(
        mailbox_manager_t *mm,
        const clone_ptr_t<watchable_t<boost::optional<boost::optional<master_business_card_t<protocol_t> > > > > &master,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, resource_lost_exc_t) :
    mailbox_manager(mm),
    multi_throttling_client(
        mailbox_manager,
        master->subview(&master_access_t<protocol_t>::extract_multi_throttling_business_card),
        typename master_business_card_t<protocol_t>::inner_client_business_card_t(),
        interruptor)
{
    boost::optional<boost::optional<master_business_card_t<protocol_t> > > business_card = master->get();
    if (!business_card || !business_card.get()) {
        throw resource_lost_exc_t();
    }

    region = business_card.get().get().region;
}

template <class protocol_t>
void master_access_t<protocol_t>::new_read_token(fifo_enforcer_sink_t::exit_read_t *out) {
    out->begin(&internal_fifo_sink, internal_fifo_source.enter_read());
}

template <class protocol_t>
void master_access_t<protocol_t>::read(
        const typename protocol_t::read_t &read,
        typename protocol_t::read_response_t *response,
        order_token_t otok,
        fifo_enforcer_sink_t::exit_read_t *token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t,
                    resource_lost_exc_t,
                    cannot_perform_query_exc_t) {
    rassert(region_is_superset(region, read.get_region()));

    promise_t<boost::variant<typename protocol_t::read_response_t, std::string> >
        result_or_failure;
    mailbox_t<void(boost::variant<typename protocol_t::read_response_t, std::string>)>
        result_or_failure_mailbox(
            mailbox_manager,
            boost::bind(&promise_t<boost::variant<typename protocol_t::read_response_t,
                                                  std::string> >::pulse,
                        &result_or_failure, _1));

    wait_interruptible(token, interruptor);
    fifo_enforcer_read_token_t token_for_master = source_for_master.enter_read();
    typename multi_throttling_client_t<
            typename master_business_card_t<protocol_t>::request_t,
            typename master_business_card_t<protocol_t>::inner_client_business_card_t
            >::ticket_acq_t ticket(&multi_throttling_client);
    token->end();

    typename master_business_card_t<protocol_t>::read_request_t read_request(
        read,
        otok,
        token_for_master,
        result_or_failure_mailbox.get_address());

    multi_throttling_client.spawn_request(read_request, &ticket, interruptor);

    wait_any_t waiter(result_or_failure.get_ready_signal(), get_failed_signal());
    wait_interruptible(&waiter, interruptor);

    if (result_or_failure.is_pulsed()) {
        if (const std::string *error
            = boost::get<std::string>(&result_or_failure.wait())) {
            throw cannot_perform_query_exc_t(*error);
        } else if (const typename protocol_t::read_response_t *result =
                boost::get<typename protocol_t::read_response_t>(
                    &result_or_failure.wait())) {
            *response = *result;
        } else {
            unreachable();
        }
    } else {
        throw resource_lost_exc_t();
    }
}

template <class protocol_t>
void master_access_t<protocol_t>::new_write_token(fifo_enforcer_sink_t::exit_write_t *out) {
    out->begin(&internal_fifo_sink, internal_fifo_source.enter_write());
}

template <class protocol_t>
void master_access_t<protocol_t>::write(
        const typename protocol_t::write_t &write,
        typename protocol_t::write_response_t *response,
        order_token_t otok,
        fifo_enforcer_sink_t::exit_write_t *token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, resource_lost_exc_t, cannot_perform_query_exc_t) {
    rassert(region_is_superset(region, write.get_region()));

    promise_t<boost::variant<typename protocol_t::write_response_t, std::string> > result_or_failure;
    mailbox_t<void(boost::variant<typename protocol_t::write_response_t, std::string>)> result_or_failure_mailbox(
        mailbox_manager,
        boost::bind(&promise_t<boost::variant<typename protocol_t::write_response_t, std::string> >::pulse, &result_or_failure, _1));

    wait_interruptible(token, interruptor);
    fifo_enforcer_write_token_t token_for_master = source_for_master.enter_write();
    typename multi_throttling_client_t<
            typename master_business_card_t<protocol_t>::request_t,
            typename master_business_card_t<protocol_t>::inner_client_business_card_t
            >::ticket_acq_t ticket(&multi_throttling_client);
    token->end();

    typename master_business_card_t<protocol_t>::write_request_t write_request(
        write,
        otok,
        token_for_master,
        result_or_failure_mailbox.get_address());

    multi_throttling_client.spawn_request(write_request, &ticket, interruptor);

    wait_any_t waiter(result_or_failure.get_ready_signal(), get_failed_signal());
    wait_interruptible(&waiter, interruptor);

    if (result_or_failure.get_ready_signal()->is_pulsed()) {
        if (const std::string *error = boost::get<std::string>(&result_or_failure.wait())) {
            throw cannot_perform_query_exc_t(*error);
        } else if (const typename protocol_t::write_response_t *result =
                boost::get<typename protocol_t::write_response_t>(&result_or_failure.wait())) {
            *response = *result;
        } else {
            unreachable();
        }
    } else {
        throw resource_lost_exc_t();
    }
}


#include "rdb_protocol/protocol.hpp"
#include "memcached/protocol.hpp"
#include "mock/dummy_protocol.hpp"

template class master_access_t<rdb_protocol_t>;
template class master_access_t<memcached_protocol_t>;
template class master_access_t<mock::dummy_protocol_t>;
