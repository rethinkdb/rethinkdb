// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/remote_replicator_server.hpp"

remote_replicator_server_t::remote_replicator_server_t(
        mailbox_manager_t *_mailbox_manager,
        primary_dispatcher_t *_primary) :
    mailbox_manager(_mailbox_manager),
    primary(_primary),
    registrar(mailbox_manager, this)
    { }

remote_replicator_server_t::proxy_replica_t::proxy_replica_t(
        remote_replicator_server_t *_parent,
        const remote_replicator_client_bcard_t &_client_bcard,
        UNUSED signal_t *interruptor) :
    client_bcard(_client_bcard), parent(_parent), is_ready(false),
    ready_mailbox(
        parent->mailbox_manager,
        std::bind(&proxy_replica_t::on_ready, this, ph::_1))
{
    state_timestamp_t first_timestamp;
    registration = make_scoped<primary_dispatcher_t::dispatchee_registration_t>(
        parent->primary, this, client_bcard.server_id, 1.0, &first_timestamp);
    send(parent->mailbox_manager, client_bcard.intro_mailbox,
        remote_replicator_client_intro_t { 
            first_timestamp,
            ready_mailbox.get_address() });
}

void remote_replicator_server_t::proxy_replica_t::do_read(
        const read_t &read,
        state_timestamp_t min_timestamp,
        signal_t *interruptor,
        read_response_t *response_out) {
    guarantee(is_ready);
    cond_t got_response;
    mailbox_t<void(read_response_t)> response_mailbox(
        parent->mailbox_manager,
        [&](signal_t *, const read_response_t &response) {
            *response_out = response;
            got_response.pulse();
        });
    send(parent->mailbox_manager, client_bcard.read_mailbox,
        read, min_timestamp, response_mailbox.get_address());
    wait_interruptible(&got_response, interruptor);
}

void remote_replicator_server_t::proxy_replica_t::do_write_sync(
        const write_t &write,
        state_timestamp_t timestamp,
        order_token_t order_token,
        write_durability_t durability,
        signal_t *interruptor,
        write_response_t *response_out) {
    guarantee(is_ready);
    cond_t got_response;
    mailbox_t<void(write_response_t)> response_mailbox(
        parent->mailbox_manager,
        [&](signal_t *, const write_response_t &response) {
            *response_out = response;
            got_response.pulse();
        });
    send(parent->mailbox_manager, client_bcard.write_sync_mailbox,
        write, timestamp, order_token, durability, response_mailbox.get_address());
    wait_interruptible(&got_response, interruptor);
}

void remote_replicator_server_t::proxy_replica_t::do_write_async(
        const write_t &write,
        state_timestamp_t timestamp,
        order_token_t order_token,
        signal_t *interruptor) {
    cond_t got_ack;
    mailbox_t<void()> ack_mailbox(
        parent->mailbox_manager,
        [&](signal_t *) { got_ack.pulse(); });
    send(parent->mailbox_manager, client_bcard.write_async_mailbox,
        write, timestamp, order_token, ack_mailbox.get_address());
    wait_interruptible(&got_ack, interruptor);
}

void remote_replicator_server_t::proxy_replica_t::on_ready(signal_t *) {
    guarantee(!is_ready);
    is_ready = true;
    registration->mark_ready();
}

