// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/servers/name_client.hpp"

#include "debug.hpp"

server_name_client_t::server_name_client_t(
        mailbox_manager_t *_mailbox_manager,
        clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t,
            cluster_directory_metadata_t> > > _directory_view,
        boost::shared_ptr<semilattice_readwrite_view_t<servers_semilattice_metadata_t> >
            _semilattice_view) :
    mailbox_manager(_mailbox_manager),
    directory_view(_directory_view),
    semilattice_view(_semilattice_view),
    server_id_to_peer_id_map(std::map<server_id_t, peer_id_t>()),
    peer_id_to_server_id_map(std::map<peer_id_t, server_id_t>()),
    name_to_server_id_map(std::multimap<name_string_t, server_id_t>()),
    server_id_to_name_map(std::map<server_id_t, name_string_t>()),
    directory_subs([this]() {
        this->recompute_server_id_to_peer_id_map();
        }),
    semilattice_subs([this]() {
        this->recompute_server_id_to_peer_id_map();
        this->recompute_name_to_server_id_map();
        })
{
    watchable_t< change_tracking_map_t<peer_id_t, cluster_directory_metadata_t> >
        ::freeze_t freeze(directory_view);
    directory_subs.reset(directory_view, &freeze);
    semilattice_subs.reset(semilattice_view);
    recompute_server_id_to_peer_id_map();
    recompute_name_to_server_id_map();
}

std::set<server_id_t> server_name_client_t::get_servers_with_tag(
        const name_string_t &tag) {
    std::set<server_id_t> servers;
    servers_semilattice_metadata_t md = semilattice_view->get();
    for (auto it = md.servers.begin();
              it != md.servers.end();
            ++it) {
        if (it->second.is_deleted()) {
            continue;
        }
        if (it->second.get_ref().tags.get_ref().count(tag) == 1) {
            servers.insert(it->first);
        }
    }
    return servers;
}

bool server_name_client_t::rename_server(
        const server_id_t &server_id,
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
    server_id_to_peer_id_map.apply_read(
        [&](const std::map<server_id_t, peer_id_t> *map) {
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
    name_to_server_id_map.apply_read(
        [&](const std::multimap<name_string_t, server_id_t> *map) {
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
        [&](UNUSED signal_t *interruptor2) {
            got_reply.pulse();
        });
    disconnect_watcher_t disconnect_watcher(mailbox_manager, rename_addr.get_peer());
    send(mailbox_manager, rename_addr, new_name, ack_mailbox.get_address());
    wait_any_t waiter(&got_reply, &disconnect_watcher);
    wait_interruptible(&waiter, interruptor);

    if (!got_reply.is_pulsed()) {
        *error_out = lost_contact_message;
        return false;
    }

    try {
        /* Don't return until the change is visible in the semilattices */
        semilattice_view->sync_from(peer, interruptor);
    } catch (const sync_failed_exc_t &) {
        *error_out = lost_contact_message;
        return false;
    }

    return true;
}

bool server_name_client_t::retag_server(
        const server_id_t &server_id,
        const name_string_t &server_name,
        const std::set<name_string_t> &new_tags,
        signal_t *interruptor,
        std::string *error_out) {

    /* We can produce this error message for several different reasons, so it's stored in
    a local variable instead of typed out multiple times. */
    std::string lost_contact_message = strprintf("Cannot change the tags of server `%s` "
        "because we lost contact with it.", server_name.c_str());

    peer_id_t peer;
    server_id_to_peer_id_map.apply_read(
        [&](const std::map<server_id_t, peer_id_t> *map) {
            auto it = map->find(server_id);
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
        [&](UNUSED signal_t *interruptor2) {
            got_reply.pulse();
        });
    disconnect_watcher_t disconnect_watcher(mailbox_manager, retag_addr.get_peer());
    send(mailbox_manager, retag_addr, new_tags, ack_mailbox.get_address());
    wait_any_t waiter(&got_reply, &disconnect_watcher);
    wait_interruptible(&waiter, interruptor);

    if (!got_reply.is_pulsed()) {
        *error_out = lost_contact_message;
        return false;
    }

    try {
        /* Don't return until the change is visible in the semilattices */
        semilattice_view->sync_from(peer, interruptor);
    } catch (const sync_failed_exc_t &) {
        *error_out = lost_contact_message;
        return false;
    }

    return true; 
}

bool server_name_client_t::permanently_remove_server(const name_string_t &name,
        std::string *error_out) {
    assert_thread();

    server_id_t server_id = nil_uuid();
    bool name_collision = false;
    name_to_server_id_map.apply_read(
        [&](const std::multimap<name_string_t, server_id_t> *map) {
            if (map->count(name) > 1) {
                name_collision = true;
            } else {
                auto it = map->find(name);
                if (it != map->end()) {
                    server_id = it->second;
                }
            }
        });
    if (name_collision) {
        *error_out = strprintf("There are multiple servers called `%s`.", name.c_str());
        return false;
    }
    if (server_id == nil_uuid()) {
        *error_out = strprintf("Server `%s` does not exist.", name.c_str());
        return false;
    }
    
    bool is_visible;
    server_id_to_peer_id_map.apply_read(
        [&](const std::map<server_id_t, peer_id_t> *map) {
            is_visible = (map->count(server_id) != 0);
        });
    if (is_visible) {
        *error_out = strprintf("Server `%s` is currently connected to the cluster, so "
            "it should not be permanently removed.", name.c_str());
        return false;
    }

    servers_semilattice_metadata_t sl_metadata = semilattice_view->get();
    sl_metadata.servers.at(server_id).mark_deleted();
    semilattice_view->join(sl_metadata);
    return true;
}

void server_name_client_t::recompute_name_to_server_id_map() {
    std::multimap<name_string_t, server_id_t> new_map_n2m;
    std::map<server_id_t, name_string_t> new_map_m2n;
    servers_semilattice_metadata_t sl_metadata = semilattice_view->get();
    for (auto sl_it = sl_metadata.servers.begin();
              sl_it != sl_metadata.servers.end();
            ++sl_it) {
        if (sl_it->second.is_deleted()) {
            continue;
        }
        name_string_t name = sl_it->second.get_ref().name.get_ref();
        new_map_n2m.insert(std::make_pair(name, sl_it->first));
        auto res2 = new_map_m2n.insert(std::make_pair(sl_it->first, name));
        if (!res2.second) {
            crash("Two servers have the same server ID.");
        }
    }
    name_to_server_id_map.set_value(new_map_n2m);
    server_id_to_name_map.set_value(new_map_m2n);
}

void server_name_client_t::recompute_server_id_to_peer_id_map() {
    std::map<server_id_t, peer_id_t> new_map_s2p;
    std::map<peer_id_t, server_id_t> new_map_p2s;
    directory_view->apply_read(
        [&](const change_tracking_map_t<peer_id_t, cluster_directory_metadata_t> *dir) {
            for (auto dir_it = dir->get_inner().begin();
                      dir_it != dir->get_inner().end();
                    ++dir_it) {
                new_map_s2p.insert(std::make_pair(
                    dir_it->second.server_id, dir_it->first));
                new_map_p2s.insert(std::make_pair(
                    dir_it->first, dir_it->second.server_id));
            }
    });
    server_id_to_peer_id_map.set_value(new_map_s2p);
    peer_id_to_server_id_map.set_value(new_map_p2s);
}

