// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_manager/table_meta_client.hpp"

#include "clustering/generic/raft_core.tcc"

table_meta_client_t::table_meta_client_t(
        mailbox_manager_t *_mailbox_manager,
        watchable_map_t<peer_id_t, table_meta_manager_business_card_t>
            *_table_meta_manager_directory,
        watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_meta_business_card_t>
            *_table_meta_directory) :
    mailbox_manager(_mailbox_manager),
    table_meta_manager_directory(_table_meta_manager_directory),
    table_meta_directory(_table_meta_directory)
    { }

bool table_meta_client_t::create(
        const table_config_t &initial_config,
        signal_t *interruptor,
        namespace_id_t *table_id_out) {
    *table_id_out = generate_uuid();

    table_meta_manager_business_card_t::action_timestamp_t timestamp;
    timestamp.epoch.timestamp = current_microtime();
    timestamp.epoch.id = generate_uuid();
    timestamp.log_index = 0;

    std::set<server_id_t> servers;
    for (const table_config_t::shard_t &shard : initial_config.shards) {
        servers.insert(shard.replicas.begin(), shard.replicas.end());
    }

    table_raft_state_t raft_state;
    raft_state.config = initial_config;

    raft_config_t raft_config;
    for (const server_id_t &server_id : servers) {
        raft_member_id_t member_id = generate_uuid();
        raft_state.member_ids[server_id] = member_id;
        raft_config.voting_members.insert(member_id);
    }

    raft_persistent_state_t<table_raft_state_t> raft_ps =
        raft_persistent_state_t<table_raft_state_t>::make_initial(
            raft_state, raft_config);

    std::map<server_id_t, table_meta_manager_business_card_t> bcards;
    table_meta_manager_directory->read_all(
        [&](const peer_id_t &, const table_meta_manager_business_card_t *bc) {
            if (servers.count(bc->server_id) == 1) {
                bcards[bc->server_id] = *bc;
            }
        });

    size_t acks = 0;
    pmap(bcards.begin(), bcards.end(),
    [&](const std::pair<server_id_t, table_meta_manager_business_card_t> &pair) {
        try {
            disconnect_watcher_t dw(mailbox_manager,
                pair.second.action_mailbox.get_peer());
            cond_t got_ack;
            mailbox_t<void()> ack_mailbox(mailbox_manager,
                [&](signal_t *) { got_ack.pulse(); });
            send(mailbox_manager, pair.second.action_mailbox,
                *table_id_out,
                timestamp,
                false,
                boost::optional<raft_member_id_t>(raft_state.member_ids.at(pair.first)),
                boost::optional<raft_persistent_state_t<table_raft_state_t> >(raft_ps),
                ack_mailbox.get_address());
            wait_any_t interruptor2(&dw, interruptor);
            wait_interruptible(&got_ack, &interruptor2);
            ++acks;
        } catch (const interrupted_exc_t &) {
            /* do nothing */
        }
    });

    return (acks > 0);
}

bool table_meta_client_t::drop(
        const namespace_id_t &table_id,
        signal_t *interruptor) {
    /* Construct a special timestamp that supersedes all regular timestamps */
    table_meta_manager_business_card_t::action_timestamp_t drop_timestamp;
    drop_timestamp.epoch.timestamp = std::numeric_limits<microtime_t>::max();
    drop_timestamp.epoch.id = nil_uuid();
    drop_timestamp.log_index = std::numeric_limits<raft_log_index_t>::max();

    std::map<server_id_t, table_meta_manager_business_card_t> bcards;
    table_meta_directory->read_all(
    [&](const std::pair<peer_id_t, namespace_id_t> &key,
            const table_meta_business_card_t *) {
        if (key.second == table_id) {
            table_meta_manager_directory->read_key(key.first,
            [&](const table_meta_manager_business_card_t *bc) {
                if (bc != nullptr) {
                    bcards[bc->server_id] = *bc;
                }
            });
        }
    });

    size_t acks = 0;
    pmap(bcards.begin(), bcards.end(),
    [&](const std::pair<server_id_t, table_meta_manager_business_card_t> &pair) {
        try {
            disconnect_watcher_t dw(mailbox_manager,
                pair.second.action_mailbox.get_peer());
            cond_t got_ack;
            mailbox_t<void()> ack_mailbox(mailbox_manager,
                [&](signal_t *) { got_ack.pulse(); });
            send(mailbox_manager, pair.second.action_mailbox,
                table_id, drop_timestamp, true, boost::optional<raft_member_id_t>(),
                boost::optional<raft_persistent_state_t<table_raft_state_t> >(),
                ack_mailbox.get_address());
            wait_any_t interruptor2(&dw, interruptor);
            wait_interruptible(&got_ack, &interruptor2);
            ++acks;
        } catch (const interrupted_exc_t &) {
            /* do nothing */
        }
    });

    return (acks > 0);
}

bool table_meta_client_t::get_config(
        const namespace_id_t &table_id,
        signal_t *interruptor,
        table_config_t *config_out) {
    table_meta_manager_business_card_t::get_config_mailbox_t::address_t best_mailbox;
    table_meta_manager_business_card_t::action_timestamp_t::epoch_t best_epoch;
    table_meta_directory->read_all(
    [&](const std::pair<peer_id_t, namespace_id_t> &key,
            const table_meta_business_card_t *table_bcard) {
        if (key.second == table_id) {
            table_meta_manager_directory->read_key(key.first,
            [&](const table_meta_manager_business_card_t *server_bcard) {
                if (server_bcard != nullptr) {
                    if (best_mailbox.is_nil() ||
                            table_bcard->epoch.supersedes(best_epoch)) {
                        best_mailbox = server_bcard->get_config_mailbox;
                        best_epoch = table_bcard->epoch;
                    }
                }
            });
        }
    });
    if (best_mailbox.is_nil()) {
        return false;
    }
    disconnect_watcher_t dw(mailbox_manager, best_mailbox.get_peer());
    promise_t<boost::optional<table_config_t> > promise;
    mailbox_t<void(boost::optional<table_config_t>)> ack_mailbox(mailbox_manager,
        [&](signal_t *, const boost::optional<table_config_t> &config) {
            promise.pulse(config);
        });
    send(mailbox_manager, best_mailbox, table_id, ack_mailbox.get_address());
    wait_any_t interruptor2(&dw, interruptor);
    wait_interruptible(promise.get_ready_signal(), &interruptor2);
    boost::optional<table_config_t> maybe_result = promise.wait();
    if (!static_cast<bool>(maybe_result)) {
        return false;
    }
    *config_out = *maybe_result;
    return true;
}

bool table_meta_client_t::set_config(
        const namespace_id_t &table_id,
        const table_config_t &new_config,
        signal_t *interruptor) {
    table_meta_manager_business_card_t::set_config_mailbox_t::address_t best_mailbox;
    table_meta_manager_business_card_t::action_timestamp_t::epoch_t best_epoch;
    table_meta_directory->read_all(
    [&](const std::pair<peer_id_t, namespace_id_t> &key,
            const table_meta_business_card_t *table_bcard) {
        if (key.second == table_id) {
            table_meta_manager_directory->read_key(key.first,
            [&](const table_meta_manager_business_card_t *server_bcard) {
                if (server_bcard != nullptr && table_bcard->is_leader) {
                    if (best_mailbox.is_nil() ||
                            table_bcard->epoch.supersedes(best_epoch)) {
                        best_mailbox = server_bcard->set_config_mailbox;
                        best_epoch = table_bcard->epoch;
                    }
                }
            });
        }
    });
    if (best_mailbox.is_nil()) {
        return false;
    }
    disconnect_watcher_t dw(mailbox_manager, best_mailbox.get_peer());
    promise_t<bool> promise;
    mailbox_t<void(bool)> ack_mailbox(mailbox_manager,
        [&](signal_t *, bool ok) {
            promise.pulse(ok);
        });
    send(mailbox_manager, best_mailbox, table_id, new_config, ack_mailbox.get_address());
    wait_any_t interruptor2(&dw, interruptor);
    wait_interruptible(promise.get_ready_signal(), &interruptor2);
    return promise.wait();
}

