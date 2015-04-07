// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/query/master_access.hpp"

#include <math.h>

#include <functional>

#include "arch/timing.hpp"
#include "concurrency/promise.hpp"
#include "containers/archive/boost_types.hpp"
#include "rdb_protocol/protocol.hpp"

master_access_t::master_access_t(
        mailbox_manager_t *mm,
        const clone_ptr_t<watchable_t<boost::optional<boost::optional<master_business_card_t> > > > &master,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, resource_lost_exc_t) :
    mailbox_manager(mm),
    multi_client_client(
        mailbox_manager,
        master->subview(&master_access_t::extract_multi_client_business_card),
        master_business_card_t::inner_client_business_card_t(),
        interruptor)
{
    boost::optional<boost::optional<master_business_card_t> > business_card = master->get();
    if (!business_card || !business_card.get()) {
        throw resource_lost_exc_t();
    }

    region = business_card.get().get().region;
}

void master_access_t::new_read_token(fifo_enforcer_sink_t::exit_read_t *out) {
    out->begin(&internal_fifo_sink, internal_fifo_source.enter_read());
}

void master_access_t::read(
        const read_t &read,
        read_response_t *response,
        order_token_t otok,
        fifo_enforcer_sink_t::exit_read_t *token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t,
                    resource_lost_exc_t,
                    cannot_perform_query_exc_t) {
    rassert(region_is_superset(region, read.get_region()));

    promise_t<boost::variant<read_response_t, std::string> >
        result_or_failure;
    mailbox_t<void(boost::variant<read_response_t, std::string>)>
        result_or_failure_mailbox(
            mailbox_manager,
            [&](signal_t *, const boost::variant<read_response_t, std::string> &res) {
                result_or_failure.pulse(res);
            });

    wait_interruptible(token, interruptor);
    fifo_enforcer_read_token_t token_for_master = source_for_master.enter_read();
    token->end();

    master_business_card_t::read_request_t read_request(
        read,
        otok,
        token_for_master,
        result_or_failure_mailbox.get_address());

    multi_client_client.spawn_request(read_request);

    wait_any_t waiter(result_or_failure.get_ready_signal(), get_failed_signal());
    wait_interruptible(&waiter, interruptor);

    if (result_or_failure.is_pulsed()) {
        if (const std::string *error
            = boost::get<std::string>(&result_or_failure.wait())) {
            throw cannot_perform_query_exc_t(*error);
        } else if (const read_response_t *result =
                boost::get<read_response_t>(
                    &result_or_failure.wait())) {
            *response = *result;
        } else {
            unreachable();
        }
    } else {
        throw resource_lost_exc_t();
    }
}

void master_access_t::new_write_token(fifo_enforcer_sink_t::exit_write_t *out) {
    out->begin(&internal_fifo_sink, internal_fifo_source.enter_write());
}

void master_access_t::write(
        const write_t &write,
        write_response_t *response,
        order_token_t otok,
        fifo_enforcer_sink_t::exit_write_t *token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, resource_lost_exc_t, cannot_perform_query_exc_t) {
    rassert(region_is_superset(region, write.get_region()));

    promise_t<boost::variant<write_response_t, std::string> > result_or_failure;
    mailbox_t<void(boost::variant<write_response_t, std::string>)> result_or_failure_mailbox(
        mailbox_manager,
        [&](signal_t *, const boost::variant<write_response_t, std::string> &res) {
            result_or_failure.pulse(res);
        });

    wait_interruptible(token, interruptor);
    fifo_enforcer_write_token_t token_for_master = source_for_master.enter_write();
    token->end();

    master_business_card_t::write_request_t write_request(
        write,
        otok,
        token_for_master,
        result_or_failure_mailbox.get_address());

    multi_client_client.spawn_request(write_request);

    wait_any_t waiter(result_or_failure.get_ready_signal(), get_failed_signal());
    wait_interruptible(&waiter, interruptor);

    if (result_or_failure.get_ready_signal()->is_pulsed()) {
        if (const std::string *error = boost::get<std::string>(&result_or_failure.wait())) {
            throw cannot_perform_query_exc_t(*error);
        } else if (const write_response_t *result =
                boost::get<write_response_t>(&result_or_failure.wait())) {
            *response = *result;
        } else {
            unreachable();
        }
    } else {
        throw resource_lost_exc_t();
    }
}
