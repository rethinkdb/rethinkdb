// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/query_routing/primary_query_client.hpp"

#include <math.h>

#include <functional>

#include "arch/timing.hpp"
#include "concurrency/promise.hpp"
#include "containers/archive/boost_types.hpp"
#include "rdb_protocol/protocol.hpp"

// TODO: Was this macro supposed to be used?
// #define THROTTLE_THRESHOLD 200

primary_query_client_t::primary_query_client_t(
        mailbox_manager_t *mm,
        const primary_query_bcard_t &master,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) :
    mailbox_manager(mm),
    region(master.region),
    multi_client_client(
        mailbox_manager,
        master.multi_client,
        interruptor)
    { }

void primary_query_client_t::new_read_token(fifo_enforcer_sink_t::exit_read_t *out) {
    out->begin(&internal_fifo_sink, internal_fifo_source.enter_read());
}

void primary_query_client_t::read(
        const read_t &read,
        read_response_t *response,
        order_token_t otok,
        fifo_enforcer_sink_t::exit_read_t *token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t) {
    rassert(region_is_superset(region, read.get_region()));
    otok.assert_read_mode();

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

    primary_query_bcard_t::read_request_t read_request(
        read,
        otok,
        token_for_master,
        result_or_failure_mailbox.get_address());

    multi_client_client.spawn_request(read_request);

    wait_interruptible(result_or_failure.get_ready_signal(), interruptor);

    if (const std::string *error = boost::get<std::string>(&result_or_failure.wait())) {
        throw cannot_perform_query_exc_t(*error);
    } else if (const read_response_t *result =
            boost::get<read_response_t>(
                &result_or_failure.wait())) {
        *response = *result;
    } else {
        unreachable();
    }
}

void primary_query_client_t::new_write_token(fifo_enforcer_sink_t::exit_write_t *out) {
    out->begin(&internal_fifo_sink, internal_fifo_source.enter_write());
}

void primary_query_client_t::write(
        const write_t &write,
        write_response_t *response,
        order_token_t otok,
        fifo_enforcer_sink_t::exit_write_t *token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t) {
    rassert(region_is_superset(region, write.get_region()));
    otok.assert_write_mode();

    promise_t<boost::variant<write_response_t, std::string> > result_or_failure;
    mailbox_t<void(boost::variant<write_response_t, std::string>)> result_or_failure_mailbox(
        mailbox_manager,
        [&](signal_t *, const boost::variant<write_response_t, std::string> &res) {
            result_or_failure.pulse(res);
        });

    wait_interruptible(token, interruptor);
    fifo_enforcer_write_token_t token_for_master = source_for_master.enter_write();
    token->end();

    primary_query_bcard_t::write_request_t write_request(
        write,
        otok,
        token_for_master,
        result_or_failure_mailbox.get_address());

    multi_client_client.spawn_request(write_request);

    wait_interruptible(result_or_failure.get_ready_signal(), interruptor);

    if (const std::string *error = boost::get<std::string>(&result_or_failure.wait())) {
        throw cannot_perform_query_exc_t(*error);
    } else if (const write_response_t *result =
            boost::get<write_response_t>(&result_or_failure.wait())) {
        *response = *result;
    } else {
        unreachable();
    }
}
