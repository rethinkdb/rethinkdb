#include "clustering/immediate_consistency/query/master_access.hpp"

#include <math.h>

#include "arch/timing.hpp"
#include "concurrency/promise.hpp"

#define THROTTLE_THRESHOLD 200

template <class protocol_t>
master_access_t<protocol_t>::master_access_t(
        mailbox_manager_t *mm,
        const clone_ptr_t<watchable_t<boost::optional<boost::optional<master_business_card_t<protocol_t> > > > > &master,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, resource_lost_exc_t) :
    mailbox_manager(mm),
    master_access_id(generate_uuid()), allocated_writes(0),
    allocation_mailbox(mailbox_manager, boost::bind(&master_access_t<protocol_t>::on_allocation, this, _1), mailbox_callback_mode_inline)
{
    boost::optional<boost::optional<master_business_card_t<protocol_t> > > business_card = master->get();
    if (!business_card || !business_card.get()) {
        throw resource_lost_exc_t();
    }

    region = business_card.get().get().region;
    read_address = business_card.get().get().read_mailbox;
    write_address = business_card.get().get().write_mailbox;

    cond_t ack_cond;
    mailbox_t<void()> ack_mailbox(mailbox_manager, boost::bind(&cond_t::pulse, &ack_cond), mailbox_callback_mode_inline);

    registrant.reset(new registrant_t<master_access_business_card_t>(
        mailbox_manager,
        master->subview(&master_access_t<protocol_t>::extract_registrar_business_card),
        master_access_business_card_t(master_access_id, ack_mailbox.get_address(), allocation_mailbox.get_address())
        ));

    wait_interruptible(&ack_cond, interruptor);
}

template <class protocol_t>
void master_access_t<protocol_t>::new_read_token(fifo_enforcer_sink_t::exit_read_t *out) {
    out->begin(&internal_fifo_sink, internal_fifo_source.enter_read());
}

template <class protocol_t>
typename protocol_t::read_response_t master_access_t<protocol_t>::read(
        const typename protocol_t::read_t &read,
        order_token_t otok,
        fifo_enforcer_sink_t::exit_read_t *token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, resource_lost_exc_t, cannot_perform_query_exc_t) {
    rassert(region_is_superset(region, read.get_region()));

    promise_t<boost::variant<typename protocol_t::read_response_t, std::string> > result_or_failure;
    mailbox_t<void(boost::variant<typename protocol_t::read_response_t, std::string>)> result_or_failure_mailbox(
        mailbox_manager,
        boost::bind(&promise_t<boost::variant<typename protocol_t::read_response_t, std::string> >::pulse, &result_or_failure, _1),
        mailbox_callback_mode_inline);

    wait_interruptible(token, interruptor);
    fifo_enforcer_read_token_t token_for_master = source_for_master.enter_read();
    token->end();

    send(mailbox_manager, read_address,
        master_access_id,
        read,
        otok, token_for_master,
        result_or_failure_mailbox.get_address());

    wait_any_t waiter(result_or_failure.get_ready_signal(), get_failed_signal());
    wait_interruptible(&waiter, interruptor);

    if (result_or_failure.get_ready_signal()->is_pulsed()) {
        if (const std::string *error = boost::get<std::string>(&result_or_failure.get_value())) {
            throw cannot_perform_query_exc_t(*error);
        } else {
            return boost::get<typename protocol_t::read_response_t>(result_or_failure.get_value());
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
typename protocol_t::write_response_t master_access_t<protocol_t>::write(
        const typename protocol_t::write_t &write,
        order_token_t otok,
        fifo_enforcer_sink_t::exit_write_t *token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, resource_lost_exc_t, cannot_perform_query_exc_t) {
    rassert(region_is_superset(region, write.get_region()));

    promise_t<boost::variant<typename protocol_t::write_response_t, std::string> > result_or_failure;
    mailbox_t<void(boost::variant<typename protocol_t::write_response_t, std::string>)> result_or_failure_mailbox(
        mailbox_manager,
        boost::bind(&promise_t<boost::variant<typename protocol_t::write_response_t, std::string> >::pulse, &result_or_failure, _1),
        mailbox_callback_mode_inline);

    wait_interruptible(token, interruptor);
    fifo_enforcer_write_token_t token_for_master = source_for_master.enter_write();
    token->end();

    if (allocated_writes < THROTTLE_THRESHOLD) {
        nap(static_cast<int>(std::min(1000.0, pow(2, -(float(allocated_writes)/100.0)))), interruptor);
    }
    allocated_writes--;

    send(mailbox_manager, write_address,
        master_access_id,
        write,
        otok, token_for_master,
        result_or_failure_mailbox.get_address());

    wait_any_t waiter(result_or_failure.get_ready_signal(), get_failed_signal());
    wait_interruptible(&waiter, interruptor);

    if (result_or_failure.get_ready_signal()->is_pulsed()) {
        if (const std::string *error = boost::get<std::string>(&result_or_failure.get_value())) {
            throw cannot_perform_query_exc_t(*error);
        } else {
            return boost::get<typename protocol_t::write_response_t>(result_or_failure.get_value());
        }
    } else {
        throw resource_lost_exc_t();
    }
}

template <class protocol_t>
void master_access_t<protocol_t>::on_allocation(int amount) {
    allocated_writes += amount;
}

#include "rdb_protocol/protocol.hpp"
#include "memcached/protocol.hpp"
#include "mock/dummy_protocol.hpp"

template class master_access_t<rdb_protocol_t>;
template class master_access_t<memcached_protocol_t>;
template class master_access_t<mock::dummy_protocol_t>;
