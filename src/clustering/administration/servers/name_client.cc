// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/servers/name_client.hpp"

#include "debug.hpp"

server_name_client_t::server_name_client_t(
        mailbox_manager_t *_mailbox_manager,
        clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t,
            cluster_directory_metadata_t> > > _directory_view,
        boost::shared_ptr<semilattice_readwrite_view_t<machines_semilattice_metadata_t> >
            _semilattice_view) :
    mailbox_manager(_mailbox_manager),
    directory_view(_directory_view),
    semilattice_view(_semilattice_view),
    machine_id_to_peer_id_map(std::map<machine_id_t, peer_id_t>()),
    peer_id_to_machine_id_map(std::map<peer_id_t, machine_id_t>()),
    name_to_machine_id_map(std::multimap<name_string_t, machine_id_t>()),
    machine_id_to_name_map(std::map<machine_id_t, name_string_t>()),
    directory_subs([this]() {
        this->recompute_machine_id_to_peer_id_map();
        }),
    semilattice_subs([this]() {
        this->recompute_machine_id_to_peer_id_map();
        this->recompute_name_to_machine_id_map();
        })
{
    watchable_t< change_tracking_map_t<peer_id_t, cluster_directory_metadata_t> >
        ::freeze_t freeze(directory_view);
    directory_subs.reset(directory_view, &freeze);
    semilattice_subs.reset(semilattice_view);
    recompute_machine_id_to_peer_id_map();
    recompute_name_to_machine_id_map();
}

std::set<name_string_t> server_name_client_t::get_servers_with_tag(
        const name_string_t &tag) {
    std::set<name_string_t> servers;
    machines_semilattice_metadata_t md = semilattice_view->get();
    for (auto it = md.machines.begin();
              it != md.machines.end();
            ++it) {
        if (it->second.is_deleted()) {
            continue;
        }
        if (it->second.get_ref().tags.get_ref().count(tag) == 1) {
            servers.insert(it->second.get_ref().name.get_ref());
        }
    }
    return servers;
}

bool server_name_client_t::rename_server(
        const machine_id_t &server_id,
        const name_string_t &server_name,
        const name_string_t &new_name,
        signal_t *interruptor,
        std::string *error_out) {

    this->assert_thread();

    /* We can produce this error message for several different reasons, so it's stored in
    a local variable instead of typed out multiple times. */
    std::string lost_contact_message = strprintf("Cannot rename server `%s` because we "
        "lost contact with it.", server_name.c_str());

    peer_id_t peer;
    machine_id_to_peer_id_map.apply_read(
        [&](const std::map<machine_id_t, peer_id_t> *map) {
            auto it = map->find(server_id);
            if (it != map->end()) {
                peer = it->second;
            }
        });
    if (peer.is_nil()) {
        *error_out = lost_contact_message;
        return false;
    }

    if (server_name == new_name) {
        /* This is a no-op */
        return true;
    }

    bool new_name_in_use = false;
    name_to_machine_id_map.apply_read(
        [&](const std::multimap<name_string_t, machine_id_t> *map) {
            new_name_in_use = (map->count(new_name) != 0);
        });
    if (new_name_in_use) {
        *error_out = strprintf("Cannot rename server `%s` to `%s` because server `%s` "
            "already exists.", server_name.c_str(), new_name.c_str(), new_name.c_str());
        return false;
    }

    server_name_business_card_t::rename_mailbox_t::address_t rename_addr;
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
    mailbox_t<void()> ack_mailbox(
        mailbox_manager,
        [&]() { got_reply.pulse(); }); // NOLINT whitespace/newline
    disconnect_watcher_t disconnect_watcher(mailbox_manager, rename_addr.get_peer());
    send(mailbox_manager, rename_addr, new_name, ack_mailbox.get_address());
    wait_any_t waiter(&got_reply, &disconnect_watcher);
    wait_interruptible(&waiter, interruptor);

    if (!got_reply.is_pulsed()) {
        *error_out = lost_contact_message;
        return false;
    }

    return true;
}

bool server_name_client_t::retag_server(
        const machine_id_t &machine_id,
        const name_string_t &server_name,
        const std::set<name_string_t> &new_tags,
        signal_t *interruptor,
        std::string *error_out) {

    /* We can produce this error message for several different reasons, so it's stored in
    a local variable instead of typed out multiple times. */
    std::string lost_contact_message = strprintf("Cannot change the tags of server `%s` "
        "because we lost contact with it.", server_name.c_str());

    peer_id_t peer;
    machine_id_to_peer_id_map.apply_read(
        [&](const std::map<machine_id_t, peer_id_t> *map) {
            auto it = map->find(machine_id);
            if (it != map->end()) {
                peer = it->second;
            }
        });
    if (peer.is_nil()) {
        *error_out = lost_contact_message;
        return false;
    }

    server_name_business_card_t::retag_mailbox_t::address_t retag_addr;
    directory_view->apply_read(
        [&](const change_tracking_map_t<peer_id_t, cluster_directory_metadata_t>
                *dir_metadata) {
            auto it = dir_metadata->get_inner().find(peer);
            if (it != dir_metadata->get_inner().end()) {
                guarantee(it->second.server_name_business_card, "We shouldn't be trying "
                    "to retag a proxy.");
                retag_addr = it->second.server_name_business_card.get().retag_addr;
            }
        });
    if (retag_addr.is_nil()) {
        *error_out = lost_contact_message;
        return false;
    }

    cond_t got_reply;
    mailbox_t<void()> ack_mailbox(
        mailbox_manager,
        [&]() { got_reply.pulse(); }); // NOLINT whitespace/newline
    disconnect_watcher_t disconnect_watcher(mailbox_manager, retag_addr.get_peer());
    send(mailbox_manager, retag_addr, new_tags, ack_mailbox.get_address());
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

    machine_id_t machine_id = nil_uuid();
    bool name_collision = false;
    name_to_machine_id_map.apply_read(
        [&](const std::multimap<name_string_t, machine_id_t> *map) {
            if (map->count(name) > 1) {
                name_collision = true;
            } else {
                auto it = map->find(name);
                if (it != map->end()) {
                    machine_id = it->second;
                }
            }
        });
    if (name_collision) {
        *error_out = strprintf("There are multiple servers called `%s`.", name.c_str());
        return false;
    }
    if (machine_id == nil_uuid()) {
        *error_out = strprintf("Server `%s` does not exist.", name.c_str());
        return false;
    }
    
    bool is_visible;
    machine_id_to_peer_id_map.apply_read(
        [&](const std::map<machine_id_t, peer_id_t> *map) {
            is_visible = (map->count(machine_id) != 0);
        });
    if (is_visible) {
        *error_out = strprintf("Server `%s` is currently connected to the cluster, so "
            "it should not be permanently removed.", name.c_str());
        return false;
    }

    machines_semilattice_metadata_t sl_metadata = semilattice_view->get();
    sl_metadata.machines.at(machine_id).mark_deleted();
    semilattice_view->join(sl_metadata);
    return true;
}

void server_name_client_t::recompute_name_to_machine_id_map() {
    std::multimap<name_string_t, machine_id_t> new_map_n2m;
    std::map<machine_id_t, name_string_t> new_map_m2n;
    machines_semilattice_metadata_t sl_metadata = semilattice_view->get();
    for (auto sl_it = sl_metadata.machines.begin();
              sl_it != sl_metadata.machines.end();
            ++sl_it) {
        if (sl_it->second.is_deleted()) {
            continue;
        }
        name_string_t name = sl_it->second.get_ref().name.get_ref();
        new_map_n2m.insert(std::make_pair(name, sl_it->first));
        auto res2 = new_map_m2n.insert(std::make_pair(sl_it->first, name));
        if (!res2.second) {
            crash("Two servers have the same machine ID.");
        }
    }
    name_to_machine_id_map.set_value(new_map_n2m);
    machine_id_to_name_map.set_value(new_map_m2n);
}

void server_name_client_t::recompute_machine_id_to_peer_id_map() {
    std::map<machine_id_t, peer_id_t> new_map_m2p;
    std::map<peer_id_t, machine_id_t> new_map_p2m;
    machines_semilattice_metadata_t sl_metadata = semilattice_view->get();
    for (auto sl_it = sl_metadata.machines.begin();
              sl_it != sl_metadata.machines.end();
            ++sl_it) {
        if (sl_it->second.is_deleted()) {
            continue;
        }
        new_map_m2p.insert(std::make_pair(sl_it->first, peer_id_t()));
    }
    directory_view->apply_read(
        [&](const change_tracking_map_t<peer_id_t, cluster_directory_metadata_t> *dir) {
            for (auto dir_it = dir->get_inner().begin();
                      dir_it != dir->get_inner().end();
                    ++dir_it) {
                auto map_it = new_map_m2p.find(dir_it->second.machine_id);
                if (map_it == new_map_m2p.end()) {
                    /* This machine is in the directory but not the semilattices. This
                    can happen temporarily at startup, or if a permanently removed server
                    comes back online. Ignore it. */
                    continue;
                }
                guarantee(map_it->second.is_nil(),
                    "Two servers have the same machine ID.");
                map_it->second = dir_it->first;
                new_map_p2m.insert(std::make_pair(
                    dir_it->first, dir_it->second.machine_id));
            }
    });
    machine_id_to_peer_id_map.apply_atomic_op(
        [&](std::map<machine_id_t, peer_id_t> *map) -> bool {
            if (*map == new_map_m2p) {
                return false;
            } else {
                *map = new_map_m2p;
                return true;
            }
        });
    peer_id_to_machine_id_map.apply_atomic_op(
        [&](std::map<peer_id_t, machine_id_t> *map) -> bool {
            if (*map == new_map_p2m) {
                return false;
            } else {
                *map = new_map_p2m;
                return true;
            }
        });
}

