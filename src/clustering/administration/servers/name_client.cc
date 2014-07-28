// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/servers/name_client.hpp"

server_name_client_t::server_name_client_t(
        mailbox_manager_t *_mailbox_manager,
        clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t,
            cluster_directory_metadata_t> > > _directory_view,
        boost::shared_ptr<semilattice_readwrite_view_t<machines_semilattice_metadata_t> >
            _semilattice_view) :
    mailbox_manager(_mailbox_manager),
    directory_view(_directory_view),
    semilattice_view(_semilattice_view),
    name_map(std::map<name_string_t, peer_id_t>()),
    directory_subs([this]() { this->recompute_name_map(); }),
    semilattice_subs([this]() { this->recompute_name_map(); })
{
    watchable_t<change_tracking_map_t<peer_id_t, cluster_directory_metadata_t> >
        ::freeze_t freeze(directory_view);
    directory_subs.reset(directory_view, &freeze);
    semilattice_subs.reset(semilattice_view);
    recompute_name_map();
}

bool server_name_client_t::rename_server(const name_string_t &old_name,
        const name_string_t &new_name, signal_t *interruptor, std::string *error_out) {

    assert_thread();

    /* We can produce this error message for several different reasons, so it's stored in
    a local variable instead of typed out multiple times. */
    std::string lost_contact_message = strprintf("Cannot rename server `%s` because we "
        "lost contact with it.", old_name.c_str());

    /* Look up `new_name` and make sure it's not currently in use. Also check if
    `old_name` is present; that information helps provide better error messages. */
    bool old_name_in_sl = false, new_name_in_sl = false;
    machines_semilattice_metadata_t sl_metadata = semilattice_view->get();
    for (auto it = sl_metadata.machines.begin();
              it != sl_metadata.machines.end();
            ++it) {
        if (!it->second.is_deleted()) {
            it_name = it->second.get_ref().get_ref().name;
            if (it_name == old_name) old_name_present = true;
            if (it_name == new_name) new_name_present = true;
        }
    }

    /* Look up `old_name` and figure out which peer it corresponds to */
    peer_id_t peer;
    name_map.apply_read([&](const std::map<name_string_t, peer_id_t> *map) {
        auto it = map->find(old_name);
        if (it != map->end()) {
            peer = it->second;
        }
    });
    if (peer.is_nil()) {
        *error_out = old_name_in_sl
            ? lost_contact_message
            : strprintf("Server `%s` does not exist.", old_name.c_str());
        return false;
    }

    if (old_name == new_name) {
        /* This is a no-op */
        return true;
    }

    if (new_name_in_sl) {
        *error_out = strprintf("Server `%s` already exists.", new_name.c_str());
        return false;
    }

    server_name_business_card_t::rename_mailbox_t::addr_t rename_addr;
    directory_view->apply_read(
        [&](const change_tracking_map_t<peer_id_t, cluster_directory_metadata_t>
                *dir_metadata) {
            auto it = dir_metadata->get_inner().find(peer);
            if (it != dir_metadata->get_inner().end()) {
                guarantee(it->second.server_name_business_card, "We shouldn't be trying "
                    "to rename a proxy.");
                rename_addr = it->second.server_name_business_card.get().rename_addr;
            }
        });
    if (rename_addr.is_nil()) {
        *error_out = lost_contact_message;
        return false;
    }

    cond_t got_reply;
    mailbox_t<void()> ack_mailbox([&]() { got_reply.pulse(); });
    disconnect_watcher_t disconnect_watcher(mailbox_manager, rename_addr.get_peer());
    send(mailbox_manager, rename_addr, new_name, ack_mailbox.get_addr());
    wait_any_t waiter(&got_reply, &disconnect_watcher);
    wait_interruptible(&waiter, interruptor);

    if (!got_reply.is_pulsed()) {
        *error_out = lost_contact_message;
        return false;
    }

    return true; 
}

bool server_name_client_t::permanently_remove_server(const name_string_t &name,
        std::string *error_out) {
    assert_thread();

    std::set<machine_id_t> machine_ids;
    machines_semilattice_metadata_t sl_metadata = semilattice_view->get();
    for (auto it = sl_metadata.machines.begin();
              it != sl_metadata.machines.end();
            ++it) {
        if (!it->second.is_deleted()) {
            if (it->second.get_ref().get_ref().name == name) {
                machine_ids.insert(it->first);
            }
        }
    }
    if (machine_ids.empty()) {
        *error_out = strprintf("Server `%s` does not exist.", name.c_str());
        return false;
    }
    
    bool is_visible = false;
    directory_view->apply_read(
        [&](const change_tracking_map_t<peer_id_t, cluster_directory_metadata_t>
                *dir_metadata) {
            for (auto it = dir_metadata->get_inner().first();
                      it != dir_metadata->get_inner().end();
                    ++it) {
                if (machine_ids.count(it->second.machine_id) != 0) {
                    is_visible = true;
                    break;
                }
            }
        }); 
    if (is_visible) {
        *error_out = strprintf("Server `%s` is currently connected to the cluster, so "
            "it should not be permanently removed.", name.c_str());
        return false;
    }

    for (const machine_id_t &machine_id : machine_ids) {
        sl_metadata.servers.at(machine_id).mark_deleted();
    }
    semilattice_view->join(sl_metadata);
    return true;
}

void server_name_client_t::recompute_name_map() {
    std::map<name_string_t, peer_id_t> new_name_map;
    bool name_collision = false;
    machines_semilattice_metadata_t sl_metadata = semilattice_view->get();
    directory_view->apply_read([&](const change_tracking_map_t<peer_id_t,
            cluster_directory_metadata_t> *dir_metadata) {
        for (auto dir_it = dir_metadata->begin();
                  dir_it != dir_metadata->end();
                ++dir_it) {
            auto sl_it = sl_metadata.servers->find(dir_it->second.machine_id);
            if (sl_it == sl_metadata.servers->end()) {
                /* This situation is unlikely, but it could occur briefly during startup
                */
                continue;
            }
            if (sl_it->second.is_deleted()) {
                continue;
            }
            name_string_t name = sl_it->second.get_ref().get_ref().name;
            auto res = new_name_map.insert(std::make_pair(name, it->first));
            if (res.second) {
                name_collision = true;
                break;
            }
        }
    });
    if (!name_collision) {
        name_map.set_value(new_name_map);
    }
}

