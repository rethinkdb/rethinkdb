// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/local_replicator.hpp"

#include "concurrency/cross_thread_signal.hpp"

local_replicator_t::local_replicator_t(
        mailbox_manager_t *mailbox_manager,
        const server_id_t &server_id,
        primary_dispatcher_t *primary,
        store_view_t *_store,
        branch_history_manager_t *bhm,
        signal_t *interruptor) :
    store(_store),
    replica(
        mailbox_manager,
        store,
        bhm,
        primary->get_branch_id(),
        primary->get_branch_birth_certificate().initial_timestamp)
{
    order_source_t order_source;

#ifndef NDEBUG
    /* Make sure the store's initial value matches the initial value in the branch birth
    certificate (just as a sanity check) */
    read_token_t read_token;
    store->new_read_token(&read_token);
    region_map_t<binary_blob_t> origin_blob;
    store->do_get_metainfo(
        order_source.check_in("local_replica_t(read)").with_read_mode(),
        &read_token,
        interruptor,
        &origin_blob);
    guarantee(to_version_map(origin_blob) ==
        primary->get_branch_birth_certificate().origin);
    guarantee(store->get_region() ==
        primary->get_branch_birth_certificate().region);
#endif

    /* Store the new branch in the branch history manager. We have to do this before we
    put the branch in the store's metainfo so that if we crash, the branch will never be
    in the metainfo but not the branch history manager. */
    {
        cross_thread_signal_t interruptor_on_bhm_thread(interruptor, bhm->home_thread());
        on_thread_t thread_switcher(bhm->home_thread());
        bhm->create_branch(
            primary->get_branch_id(),
            primary->get_branch_birth_certificate(),
            &interruptor_on_bhm_thread);
    }

    /* Initialize the metainfo to the new branch. Note we use hard durability, to avoid
    the potential for subtle bugs. */
    write_token_t write_token;
    store->new_write_token(&write_token);
    store->set_metainfo(
        region_map_t<binary_blob_t>(
            store->get_region(),
            binary_blob_t(version_t(
                primary->get_branch_id(),
                primary->get_branch_birth_certificate().initial_timestamp))),
        order_source.check_in("local_replica_t(write)"),
        &write_token,
        write_durability_t::HARD,
        interruptor);

    state_timestamp_t first_timestamp;
    registration = make_scoped<primary_dispatcher_t::dispatchee_registration_t>(
        primary, this, server_id, 2.0, &first_timestamp);
    guarantee(first_timestamp ==
        primary->get_branch_birth_certificate().initial_timestamp);
    registration->mark_ready();
}

void local_replicator_t::do_read(
        const read_t &read,
        state_timestamp_t min_timestamp,
        signal_t *interruptor,
        read_response_t *response_out) {
    replica.do_read(read, min_timestamp, interruptor, response_out);
}

void local_replicator_t::do_write_sync(
        const write_t &write,
        state_timestamp_t timestamp,
        order_token_t order_token,
        write_durability_t durability,
        signal_t *interruptor,
        write_response_t *response_out) {
    replica.do_write(
        write, timestamp, order_token, durability,
        interruptor, response_out);
}

void local_replicator_t::do_write_async(
        const write_t &write,
        state_timestamp_t timestamp,
        order_token_t order_token,
        signal_t *interruptor) {
    write_response_t dummy;
    replica.do_write(
        write, timestamp, order_token, write_durability_t::SOFT,
        interruptor, &dummy);
}


