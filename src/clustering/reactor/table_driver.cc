// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/reactor/table_driver.hpp"

table_driver_t::table_driver_t(
        mailbox_manager_t *_mailbox_manager,
        watchable_map_t<peer_id_t, table_driver_business_card_t>
            *_table_driver_directory,
        watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_business_card_t>
            *_table_directory) :
    mailbox_manager(_mailbox_manager),
    table_driver_directory(_table_driver_directory),
    table_directory(_table_directory),
    persistent_file(_persistent_file),
    control_mailbox(mailbox_manager,
        std::bind(&table_driver_t::on_control, this, ph::_1, ph::_2, ph::_3, ph::_4))
{
    persistent_state = persistent_file->read_metadata();
    for (const auto &pair : persistent_state.tables) {
        meta_states[pair.first] = pair.second.meta_state;
        tables[pair.first] = make_scoped<table_t>(pair.second.raft_state);
    }
}

table_driver_business_card_t table_driver_t::get_table_driver_business_card() {
    table_driver_business_card_t b;
    b.control = control_mailbox.get_address();
    return b;
}

void table_driver_t::on_table_directory_change(
        const std::pair<peer_id_t, namespace_id_t> &key,
        const table_business_card_t *value) {
    /* Pass on the update to the `raft_networked_member_t` */
    auto tab_it = tables.find(key.second);
    if (tab_it != tables.end()) {
        /* Note that we filter out updates for peers which are in a different epoch; the
        `raft_networked_member_t` should only ever see business cards of other
        `raft_networked_member_t`s in the same epoch. */
        if (value != nullptr && value->meta_state.epoch == tab_it->second.epoch) {
            tab_it->second.peers.set_key(
                key.first,
                value->meta_state.member_id,
                value->raft_business_card);
        } else {
            tab_it->second.peers.delete_key(key.first);
        }
    }
    /* Send a control message to the peer if their new state warrants a control message
    */
    maybe_send_control(key.second, peer.first);
}

void table_driver_t::on_control(
        signal_t *interruptor,
        const namespace_id_t &table_id,
        const table_driver_meta_state_t &new_meta_state,
        const boost::optional<raft_persistent_state_t<table_raft_state_t> >
            &initial_raft_state) {

    table_driver_meta_state_t current_meta_state;
    if (meta_states.count(table_id) != 0) {
        current_meta_state = meta_states.at(table_id);
    }

    if (!meta_state_supersedes(new_meta_state, current_meta_state)) {
        return;
    }
    meta_states[table_id] = new_meta_state;

    if (current_meta_state.server_is_member &&
            (!new_meta_state.server_is_member ||
                new_meta_state.member_id != current_meta_state.member_id)) {
        guarantee(tables.count(table_id) == 1);
        tables.erase(table_id);
        persistent_state.tables.erase(table_id);
        persistent_file->update_metadata(persistent_state);
    }
    if (new_meta_state.server_is_member &&
            (!current_meta_state.server_is_member ||
                current_meta_state.member_id != new_meta_state.member_id)) {
        guarantee(tables.count(table_id) == 0);
        persistent_state.tables[table_id].meta_state = new_meta_state;
        persistent_state.tables[table_id].raft_state = *initial_raft_state;
        tables[table_id] = make_scoped<table_t>(*initial_raft_state);
    }
}

void table_driver_t::maybe_send_control(
        const namespace_id_t &table_id,
        const peer_id_t &peer_id) {
    /* First, look up the peer's mailbox and machine ID. If we can't find these we'll
    bail out; when they become available `maybe_send_control()` will be called again. */
    boost::optional<table_driver_business_card_t> peer_driver_bcard =
        table_driver_directory->get_key(peer_id);
    boost::optional<machine_id_t> server_id =
        server_name_client->get_peer_id_to_machine_id_map()->get_key(peer_id);
    if (!static_cast<bool>(peer_driver_bcard) && !static_cast<bool>(server_id)) {
        return;
    }

    /* Determine the peer's current state */
    boost::optional<table_business_card_t> peer_bcard =
        table_directory->get_key(std::make_pair(peer_id, table_id));
    table_driver_meta_state_t *peer_meta_state =
        static_cast<bool>(peer_bcard) ? &peer_bcard->meta_state : nullptr;

    /* Calculate what we think the peer's meta state ought to be. */
    table_driver_meta_state_t desired_meta_state;
    auto ms_it = meta_states.find(table_id);
    auto tab_it = tables.find(table_id);
    if (ms_it != meta_states.end() && ms_it->second.table_is_dropped) {
        /* If the table has been dropped, then we should inform the peer of that. This is
        the only situation where we send control messages even if we aren't a member of
        the Raft cluster. */
        desired_meta_state = ms_it->second;
    } else if (tab_it != tables.end()) {
        guarantee(ms_it != meta_states.end());
        // RSI: Fill in desired_meta_state using tab_it->member
    } else {
        /* The table isn't deleted, but we aren't a member of the Raft cluster for the
        table. So we don't have enough information to help the peer. We could
        hypothetically determine that it was in the wrong epoch, but since we're not a
        member of the Raft cluster, we wouldn't be able to send it the correct initial
        Raft state. So don't do anything. The servers that are in the Raft cluster will
        take care of the problem. */
        return;
    }

    /* Note that if the peer isn't a member of the Raft cluster, we can't actually
    determine its current meta-state, so we send a control message if the desired meta
    state indicates that the peer should be a member of the Raft cluster. It's possible
    that the peer will ignore our control message if it has a more up-to-date meta state
    that indicates that it's not supposed to be a member of the Raft cluster. */
    bool should_send_control =
        peer_meta_state != nullptr
            ? meta_state_supersedes(desired_meta_state, *peer_meta_state)
            : desired_meta_state.server_is_member;
    if (!should_send_control) {
        return;
    }

    /* Actually send the message now */
    boost::optional<raft_persistent_state_t<table_raft_state_t> > initial_raft_state;
    if (desired_meta_state.server_is_member) {
        initial_raft_state = // RSI: fill in using tab_it->member
    }
    auto_drainer_t::lock_t keepalive(&drainer);
    coro_t::spawn_sometime([this, table_id, peer_driver_bcard, desired_meta_state,
            initial_raft_state, keepalive /* important to capture */]() {
        send(mailbox_manager, peer_driver_bcard->control_mailbox,
            table_id, desired_meta_state, initial_raft_state);
    });
}

